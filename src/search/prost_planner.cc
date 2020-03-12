#include "prost_planner.h"

#include "iterative_deepening_search.h"
#include "minimal_lookahead_search.h"
#include "search_engine.h"

#include "utils/logger.h"
#include "utils/math_utils.h"
#include "utils/stopwatch.h"
#include "utils/string_utils.h"
#include "utils/system_utils.h"

#include <iostream>

using namespace std;

ProstPlanner::ProstPlanner(string& plannerDesc)
    : searchEngine(nullptr),
      currentState(SearchEngine::initialState),
      currentRound(-1),
      currentStep(-1),
      stepsToGo(SearchEngine::horizon),
      executedActionIndex(-1),
      numberOfRounds(-1),
      cachingEnabled(true),
      ramLimit(2097152),
      bitSize(sizeof(long) * 8),
      tmMethod(NONE) {
    setSeed((int)time(nullptr));

    StringUtils::trim(plannerDesc);
    assert(plannerDesc[0] == '[' && plannerDesc[plannerDesc.size() - 1] == ']');
    StringUtils::removeFirstAndLastCharacter(plannerDesc);
    StringUtils::trim(plannerDesc);

    assert(plannerDesc.find("PROST") == 0);
    plannerDesc = plannerDesc.substr(5, plannerDesc.size());

    bool searchEngineDefined = false;

    while (!plannerDesc.empty()) {
        string param;
        string value;
        StringUtils::nextParamValuePair(plannerDesc, param, value);

        if (param == "-s") {
            setSeed(atoi(value.c_str()));
        } else if (param == "-ram") {
            setRAMLimit(atoi(value.c_str()));
        } else if (param == "-bit") {
            setBitSize(atoi(value.c_str()));
        } else if (param == "-tm") {
            if (value == "UNI") {
                setTimeoutManagementMethod(UNIFORM);
            } else if (value == "NONE") {
                setTimeoutManagementMethod(NONE);
            } else {
                SystemUtils::abort("Illegal timeout management method: " +
                                   value);
            }
        } else if (param == "-se") {
            setSearchEngine(SearchEngine::fromString(value));
            searchEngineDefined = true;
        } else if (param == "-log") {
            if (value == "SILENT") {
                Logger::runVerbosity = Verbosity::SILENT;
            } else if (value == "NORMAL") {
                Logger::runVerbosity = Verbosity::NORMAL;
            } else if (value == "VERBOSE") {
                Logger::runVerbosity = Verbosity::VERBOSE;
            } else if (value == "DEBUG") {
                Logger::runVerbosity = Verbosity::DEBUG;
            } else {
                SystemUtils::abort("No valid value for -logLine: " + value);
            }
        } else {
            SystemUtils::abort(
                "Unused parameter value pair: " + param + " / " + value);
        }
    }

    if (!searchEngineDefined) {
        SystemUtils::abort(
            "Error: search engine specification is insufficient. Rerun PROST "
            "without parameters to see detailed instructions.");
    }
}

void ProstPlanner::setSeed(int _seed) {
    seed = _seed;
    MathUtils::rnd->seed(seed);
}

void ProstPlanner::initSession(int _numberOfRounds, long /*totalTime*/) {
    Logger::logSeparator(Verbosity::VERBOSE);
    Logger::logLine("Final task: ", Verbosity::VERBOSE);
    SearchEngine::printTask();

    currentRound = -1;
    numberOfRounds = _numberOfRounds;
    if (tmMethod == UNIFORM) {
        remainingTimeFactor = numberOfRounds * SearchEngine::horizon;
    }

    cout.precision(6);

    searchEngine->initSession();

    if (searchEngine->usesBDDs()) {
        // TODO: These numbers are rather random. Since I know only little on
        //  what they actually mean, it'd be nice to re-adjust these.
        bdd_init(5000000, 20000);

        int* domains = new int[KleeneState::stateSize];
        for (size_t index = 0; index < SearchEngine::allCPFs.size(); ++index) {
            domains[index] = SearchEngine::allCPFs[index]->getDomainSize();
        }
        fdd_extdomain(domains, KleeneState::stateSize);
    }

    printConfig();
}

void ProstPlanner::finishSession(double& totalReward) {
    Logger::logLine("", Verbosity::NORMAL);
    Logger::logSeparator(Verbosity::NORMAL);
    Logger::logLine(">>> END OF SESSION  -- TOTAL REWARD: " +
                    to_string(totalReward), Verbosity::SILENT);

    double avgReward = totalReward / numberOfRounds;
    Logger::logLine(">>> END OF SESSION  -- AVERAGE REWARD: " +
                    to_string(avgReward), Verbosity::SILENT);
}

void ProstPlanner::initRound(long const& remainingTime) {
    ++currentRound;
    currentStep = -1;
    stepsToGo = SearchEngine::horizon + 1;

    Logger::logSeparator(Verbosity::NORMAL);
    Logger::logLine(">>> STARTING ROUND " + to_string(currentRound + 1) +
                    " -- REMAINING TIME " + to_string(remainingTime / 1000) +
                    "s", Verbosity::SILENT);

    // Notify search engine
    searchEngine->initRound();
}

void ProstPlanner::finishRound(double const& roundReward) {
    // Print per round statistics
    Logger::logLine("", Verbosity::NORMAL);
    Logger::logSeparator(Verbosity::NORMAL);
    Logger::logLine(">>> END OF ROUND " + to_string(currentRound + 1) +
                    " -- REWARD RECEIVED: " + to_string(roundReward),
                    Verbosity::SILENT);
    Logger::logSmallSeparator(Verbosity::NORMAL);
    searchEngine->printRoundStatistics("");
    Logger::logLine("", Verbosity::NORMAL);

    // Notify search engine
    searchEngine->finishRound();
}

void ProstPlanner::initStep(vector<double> const& nextStateVec,
                            long const& remainingTime) {
    // Update current step and log it
    ++currentStep;
    --stepsToGo;
    Logger::logSeparator(Verbosity::NORMAL);
    Logger::logLine("Planning step " + to_string(currentStep + 1) + "/" +
                    to_string(SearchEngine::horizon) + " in round " +
                    to_string(currentRound + 1) + "/" +
                    to_string(numberOfRounds), Verbosity::NORMAL);
    Logger::logLine("", Verbosity::VERBOSE);

    // Determine if too much RAM is used and stop caching if this is the case
    monitorRAMUsage();

    // Create current state
    assert(nextStateVec.size() == State::numberOfDeterministicStateFluents +
                                  State::numberOfProbabilisticStateFluents);
    currentState = State(nextStateVec, stepsToGo);
    State::calcStateFluentHashKeys(currentState);
    State::calcStateHashKey(currentState);

    // Log current state
    Logger::logLine("Current state:", Verbosity::VERBOSE);
    Logger::logLineIf(
        currentState.toString(), Verbosity::VERBOSE,
        "Current state: " + currentState.toCompactString(), Verbosity::NORMAL);

    manageTimeouts(remainingTime);

    // Notify search engine
    searchEngine->initStep(currentState);
}

void ProstPlanner::finishStep(double const& immediateReward) {
    int usedRAM = SystemUtils::getRAMUsedByThis();
    Logger::logLine("Used RAM: " + to_string(usedRAM), Verbosity::NORMAL);

    Logger::logLine("", Verbosity::NORMAL);
    searchEngine->printStepStatistics("");

    Logger::logLine(
        "Submitted action: " +
        SearchEngine::actionStates[executedActionIndex].toCompactString(),
        Verbosity::SILENT);
    Logger::logLine("Immediate reward: " + to_string(immediateReward),
                    Verbosity::NORMAL);

    // Notify search engine
    searchEngine->finishStep();
}

vector<string> ProstPlanner::plan() {
    // Call the search engine
    vector<int> bestActions;
    searchEngine->estimateBestActions(currentState, bestActions);

    // Pick one of the recommended actions uniformly at random
    executedActionIndex = MathUtils::rnd->randomElement(bestActions);
    ActionState const& executedAction =
            SearchEngine::actionStates[executedActionIndex];

    // PROST's communication with the environment works with strings, so we
    // collect the names of all true action fluents of the chosen action
    vector<string> result;
    for (ActionFluent const* af : executedAction.scheduledActionFluents) {
        result.push_back(af->name);
    }
    return result;
}

void ProstPlanner::monitorRAMUsage() {
    if (cachingEnabled && (SystemUtils::getRAMUsedByThis() > ramLimit)) {
        cachingEnabled = false;

        SearchEngine::cacheApplicableActions = false;
        for (size_t i = 0; i < State::numberOfDeterministicStateFluents; ++i) {
            SearchEngine::deterministicCPFs[i]->disableCaching();
        }

        for (size_t i = 0; i < State::numberOfProbabilisticStateFluents; ++i) {
            SearchEngine::probabilisticCPFs[i]->disableCaching();
            SearchEngine::determinizedCPFs[i]->disableCaching();
        }

        SearchEngine::rewardCPF->disableCaching();

        for (size_t i = 0; i < SearchEngine::actionPreconditions.size(); ++i) {
            SearchEngine::actionPreconditions[i]->disableCaching();
        }

        searchEngine->disableCaching();
        Logger::logLine(
            "CACHING ABORTED IN STEP " + to_string(currentStep + 1) +
            " OF ROUND " + to_string(currentRound + 1), Verbosity::SILENT);
    }
}

void ProstPlanner::manageTimeouts(long const& remainingTime) {
    double remainingTimeInSeconds = ((double)remainingTime) / 1000.0;
    if (MathUtils::doubleIsGreater(remainingTimeInSeconds, 3.0)) {
        // We use a buffer of 3 seconds
        remainingTimeInSeconds -= 3.0;
    }
    double timeForThisStep = 0.0;

    switch (tmMethod) {
    case NONE:
        return;
        break;
    case UNIFORM:
        timeForThisStep = remainingTimeInSeconds / remainingTimeFactor;
        --remainingTimeFactor;
        break;
    }
    Logger::logLine("Setting time for this decision to " +
                    to_string(timeForThisStep) + "s.", Verbosity::NORMAL);

    searchEngine->setTimeout(timeForThisStep);
}

void ProstPlanner::printConfig() const {
    Logger::logSeparator(Verbosity::VERBOSE);
    Logger::logLine("Configuration of PROST planner:", Verbosity::VERBOSE);
    Logger::logLine(
        "  Random seed: " + std::to_string(seed), Verbosity::VERBOSE);
    Logger::logLine(
        "  RAM limit: " + std::to_string(ramLimit), Verbosity::VERBOSE);
    Logger::logLine(
        "  Bit size: " + std::to_string(bitSize), Verbosity::VERBOSE);

    switch(tmMethod) {
        case UNIFORM:
            Logger::logLine("  Timeout method: UNIFORM", Verbosity::VERBOSE);
            break;
        case NONE:
            Logger::logLine("  Timeout method: NONE", Verbosity::VERBOSE);
            break;
    }
    switch (Logger::runVerbosity) {
        case Verbosity::SUPPRESS:
            SystemUtils::abort("ERROR: Log level must not be SUPPRESS!");
            break;
        case Verbosity::SILENT:
            Logger::logLine("  Log level: SILENT", Verbosity::VERBOSE);
            break;
        case Verbosity::NORMAL:
            Logger::logLine("  Log level: NORMAL", Verbosity::VERBOSE);
            break;
        case Verbosity::VERBOSE:
            Logger::logLine("  Log level: VERBOSE", Verbosity::VERBOSE);
            break;
        case Verbosity::DEBUG:
            Logger::logLine("  Log level: VERBOSE", Verbosity::DEBUG);
            break;
    }

    Logger::logLine("", Verbosity::VERBOSE);
    searchEngine->printConfig("  ");
}

void ProstPlanner::resetStaticMembers() {
    SearchEngine::actionFluents.clear();
    SearchEngine::stateFluents.clear();
    SearchEngine::probabilisticCPFs.clear();
    SearchEngine::allCPFs.clear();
    SearchEngine::determinizedCPFs.clear();
    SearchEngine::deterministicCPFs.clear();
    SearchEngine::actionPreconditions.clear();
    SearchEngine::actionStates.clear();
    SearchEngine::trainingSet.clear();
    SearchEngine::actionPreconditions.clear();
    SearchEngine::candidatesForOptimalFinalAction.clear();
    State::stateFluentHashKeysOfDeterministicStateFluents.clear();
    State::stateFluentHashKeysOfProbabilisticStateFluents.clear();
    State::stateHashKeysOfDeterministicStateFluents.clear();
    State::stateHashKeysOfProbabilisticStateFluents.clear();
    KleeneState::hashKeyBases.clear();
    KleeneState::indexToStateFluentHashKeyMap.clear();
    MathUtils::resetRNG();
}
