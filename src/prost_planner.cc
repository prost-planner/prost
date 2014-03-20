#include "prost_planner.h"

#include "search_engine.h"
#include "state_set_generator.h"
#include "planning_task.h"

#include "utils/timer.h"
#include "utils/math_utils.h"
#include "utils/system_utils.h"
#include "utils/string_utils.h"

#include <iostream>

using namespace std;

ProstPlanner::ProstPlanner(string& plannerDesc, PlanningTask* _task) :
    task(_task),
    searchEngine(NULL),
    currentState(task->getInitialState()),
    currentRound(-1), 
    remainingSteps(task->getHorizon()),
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
            searchEngineDesc = value;
            searchEngineDefined = true;
        } else {
            SystemUtils::abort("Unused parameter value pair: " + param + " / " + value);
        }
    }

    if(!searchEngineDefined) {
        SystemUtils::abort("A Search Engine must be specified");
    }
}

void ProstPlanner::init() {
    setSearchEngine(SearchEngine::fromString(searchEngineDesc, this));

    Timer t;
    cout << "generating training set..." << endl;
    StateSetGenerator* gen = new StateSetGenerator(task);
    std::vector<State> trainingSet = gen->generateStateSet(currentState);
    delete gen;

    // We set the seed again if the training set is smaller than 200 because we
    // terminated because of the state set generator timeout in that case, making
    // everything following this nondeterministic! TODO: Make the 200 a parameter
    if(trainingSet.size() != 200) {
        setSeed(seed);
    }
    cout << "...finished (" << t << ")." << endl;

    t.reset();
    cout << "learning on training set..." << endl;

    int lastSize = learningComponents.size();
    task->learn(trainingSet);
    while(!learningComponents.empty()) {
        for(unsigned int i = 0; i < learningComponents.size(); ++i) {
            if(learningComponents[i]->learn(trainingSet)) {
                swap(learningComponents[i], learningComponents[learningComponents.size()-1]);
                learningComponents.pop_back();
                --i;
            }
        }
        if(!learningComponents.empty()) {
            if(learningComponents.size() == lastSize) {
                SystemUtils::abort("Some component(s) registered as LearningComponent but cannot learn!");
            }
            lastSize = learningComponents.size();
        }
    }

    assert(learningComponents.empty());
    cout << "...finished (" << t << ")." << endl << endl;

    cout << "Final task: " << endl;
    task->print(cout);
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
    for(unsigned int i = 0; i < task->actionState(chosenActionIndex).scheduledActionFluents.size(); ++i) {
        result.push_back(task->actionState(chosenActionIndex).scheduledActionFluents[i]->fullName);
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
        for(unsigned int i = 0; i < cachingComponents.size(); ++i) {
            cachingComponents[i]->disableCaching();
        }
        cout << endl << "caching aborted in " << cachingComponents.size() << " components." << endl << endl;
    }
}

void ProstPlanner::initNextStep(vector<double> const& nextStateVec) {
    currentState = task->getState(nextStateVec, remainingSteps);
    bestActions.clear();
}

void ProstPlanner::initNextRound() {
    ++currentRound;
    remainingSteps = task->getHorizon();
}

int ProstPlanner::combineResults() {
    return bestActions[rand() % bestActions.size()];
}


void ProstPlanner::unregisterCachingComponent(CachingComponent* component) {
    cout << "unregistering caching component" << endl;
    for(unsigned int i = 0; i < cachingComponents.size(); ++i) {
        if(cachingComponents[i] == component) {
            std::swap(cachingComponents[i],cachingComponents[cachingComponents.size()-1]);
            cachingComponents.pop_back();
            --i;
        }
    }
}

void ProstPlanner::unregisterLearningComponent(LearningComponent* component) {
    cout << "unregistering learning component" << endl;
    for(unsigned int i = 0; i < learningComponents.size(); ++i) {
        if(learningComponents[i] == component) {
            std::swap(learningComponents[i], learningComponents[learningComponents.size()-1]);
            learningComponents.pop_back();
            --i;
        }
    }
}

void ProstPlanner::setSeed(int _seed) {
    seed = _seed;
    srand(seed);
}

void ProstPlanner::printStep(int result, bool printSearchEngineLogs) {
    cout << "------------------------------------------------------------------------------------------" << endl;
    cout << "Planning step " << (task->getHorizon() - remainingSteps + 1)
         << "/" << task->getHorizon() << " in round " << (currentRound+1)
         << "/" << numberOfRounds << " with state:" << endl << endl;
    task->printState(cout, currentState);
    cout << endl;

    if(printSearchEngineLogs) {
        searchEngine->print(cout);
        cout << "------------------------------------------------------------------" << endl << endl;
    }

    cout << "Used RAM: " << SystemUtils::getRAMUsedByThis() << endl;
    cout << "Sumitting Action: ";
    task->printAction(cout, result);
    cout << endl << "------------------------------------------------------------------" << endl << endl;
    
    // cout << "deadend bdd:" << endl;
    // task->printDeadEndBDD();

    // cout << "goal bdd:" << endl;
    // task->printGoalBDD();
}


