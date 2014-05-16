#include "prost_planner.h"

#include "search_engine.h"

#include "utils/timer.h"
#include "utils/math_utils.h"
#include "utils/system_utils.h"
#include "utils/string_utils.h"

#include <iostream>

using namespace std;

ProstPlanner::ProstPlanner(string& plannerDesc) :
    searchEngine(NULL),
    currentState(SearchEngine::initialState),
    currentRound(-1), 
    remainingSteps(SearchEngine::horizon),
    numberOfRounds(-1),
    cachingEnabled(true),
    ramLimit(2621440),
    bitSize(sizeof(long)*8) {

    setSeed((int)time(NULL));
    
    StringUtils::trim(plannerDesc);
    assert(plannerDesc[0] == '[' && plannerDesc[plannerDesc.size()-1] == ']');
    StringUtils::removeFirstAndLastCharacter(plannerDesc);
    StringUtils::trim(plannerDesc);

    assert(plannerDesc.find("PROST") == 0);
    plannerDesc = plannerDesc.substr(5, plannerDesc.size());

    bool searchEngineDefined = false;

    while(!plannerDesc.empty()) {
        string param;
        string value;
        StringUtils::nextParamValuePair(plannerDesc,param,value);

        if(param == "-s") {
            setSeed(atoi(value.c_str()));
        } else if(param == "-ram") {
            setRAMLimit(atoi(value.c_str()));
        } else if(param == "-bit") {
            setBitSize(atoi(value.c_str()));
        } else if(param == "-se") {
            setSearchEngine(SearchEngine::fromString(value));
            searchEngineDefined = true;
        } else {
            SystemUtils::abort("Unused parameter value pair: " + param + " / " + value);
        }
    }

    if(!searchEngineDefined) {
        SystemUtils::abort("Error: search engine specification is insufficient. Rerun PROST without parameters to see detailed instructions.");
    }
}

void ProstPlanner::init() {
    Timer t;
    cout << "learning..." << endl;
    searchEngine->learn();

    if(ProbabilisticSearchEngine::useRewardLockDetection && ProbabilisticSearchEngine::useBDDCaching) {
        // TODO: These numbers are rather random. Since I know only little on
        // what they actually mean, it'd be nice to re-adjust these.
        bdd_init(5000000,20000);

        int* domains = new int[KleeneState::stateSize];
        for(unsigned int index = 0; index < SearchEngine::allCPFs.size(); ++index) {
            domains[index] = SearchEngine::allCPFs[index]->getDomainSize();
        }
        fdd_extdomain(domains, KleeneState::stateSize);
    }
    cout << "...finished (" << t << ")." << endl << endl;

    cout << "Final task: " << endl;
    SearchEngine::printTask(cout);
}

vector<string> ProstPlanner::plan(vector<double> const& nextStateVec) {
    manageTimeouts();
    initNextStep(nextStateVec);

    searchEngine->estimateBestActions(currentState, bestActions);
    int chosenActionIndex = combineResults();

    printStep(chosenActionIndex);
    monitorRAMUsage();

    --remainingSteps;

    // PROST's communication with the environment works with strings, so we
    // collect the names of all true action fluents of the chosen action
    vector<string> result;
    for(unsigned int i = 0; i < SearchEngine::actionStates[chosenActionIndex].scheduledActionFluents.size(); ++i) {
        result.push_back(SearchEngine::actionStates[chosenActionIndex].scheduledActionFluents[i]->name);
    }

    // assert(false);
    // SystemUtils::abort("");
    return result;
}

void ProstPlanner::manageTimeouts() {
    //no timeout managment implemented currently
}

void ProstPlanner::monitorRAMUsage() {
    if(cachingEnabled && (SystemUtils::getRAMUsedByThis() > ramLimit)) {
        cachingEnabled = false;

        SearchEngine::cacheApplicableActions = false;
        for(unsigned int i = 0; i < State::numberOfDeterministicStateFluents; ++i) {
            SearchEngine::deterministicCPFs[i]->disableCaching();
        }

        for(unsigned int i = 0; i < State::numberOfProbabilisticStateFluents; ++i) {
            SearchEngine::probabilisticCPFs[i]->disableCaching();
            SearchEngine::determinizedCPFs[i]->disableCaching();
        }

        SearchEngine::rewardCPF->disableCaching();

        for(unsigned int i = 0; i < SearchEngine::actionPreconditions.size(); ++i) {
            SearchEngine::actionPreconditions[i]->disableCaching();
        }
        
        searchEngine->disableCaching();
        cout << endl << "CACHING ABORTED IN STEP " << (SearchEngine::horizon - remainingSteps + 1) << " OF ROUND " << (currentRound+1) << endl << endl;
    }
}

void ProstPlanner::initNextStep(vector<double> const& nextStateVec) {
    assert(nextStateVec.size() == State::numberOfDeterministicStateFluents + State::numberOfProbabilisticStateFluents);

    vector<double> nextValuesOfDeterministicStateFluents;
    for(unsigned int i = 0; i < State::numberOfDeterministicStateFluents; ++i) {
        nextValuesOfDeterministicStateFluents.push_back(nextStateVec[i]);
    }
    vector<double> nextValuesOfProbabilisticStateFluents;
    for(unsigned int i = 0; i < State::numberOfProbabilisticStateFluents; ++i) {
        nextValuesOfProbabilisticStateFluents.push_back(nextStateVec[i+State::numberOfDeterministicStateFluents]);
    }
    currentState = State(nextValuesOfDeterministicStateFluents, nextValuesOfProbabilisticStateFluents, remainingSteps);
    State::calcStateFluentHashKeys(currentState);
    State::calcStateHashKey(currentState);

    bestActions.clear();
}

void ProstPlanner::initNextRound() {
    ++currentRound;
    remainingSteps = SearchEngine::horizon;
}

int ProstPlanner::combineResults() {
    return bestActions[rand() % bestActions.size()];
}

void ProstPlanner::setSeed(int _seed) {
    seed = _seed;
    srand(seed);
}

void ProstPlanner::printStep(int result, bool printSearchEngineLogs) {
    cout << "------------------------------------------------------------------------------------------" << endl;
    cout << "Planning step " << (SearchEngine::horizon - remainingSteps + 1)
         << "/" << SearchEngine::horizon << " in round " << (currentRound+1)
         << "/" << numberOfRounds << " with state:" << endl;
    currentState.printCompact(cout);
    cout << endl;

    if(printSearchEngineLogs) {
        searchEngine->print(cout);
        cout << "------------------------------------------------------------------" << endl << endl;
    }

    cout << "Used RAM: " << SystemUtils::getRAMUsedByThis() << endl;
    cout << "Submitting Action: ";
    SearchEngine::actionStates[result].printCompact(cout);
    cout << endl << "------------------------------------------------------------------" << endl << endl;
    
    // cout << "deadend bdd:" << endl;
    // SearchEngine::printDeadEndBDD();

    // cout << "goal bdd:" << endl;
    // SearchEngine::printGoalBDD();
}


