#include "prost_planner.h"

#include "search_engine.h"

//#include "iterative_deepening_search.h"
//#include "minimal_lookahead_search.h"

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
    cout << "learning..." << endl;

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
    cout << "...finished (" << time << ")." << endl << endl;

    cout << "Final task: " << endl;
    SearchEngine::printTask(cout);

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
    cout << "***********************************************" << endl
         << "Immediate rewards:" << endl;
    for (size_t i = 0; i < immediateRewards.size(); ++i) {
        double rewardSum = 0.0;
        cout << "Round " << i << ": ";
        for (size_t j = 0; j < immediateRewards[i].size(); ++j) {
            cout << immediateRewards[i][j] << " ";
            rewardSum += immediateRewards[i][j];
        }
        cout << " = " << rewardSum << endl;
    }
    cout << endl;

    double avgReward = totalReward / (double)numberOfRounds;

    cout << ">>>           TOTAL REWARD: " << totalReward << endl
         << ">>>          AVERAGE REWARD: " << avgReward << endl
         << "***********************************************" << endl;
}

void ProstPlanner::initRound(long const& remainingTime) {
    ++currentRound;
    currentStep = -1;
    stepsToGo = SearchEngine::horizon + 1;

    cout << "***********************************************" << endl
         << ">>> STARTING ROUND " << (currentRound + 1) << " -- REMAINING TIME "
         << (remainingTime / 1000) << "s" << endl
         << "***********************************************" << endl;
}

void ProstPlanner::finishRound(double const& roundReward) {
    cout << "***********************************************" << endl
         << ">>> END OF ROUND " << (currentRound + 1)
         << " -- REWARD RECEIVED: " << roundReward << endl
         << "***********************************************\n"
         << endl;
}

void ProstPlanner::initStep(vector<double> const& nextStateVec,
                            long const& remainingTime) {
    ++currentStep;
    --stepsToGo;

    cout << "***********************************************" << endl
         << "Planning step " << (currentStep + 1) << "/"
         << SearchEngine::horizon << " in round " << (currentRound + 1) << "/"
         << numberOfRounds << endl;

    monitorRAMUsage();

    assert(nextStateVec.size() ==
           State::numberOfDeterministicStateFluents +
               State::numberOfProbabilisticStateFluents);

    currentState = State(nextStateVec, stepsToGo);
    State::calcStateFluentHashKeys(currentState);
    State::calcStateHashKey(currentState);

    cout << "Current state: ";
    currentState.printCompact(cout);
    cout << endl;

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
        cout << endl
             << "CACHING ABORTED IN STEP " << (currentStep + 1) << " OF ROUND "
             << (currentRound + 1) << endl
             << endl;
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
    cout << "Setting time for this decision to " << timeForThisStep << "s."
         << endl;
    searchEngine->setTimeout(timeForThisStep);
}

void ProstPlanner::finishStep(double const& immediateReward) {
    assert(currentRound < immediateRewards.size());
    assert(currentStep < immediateRewards[currentRound].size());

    immediateRewards[currentRound][currentStep] = immediateReward;

    // searchEngine->print(cout);

    cout << endl << "Used RAM: " << SystemUtils::getRAMUsedByThis() << endl;
    // cout << "Buckets in probabilistic state value cache: " <<
    // ProbabilisticSearchEngine::stateValueCache.bucket_count() << endl;
    // cout << "Buckets in deterministic state value cache: " <<
    // DeterministicSearchEngine::stateValueCache.bucket_count() << endl;
    // cout << "Buckets in probabilistic applicable actions cache: " <<
    // ProbabilisticSearchEngine::applicableActionsCache.bucket_count() << endl;
    // cout << "Buckets in deterministic applicable actions cache: " <<
    // DeterministicSearchEngine::applicableActionsCache.bucket_count() << endl;
    // cout << "Buckets in MLS reward cache: " <<
    // MinimalLookaheadSearch::rewardCache.bucket_count() << endl;
    // cout << "Buckets in IDS reward cache: " <<
    // IDS::rewardCache.bucket_count() << endl;

    int& submittedActionIndex = chosenActionIndices[currentRound][currentStep];
    cout << endl << "Submitted action: ";
    SearchEngine::actionStates[submittedActionIndex].printCompact(cout);
    cout << endl
         << "Immediate reward: " << immediateReward << endl
         << "***********************************************" << endl
         << endl;
}
