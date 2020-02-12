#include "prost_planner.h"

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
            SystemUtils::abort("Unused parameter value pair: " + param + " / " +
                               value);
        }
    }

    if (!searchEngineDefined) {
        SystemUtils::abort(
            "Error: search engine specification is insufficient. Rerun PROST "
            "without parameters to see detailed instructions.");
    }
}

void ProstPlanner::setSeed(int _seed) {
    MathUtils::rnd->seed(_seed);
}

void ProstPlanner::init() {
    Stopwatch time;
    Logger::logLine("learning...");

    cout.precision(6);

    searchEngine->learn();

    if (searchEngine->usesBDDs()) {
        // TODO: These numbers are rather random. Since I know only little on
        // what they actually mean, it'd be nice to re-adjust these.
        bdd_init(5000000, 20000);

        int* domains = new int[KleeneState::stateSize];
        for (size_t index = 0; index < SearchEngine::allCPFs.size(); ++index) {
            domains[index] = SearchEngine::allCPFs[index]->getDomainSize();
        }
        fdd_extdomain(domains, KleeneState::stateSize);
    }
    Logger::logLine("...finished (" + to_string(time()) + ").");
    Logger::logLine();

    Logger::logLine("Final task: ");
    SearchEngine::printTask();

    cout.precision(6);
}

vector<string> ProstPlanner::plan() {
    // Call the search engine
    vector<int> bestActions;
    searchEngine->estimateBestActions(currentState, bestActions);
    chosenActionIndices[currentRound][currentStep] =
        MathUtils::rnd->randomElement(bestActions);

    // PROST's communication with the environment works with strings, so we
    // collect the names of all true action fluents of the chosen action
    int& chosenActionIndex = chosenActionIndices[currentRound][currentStep];
    vector<string> result;
    for (size_t i = 0; i < SearchEngine::actionStates[chosenActionIndex]
                               .scheduledActionFluents.size();
         ++i) {
        result.push_back(SearchEngine::actionStates[chosenActionIndex]
                             .scheduledActionFluents[i]
                             ->name);
    }

    // assert(false);
    // SystemUtils::abort("");
    return result;
}

void ProstPlanner::initSession(int _numberOfRounds, long /*totalTime*/) {
    currentRound = -1;
    numberOfRounds = _numberOfRounds;
    immediateRewards = vector<vector<double>>(
        numberOfRounds, vector<double>(SearchEngine::horizon, 0.0));
    chosenActionIndices = vector<vector<int>>(
        numberOfRounds, vector<int>(SearchEngine::horizon, -1));

    switch (tmMethod) {
    case NONE:
        break;
    case UNIFORM:
        remainingTimeFactor = numberOfRounds * SearchEngine::horizon;
        break;
    }
}

void ProstPlanner::finishSession(double& totalReward) {
    Logger::logLine("***********************************************",
                    Verbosity::SILENT);
    Logger::logLine("Immediate rewards:");
    for (size_t i = 0; i < immediateRewards.size(); ++i) {
        double rewardSum = 0.0;
        Logger::log("Round " + to_string(i) + ": ", Verbosity::SILENT);
        for (double rew : immediateRewards[i]) {
            Logger::log(to_string(rew) + " ", Verbosity::SILENT);
            rewardSum += rew;
        }
        Logger::logLine(" = " + to_string(rewardSum), Verbosity::SILENT);
    }
    Logger::logLine("", Verbosity::SILENT);

    double avgReward = totalReward / numberOfRounds;

    Logger::logLine(">>>           TOTAL REWARD: " + to_string(totalReward),
                    Verbosity::SILENT);
    Logger::logLine(">>>          AVERAGE REWARD: " + to_string(avgReward),
                    Verbosity::SILENT);

    Logger::logLine("***********************************************",
                    Verbosity::SILENT);
}

void ProstPlanner::initRound(long const& remainingTime) {
    ++currentRound;
    currentStep = -1;
    stepsToGo = SearchEngine::horizon + 1;

    Logger::logLine("***********************************************",
                    Verbosity::SILENT);
    Logger::logLine(">>> STARTING ROUND " + to_string(currentRound + 1) +
                    " -- REMAINING TIME " + to_string(remainingTime / 1000) +
                    "s", Verbosity::SILENT);
    Logger::logLine("***********************************************",
                    Verbosity::SILENT);
}

void ProstPlanner::finishRound(double const& roundReward) {
    Logger::logLine("***********************************************",
                    Verbosity::SILENT);
    Logger::logLine(">>> END OF ROUND " + to_string(currentRound + 1) +
                    " -- REWARD RECEIVED: " + to_string(roundReward),
                    Verbosity::SILENT);
    Logger::logLine("***********************************************",
                    Verbosity::SILENT);
}

void ProstPlanner::initStep(vector<double> const& nextStateVec,
                            long const& remainingTime) {
    ++currentStep;
    --stepsToGo;

    Logger::logLine("***********************************************",
                    Verbosity::NORMAL);
    Logger::logLine("Planning step " + to_string(currentStep + 1) + "/" +
                    to_string(SearchEngine::horizon) + " in round " +
                    to_string(currentRound + 1) + "/" +
                    to_string(numberOfRounds), Verbosity::NORMAL);

    monitorRAMUsage();

    assert(nextStateVec.size() == State::numberOfDeterministicStateFluents +
                                      State::numberOfProbabilisticStateFluents);

    currentState = State(nextStateVec, stepsToGo);
    State::calcStateFluentHashKeys(currentState);
    State::calcStateHashKey(currentState);

    if (Logger::runVerbosity >= Verbosity::VERBOSE) {
        Logger::logLine("Current state: " + currentState.toString());
    } else {
        Logger::logLine("Current state: " + currentState.toCompactString(),
                        Verbosity::NORMAL);
    }

    manageTimeouts(remainingTime);
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

void ProstPlanner::finishStep(double const& immediateReward) {
    assert(currentRound < immediateRewards.size());
    assert(currentStep < immediateRewards[currentRound].size());

    immediateRewards[currentRound][currentStep] = immediateReward;

    int usedRAM = SystemUtils::getRAMUsedByThis();
    int bucketsProbStateValue =
        ProbabilisticSearchEngine::stateValueCache.bucket_count();
    int bucketsDetStateValue =
        DeterministicSearchEngine::stateValueCache.bucket_count();
    int bucketsProbApplActions =
        ProbabilisticSearchEngine::applicableActionsCache.bucket_count();
    int bucketsDetApplActions =
        DeterministicSearchEngine::applicableActionsCache.bucket_count();
    // int bucketsMLSRewardCache =
    //     MinimalLookaheadSearch::rewardCache.bucket_count();
    // int bucketsIDSRewardCache =
    //          IDS::rewardCache.bucket_count();

    Logger::logLine("Used RAM: " + to_string(usedRAM), Verbosity::NORMAL);
    Logger::logLine("Buckets in probabilistic state value cache: " +
                    to_string(bucketsProbStateValue), Verbosity::DEBUG);
    Logger::logLine("Buckets in deterministic state value cache: " +
                    to_string(bucketsDetStateValue), Verbosity::DEBUG);
    Logger::logLine("Buckets in probabilistic applicable actions cache: " +
                    to_string(bucketsProbApplActions), Verbosity::DEBUG);
    Logger::logLine("Buckets in deterministic applicable actions cache: " +
                    to_string(bucketsDetApplActions), Verbosity::DEBUG);
    // Logger::logLine("Buckets in MLS reward cache: " +
    //                 to_string(bucketsMLSRewardCache), Verbosity::DEBUG);
    // Logger::logLine("Buckets in IDS reward cache: " +
    //                 to_string(bucketsIDSRewardCache), Verbosity::DEBUG);

    int submittedActionIndex = chosenActionIndices[currentRound][currentStep];
    Logger::logLine(
        "Submitted action: " +
        SearchEngine::actionStates[submittedActionIndex].toCompactString(),
        Verbosity::SILENT);
    Logger::logLine("Immediate reward: " + to_string(immediateReward),
                    Verbosity::NORMAL);
    Logger::logLine("***********************************************",
                    Verbosity::NORMAL);
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
