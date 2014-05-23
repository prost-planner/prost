#include "prost_planner.h"

#include "search_engine.h"

//#include "iterative_deepening_search.h"
//#include "minimal_lookahead_search.h"

#include "utils/timer.h"
#include "utils/math_utils.h"
#include "utils/system_utils.h"
#include "utils/string_utils.h"

#include <iostream>

using namespace std;

int const FIRST_TIME_FACTOR[40] =  { 20, 12, 10,  8,  7,  6,  5,  5,  5,  5,
                                      5,  5,  5,  5,  5,  5,  4,  4,  4,  4,
                                      4,  4,  4,  4,  4,  3,  3,  3,  3,  3,
                                      3,  3,  2,  2,  2,  2,  2,  1,  1,  0 };

int const SECOND_TIME_FACTOR[40] = { 12, 10,  9,  8,  7,  6,  5,  5,  5,  5,
                                      5,  5,  5,  5,  5,  5,  4,  4,  4,  4,
                                      4,  4,  4,  4,  4,  3,  3,  3,  3,  3,
                                      3,  3,  2,  2,  2,  2,  2,  1,  1,  0 };

int const THIRD_TIME_FACTOR[40] =  { 10,  9,  8,  7,  6,  5,  5,  5,  5,  5,
                                      5,  5,  5,  5,  5,  5,  4,  4,  4,  4,
                                      4,  4,  4,  4,  4,  3,  3,  3,  3,  3,
                                      3,  3,  2,  2,  2,  2,  2,  1,  1,  0 };

int const FOURTH_TIME_FACTOR[40] = { 10,  8,  7,  6,  6,  5,  5,  5,  5,  5,
                                      5,  5,  5,  5,  5,  5,  4,  4,  4,  4,
                                      4,  4,  4,  4,  4,  3,  3,  3,  3,  3,
                                      3,  3,  2,  2,  2,  2,  2,  1,  1,  0 };

int const FIFTH_TIME_FACTOR[40] =   { 8,  7,  6,  6,  6,  5,  5,  5,  5,  5,
                                      5,  5,  5,  5,  5,  5,  4,  4,  4,  4,
                                      4,  4,  4,  4,  4,  3,  3,  3,  3,  3,
                                      3,  3,  2,  2,  2,  2,  2,  1,  1,  0 };

int const DEFAULT_TIME_FACTOR[40] = { 7,  6,  6,  6,  6,  5,  5,  5,  5,  5,
                                      5,  5,  5,  5,  5,  5,  4,  4,  4,  4,
                                      4,  4,  4,  4,  4,  3,  3,  3,  3,  3,
                                      3,  3,  2,  2,  2,  2,  2,  1,  1,  0 };

int const LAST_TIME_FACTOR[40] =    { 7,  6,  6,  6,  6,  5,  5,  5,  5,  5,
                                      5,  5,  5,  5,  5,  5,  4,  4,  4,  4,
                                      4,  4,  4,  4,  4,  3,  3,  3,  3,  3,
                                      3,  3,  2,  2,  2,  2,  2,  1,  1,  1 };

int const FIRST_TIME_FACTOR_SUM = 182;
int const SECOND_TIME_FACTOR_SUM = 171;
int const THIRD_TIME_FACTOR_SUM = 164;
int const FOURTH_TIME_FACTOR_SUM = 161;
int const FIFTH_TIME_FACTOR_SUM = 157;
int const DEFAULT_TIME_FACTOR_SUM = 155;
int const LAST_TIME_FACTOR_SUM = 156;

ProstPlanner::ProstPlanner(string& plannerDesc) :
    searchEngine(NULL),
    currentState(SearchEngine::initialState),
    currentRound(-1),
    currentStep(-1),
    remainingSteps(SearchEngine::horizon),
    numberOfRounds(-1),
    cachingEnabled(true),
    ramLimit(4194304),
    bitSize(sizeof(long) * 8) {
    setSeed((int) time(NULL));

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
        } else if (param == "-se") {
            setSearchEngine(SearchEngine::fromString(value));
            searchEngineDefined = true;
        } else {
            SystemUtils::abort(
                    "Unused parameter value pair: " + param + " / " + value);
        }
    }

    if (!searchEngineDefined) {
        SystemUtils::abort(
                "Error: search engine specification is insufficient. Rerun PROST without parameters to see detailed instructions.");
    }
}

void ProstPlanner::setSeed(int _seed) {
    seed = _seed;
    srand(seed);
}

void ProstPlanner::init() {
    Timer t;
    cout << "learning..." << endl;
    searchEngine->learn();

    if (ProbabilisticSearchEngine::useRewardLockDetection &&
        ProbabilisticSearchEngine::useBDDCaching) {
        // TODO: These numbers are rather random. Since I know only little on
        // what they actually mean, it'd be nice to re-adjust these.
        bdd_init(5000000, 20000);

        int* domains = new int[KleeneState::stateSize];
        for (size_t index = 0; index < SearchEngine::allCPFs.size(); ++index) {
            domains[index] = SearchEngine::allCPFs[index]->getDomainSize();
        }
        fdd_extdomain(domains, KleeneState::stateSize);
    }
    cout << "...finished (" << t << ")." << endl << endl;

    cout << "Final task: " << endl;
    SearchEngine::printTask(cout);
}

vector<string> ProstPlanner::plan() {
    // Call the search engine
    vector<int> bestActions;
    searchEngine->estimateBestActions(currentState, bestActions);
    chosenActionIndices[currentRound][currentStep] =
        bestActions[rand() % bestActions.size()];

    // PROST's communication with the environment works with strings, so we
    // collect the names of all true action fluents of the chosen action
    int& chosenActionIndex = chosenActionIndices[currentRound][currentStep];
    vector<string> result;
    for (size_t i = 0; i < SearchEngine::actionStates[chosenActionIndex].scheduledActionFluents.size(); ++i) {
        result.push_back(SearchEngine::actionStates[chosenActionIndex].scheduledActionFluents[i]->name);
    }

    // assert(false);
    // SystemUtils::abort("");
    return result;
}

void ProstPlanner::initSession(int  _numberOfRounds, long /*totalTime*/) {
    currentRound = -1;
    numberOfRounds = _numberOfRounds;
    immediateRewards = vector<vector<double> >(numberOfRounds, vector<double>(SearchEngine::horizon, 0.0));
    chosenActionIndices = vector<vector<int> >(numberOfRounds, vector<int>(SearchEngine::horizon, -1));

    remainingTimeFactor = FIRST_TIME_FACTOR_SUM;
    if(numberOfRounds >= 2) {
        remainingTimeFactor += SECOND_TIME_FACTOR_SUM;
    }
    if(numberOfRounds >= 3) {
        remainingTimeFactor += THIRD_TIME_FACTOR_SUM;
    }
    if(numberOfRounds >= 4) {
        remainingTimeFactor += FOURTH_TIME_FACTOR_SUM;
    }
    if(numberOfRounds >= 5) {
        remainingTimeFactor += FIFTH_TIME_FACTOR_SUM;
    }
    if(numberOfRounds >= 6) {
        remainingTimeFactor += LAST_TIME_FACTOR_SUM;
    }
    if(numberOfRounds >= 6) {
        remainingTimeFactor += ((numberOfRounds - 6) * DEFAULT_TIME_FACTOR_SUM);
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

    double avgReward = totalReward / (double) numberOfRounds;

    cout << ">>>           TOTAL REWARD: " << totalReward << endl
         << ">>>          AVERAGE REWARD: " << avgReward << endl
         << "***********************************************" << endl;
}

void ProstPlanner::initRound(long const& remainingTime) {
    ++currentRound;
    currentStep = -1;
    remainingSteps = SearchEngine::horizon + 1;

    cout << "***********************************************" << endl
         << ">>> STARTING ROUND " << (currentRound + 1)
         << " -- REMAINING TIME " << (remainingTime / 1000) << "s" << endl
         << "***********************************************" << endl;
}

void ProstPlanner::finishRound(double const& roundReward) {
    cout << "***********************************************" << endl
         << ">>> END OF ROUND " << (currentRound + 1)
         << " -- REWARD RECEIVED: " << roundReward << endl
         << "***********************************************\n" << endl;
}

void ProstPlanner::initStep(vector<double> const& nextStateVec, long const& remainingTime) {
    ++currentStep;
    --remainingSteps;

    cout << "***********************************************"
         << endl << "Planning step " << (currentStep + 1) << "/" 
         << SearchEngine::horizon << " in round " << (currentRound + 1)
         << "/" << numberOfRounds << endl;

    monitorRAMUsage();

    assert(nextStateVec.size() == State::numberOfDeterministicStateFluents + State::numberOfProbabilisticStateFluents);

    currentState = State(nextStateVec, remainingSteps);
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
        cout << endl << "CACHING ABORTED IN STEP " << (currentStep + 1)
             << " OF ROUND " << (currentRound + 1) << endl << endl;
    }
}

void ProstPlanner::manageTimeouts(long const& remainingTime) {
    double timeFactor = 0.0;
    if(currentRound == 0) {
        timeFactor = (double)FIRST_TIME_FACTOR[currentStep];
    } else if(currentRound == 1) {
        timeFactor = (double)SECOND_TIME_FACTOR[currentStep];
    } else if(currentRound == 2) {
        timeFactor = (double)THIRD_TIME_FACTOR[currentStep];
    } else if(currentRound == 3) {
        timeFactor = (double)FOURTH_TIME_FACTOR[currentStep];
    } else if(currentRound == 4) {
        timeFactor = (double)FIFTH_TIME_FACTOR[currentStep];
    } else if(currentRound == (numberOfRounds - 1)) {
        timeFactor = (double)LAST_TIME_FACTOR[currentStep];
    } else {
        timeFactor = (double)DEFAULT_TIME_FACTOR[currentStep];
    }
    timeFactor /= ((double)remainingTimeFactor);
    double timeForStep = remainingTime * timeFactor / 1000.0;
    cout << "Setting time for this decision to " << timeForStep << "s." << endl;
    searchEngine->setTimeout(timeForStep);

    remainingTimeFactor -= timeFactor;
}

void ProstPlanner::finishStep(double const& immediateReward) {
    assert(currentRound < immediateRewards.size());
    assert(currentStep < immediateRewards[currentRound].size());

    immediateRewards[currentRound][currentStep] = immediateReward;

    searchEngine->print(cout);

    cout << "Used RAM: " << SystemUtils::getRAMUsedByThis() << endl;
    // cout << "Buckets in probabilistic state value cache: " << ProbabilisticSearchEngine::stateValueCache.bucket_count() << endl;
    // cout << "Buckets in deterministic state value cache: " << DeterministicSearchEngine::stateValueCache.bucket_count() << endl;
    // cout << "Buckets in probabilistic applicable actions cache: " << ProbabilisticSearchEngine::applicableActionsCache.bucket_count() << endl;
    // cout << "Buckets in deterministic applicable actions cache: " << DeterministicSearchEngine::applicableActionsCache.bucket_count() << endl;
    // cout << "Buckets in MLS reward cache: " << MinimalLookaheadSearch::rewardCache.bucket_count() << endl;
    // cout << "Buckets in IDS reward cache: " << IDS::rewardCache.bucket_count() << endl;

    int& submittedActionIndex = chosenActionIndices[currentRound][currentStep];
    cout << endl << "Submitted action: ";
    SearchEngine::actionStates[submittedActionIndex].printCompact(cout);
    cout << endl << "Immediate reward: " << immediateReward << endl
         << "***********************************************" << endl << endl;
}
