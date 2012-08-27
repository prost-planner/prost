#include "prost_planner.h"

#include "rddl_parser.h"
#include "instantiator.h"
#include "preprocessor.h"
#include "search_engine.h"
#include "state_set_generator.h"
#include "actions.h"
#include "conditional_probability_functions.h"
#include "caching_component.h"

#include "utils/timer.h"
#include "utils/math_utils.h"
#include "utils/system_utils.h"
#include "utils/string_utils.h"

#include <iostream>

using namespace std;

ProstPlanner::ProstPlanner(string domain, string problem, int _numberOfRounds) :
    unprocessedTask(NULL),
    probabilisticTask(NULL),
    deterministicTask(NULL),
    searchEngine(NULL),
    currentState(),
    currentRound(-1), 
    remainingSteps(-1),
    numberOfRounds(_numberOfRounds),
    cachingEnabled(true),
    ramLimit(2621440),
    bitSize(sizeof(long)*8),
    searchEngineResult() {

    setSeed((int)time(NULL));
    unprocessedTask = new UnprocessedPlanningTask(domain, problem);
}

ProstPlanner* ProstPlanner::fromString(string& desc, string& domain, string& problem, int& numberOfRounds) {
    StringUtils::trim(desc);
    assert(desc[0] == '[' && desc[desc.size()-1] == ']');
    StringUtils::removeFirstAndLastCharacter(desc);
    StringUtils::trim(desc);

    assert(desc.find("PROST") == 0);
    desc = desc.substr(5,desc.size());

    bool searchEngineDefined = false;

    ProstPlanner* result = new ProstPlanner(domain, problem, numberOfRounds);

    while(!desc.empty()) {
        string param;
        string value;
        StringUtils::nextParamValuePair(desc,param,value);

        if(param == "-s") {
            result->setSeed(atoi(value.c_str()));
        } else if(param == "-ram") {
            result->setRAMLimit(atoi(value.c_str()));
        } else if(param == "-bit") {
            result->setBitSize(atoi(value.c_str()));
        } else if(param == "-se") {
            result->searchEngineDesc = value;
            searchEngineDefined = true;
        } else {
            SystemUtils::abort("Unused parameter value pair: " + param + " / " + value);
        }
    }

    assert(searchEngineDefined);

    return result;
}

void ProstPlanner::init(map<string,int>& stateVariableIndices) {
    Timer t;
    cout << "parsing..." << endl;
    RDDLParser parser(unprocessedTask);
    parser.parse();
    cout << "...finished (" << t << ")." << endl;

    t.reset();
    cout << "instantiating..." << endl;
    probabilisticTask = new PlanningTask(this);
    Instantiator instantiator(this, unprocessedTask, probabilisticTask);
    instantiator.instantiate();
    cout << "...finished (" << t << ")." << endl;

    t.reset();
    cout << "preprocessing..." << endl;
    Preprocessor preprocessor(this, unprocessedTask, probabilisticTask);
    deterministicTask = preprocessor.preprocess(stateVariableIndices);

    remainingSteps = probabilisticTask->getHorizon();
    currentState = State(probabilisticTask->getInitialState());
    searchEngineResult = vector<double>(probabilisticTask->getNumberOfActions(),0);
    cout << "...finished (" << t << ")." << endl;

    t.reset();
    cout << "generating training set..." << endl;
    StateSetGenerator* gen = new StateSetGenerator(this);
    gen->run(currentState);
    cout << "...finished (" << t << ")." << endl;

    t.reset();
    cout << "learning on training set..." << endl;

    setSearchEngine(SearchEngine::fromString(searchEngineDesc, this, searchEngineResult));

    std::vector<State> trainingSet = gen->getGeneratedStateSet();
    for(unsigned int i = 0; i < learningComponents.size(); ++i) {
        learningComponents[i]->learn(trainingSet);
    }
    cout << "...finished (" << t << ")." << endl << endl;

    cout << "Preprocessed tasks:" << endl;
    probabilisticTask->print();
    deterministicTask->print();
}

vector<string> ProstPlanner::plan(vector<double> const& nextStateVec) {
    manageTimeouts();

    initNextStep(nextStateVec);
    searchEngine->run(currentState);

    int chosenActionIndex = combineResults();
    printStep(chosenActionIndex);
    monitorRAMUsage();

    --remainingSteps;
    vector<string> result;
    probabilisticTask->actionState(chosenActionIndex).getActions(result);

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
    currentState = State(nextStateVec, remainingSteps);
    probabilisticTask->calcStateFluentHashKeys(currentState);
    probabilisticTask->calcStateHashKey(currentState);
    
    for(unsigned int i = 0; i < searchEngineResult.size(); ++i) {
        searchEngineResult[i] = 1.0;
    }
}

void ProstPlanner::initNextRound() {
    ++currentRound;
    remainingSteps = probabilisticTask->getHorizon();
}

int ProstPlanner::combineResults() {
    vector<int> bestIndices;
    double bestRes = -numeric_limits<double>::max();
    for(unsigned int i = 0; i < searchEngineResult.size(); ++i) {
        if(MathUtils::doubleIsGreater(searchEngineResult[i], bestRes)) {
            bestIndices.clear();
            bestIndices.push_back(i);
            bestRes = searchEngineResult[i];
        } else if(MathUtils::doubleIsEqual(searchEngineResult[i], bestRes)) {
            bestIndices.push_back(i);
        }
    }

    /*
    cout << "best actions:" << endl;
    for(unsigned int i = 0; i <bestIndices.size(); ++i) {
        probabilisticTask->actions[bestIndices[i]]->printLeaf();
        cout << endl;
    }
    cout << "-----";
    */

    return bestIndices[rand() % bestIndices.size()];
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

void ProstPlanner::setSeed(int seed) {
    srand(seed);
}

void ProstPlanner::printStep(int result, bool printSearchEngineLogs, bool printSearchEngineResult) {
    cout << "------------------------------------------------------------------------------------------" << endl;
    cout << "Planning step " << (probabilisticTask->getHorizon() - remainingSteps + 1)
         << "/" << probabilisticTask->getHorizon() << " in round " << (currentRound+1)
         << "/" << numberOfRounds << " with state:" << endl << endl;
    probabilisticTask->printState(currentState);
    cout << endl;

    if(printSearchEngineLogs) {
        searchEngine->print();
        cout << "------------------------------------------------------------------" << endl << endl;
    }

    if(printSearchEngineResult) {
        for(unsigned int i = 0; i < searchEngineResult.size(); ++i) {
            if(!MathUtils::doubleIsMinusInfinity(searchEngineResult[i])) {
                probabilisticTask->printAction(i);
                cout << " : " << searchEngineResult[i] << endl;
            }
        }
        cout << "------------------------------------------------------------------" << endl << endl;
    }
    cout << "Used RAM: " << SystemUtils::getRAMUsedByThis() << endl;
    cout << "Sumitting Action: ";
    probabilisticTask->printAction(result);
    cout << endl << "------------------------------------------------------------------" << endl << endl;

    /*
    cout << "prob dead lock bdd:" << endl;
    bdd_printdot(probabilisticTask->cachedDeadLocks);

    cout << "prob goal bdd:" << endl;
    bdd_printdot(probabilisticTask->cachedGoals);

    cout << "det dead lock bdd:" << endl;
    bdd_printdot(deterministicTask->cachedDeadLocks);

    cout << "det goal bdd:" << endl;
    bdd_printdot(deterministicTask->cachedGoals);
    */
}


