#include "uct_search.h"

#include "prost_planner.h"
#include "logical_expressions.h"
#include "actions.h"
#include "conditional_probability_functions.h"
#include "iterative_deepening_search.h"
#include "random_search.h"

#include "utils/timer.h"
#include "utils/math_utils.h"
#include "utils/system_utils.h"

#include <limits>
#include <iostream>

using namespace std;

vector<map<State, vector<double>, State::CompareIgnoringRemainingSteps> > UCTSearch::rewardCache;

/******************************************************************
                              UCT Node
******************************************************************/

UCTNode::~UCTNode()  {
    for(unsigned int i = 0; i < children.size(); ++i) {
        delete children[i];
    }
}

/******************************************************************
                            Constructor 
******************************************************************/

UCTSearch::UCTSearch(ProstPlanner* _planner, vector<double>& _result):
    SearchEngine("UCT", _planner, _planner->getProbabilisticTask(), _result), 
    currentRootNode(NULL),
    chosenChild(NULL),
    nextState(task->getStateSize()),
    initializer(NULL),
    initialRewards(task->getNumberOfActions(),0.0),
    pruneWithInitialization(task->isPruningEquivalentToDeterminization()),
    timeoutMethod(UCTSearch::TIME), 
    timeout(2.0),
    maxNumberOfRollouts(0),
    numberOfInitialVisits(5),
    magicConstantScaleFactor(1.0),
    allowDecisionNodeSuccessorChoiceBasedOnVisitDifference(true),
    numberOfRuns(0) {

    nodePool.resize(18000000,NULL);

    if(rewardCache.size() < 6) {
        rewardCache.resize(6);
    }
}

bool UCTSearch::setValueFromString(string& param, string& value) {
    if(param == "-mcs") {
        setMagicConstantScaleFactor(atof(value.c_str()));
        return true;
    } else if(param == "-T") {
        if(value == "TIME") {
            setTimeoutMethod(UCTSearch::TIME);
            return true;
        } else if(value == "ROLLOUTS") {
            setTimeoutMethod(UCTSearch::NUMBER_OF_ROLLOUTS);
            return true;
        } else if(value == "TIME_AND_ROLLOUTS") {
            setTimeoutMethod(UCTSearch::TIME_AND_NUMBER_OF_ROLLOUTS);
            return true;
        } else {
            return false;
        }
    } else if(param == "-t") {
        setTimeout(atof(value.c_str()));
        return true;
    } else if(param == "-r") {
        setMaxNumberOfRollouts(atoi(value.c_str()));
        return true;
    } else if(param == "-iv") {
        setNumberOfInitialVisits(atoi(value.c_str()));
        return true;
    } else if(param == "-i") {
        setInitializer(SearchEngine::fromString(value, planner, initialRewards));
        return true;
    }

    return SearchEngine::setValueFromString(param, value);
}

void UCTSearch::setInitializer(SearchEngine* _initializer) {
    initializer = _initializer;
    if(dynamic_cast<RandomSearch*>(initializer)) {
        numberOfInitialVisits = 0;
    }

    if(pruneWithInitialization) {
        initializer->setResultType(SearchEngine::PRUNED_ESTIMATE);
    } else {
        initializer->setResultType(SearchEngine::ESTIMATE);
    }
}

void UCTSearch::learn(vector<State> const& trainingSet) {
    cout << name << ": learning..." << endl;
    initializer->learn(trainingSet);

    if(initializer->getMaxSearchDepth() <= 2) {
        delete initializer;
        initializer = new RandomSearch(planner, initialRewards);
        if(pruneWithInitialization) {
            initializer->setResultType(SearchEngine::PRUNED_ESTIMATE);
        } else {
            initializer->setResultType(SearchEngine::ESTIMATE);
        }
        cout << "Aborted initialization as search depth is too low!" << endl;
    }
    cout << name << ": ...finished" << endl;
}

/******************************************************************
                 Initialization of search phases
******************************************************************/

void UCTSearch::initStep() {
    outStream << name << ": Setting search depth to " << rootState.remainingSteps() << endl;
    rewardCacheIndexForThisStep = min(5, rootState.remainingSteps());
    if(cachingEnabled) {
        outStream << "UCTSearch: Setting rewardCacheIndex to " << rewardCacheIndexForThisStep << endl;
    }
    outStream << endl;

    currentRollout = 0;

    resetNodePool();
    currentRootNode = getUCTNode();

    initRollout();
}

void UCTSearch::initRollout() {
    currentState.setTo(rootState);
    nextState.reset(rootState.remainingSteps()-1);
}

void UCTSearch::initRolloutStep() {
    currentState.reset(nextState.remainingSteps()-1);
    currentState.swap(nextState);
}

/******************************************************************
                       Main Search Functions
******************************************************************/

void UCTSearch::_run() {
    assert(resultType == SearchEngine::RELATIVE);
    initStep();
    
    if(rewardCache[rewardCacheIndexForThisStep].find(rootState) != rewardCache[rewardCacheIndexForThisStep].end()) {
        outStream << "Reusing policy!" << endl;
        result = rewardCache[rewardCacheIndexForThisStep][rootState];
        printStats();
        return;
    }

    if(resultType == SearchEngine::RELATIVE) {
        int uniquePolicyOpIndex = getUniquePolicy();
        if(uniquePolicyOpIndex != -1) {
            outStream << "Returning unique policy!" << endl << endl;
            result[uniquePolicyOpIndex] = 0.0;
            for(unsigned int i = 0; i < result.size(); ++i) {
                if(i != uniquePolicyOpIndex) {
                    result[i] = -numeric_limits<double>::max();
                }
            }
            printStats();
            return;
        }
    }
    search();    
}

void UCTSearch::search() {
    Timer timer;

    switch(timeoutMethod) {
    case UCTSearch::TIME:
        while((timer() < timeout) && (lastUsedNodePoolIndex < 15000000)) {
            initRollout();
            rolloutDecisionNode(currentRootNode);
            ++currentRollout;
        }
        break;
    case UCTSearch::NUMBER_OF_ROLLOUTS:
        while((currentRollout < maxNumberOfRollouts) && (lastUsedNodePoolIndex < 15000000)) {
            initRollout();
            rolloutDecisionNode(currentRootNode);
            ++currentRollout;
        }
        break;
    case UCTSearch::TIME_AND_NUMBER_OF_ROLLOUTS:
        while((timer() < timeout) && (currentRollout < maxNumberOfRollouts) && (lastUsedNodePoolIndex < 15000000)) {
            initRollout();
            rolloutDecisionNode(currentRootNode);
            ++currentRollout;
        }
        break;
    }

    ++numberOfRuns;

    assert(currentRootNode->children.size() == result.size());
   
    for(unsigned int i = 0; i < currentRootNode->children.size(); ++i) {
        if(currentRootNode->children[i]) {
            //As a result is the average reward in each step in the future, we have to divide by the number of remaining steps
            result[i] = currentRootNode->children[i]->expectedRewardEstimate() / (double)rootState.remainingSteps();
        } else {
            result[i] = -numeric_limits<double>::max();
        }
    }

    if(cachingEnabled) {
        assert(rewardCache[rewardCacheIndexForThisStep].find(rootState) == rewardCache[rewardCacheIndexForThisStep].end());
        rewardCache[rewardCacheIndexForThisStep][rootState] = result;
    }

    outStream << "Search time: " << timer << endl;
    printStats();
}

//static int counter = 0;

double UCTSearch::rolloutDecisionNode(UCTNode* node) {
    //++counter;
    //cout << "current state has hash key " << currentState.getHashKey() << " (" << counter << ")" << endl;

    double reward = 0.0;

    if(node->children.empty()) {
        initializeDecisionNode(node);
    }

    if(node->isARewardLock) {
        assert(task->rewardIsNextStateIndependent());
        task->calcReward(currentState, 0, nextState, reward);
        reward *= (task->getHorizon() - rootState.remainingSteps() + currentState.remainingSteps());
    } else {
        chooseDecisionNodeSuccessor(node);
        assert(chosenChild != NULL);
        
        reward = rolloutChanceNodes(chosenChild);
    }

    node->accumulatedReward += reward;
    ++node->numberOfVisits;
    ++node->numberOfChildrenVisits;

    return reward;
}

double UCTSearch::rolloutChanceNodes(UCTNode* node) {
    //sample successor state
    double reward = 0.0;
    task->calcStateTransition(currentState, chosenActionIndex, nextState, reward);

    if((nextState.remainingSteps() == 1) && task->noopIsOptimalFinalAction()) {
        //We can save the last decision, as the reward does not depend on next
        //state (we use currentState as dummy) and not positively on running actions
        double finalReward = 0.0;
        task->calcReward(nextState, 0, nextState, finalReward);
        reward += finalReward;
    } else if(nextState.remainingSteps() > 0) {
        //choose corresponding UCTNode
        chosenChild = node;
        for(int i = task->getFirstProbabilisticVarIndex(); i < task->getStateSize(); ++i) {
            if(chosenChild->children.empty()) {
                chosenChild->children.resize(2,NULL);
            }

            if(!chosenChild->children[(unsigned int)nextState[i]]) {
                chosenChild->children[(unsigned int)nextState[i]] = getUCTNode();
            }
            chosenChild = chosenChild->children[(unsigned int)nextState[i]];
        }

        //continue rollout
        initRolloutStep();

        reward += rolloutDecisionNode(chosenChild);
    }

    //update UCTNode
    node->accumulatedReward += reward;
    ++node->numberOfVisits;
    //This is commented as chance nodes don't use the numberOfChildrenVisits
    //++node->numberOfChildrenVisits; 
    return reward;
}

/******************************************************************
                       Successor Choice
******************************************************************/

void UCTSearch::chooseDecisionNodeSuccessor(UCTNode* node) {
    bestDecisionNodeChildren.clear();

    chooseUnvisitedChild(node);

    if(allowDecisionNodeSuccessorChoiceBasedOnVisitDifference && bestDecisionNodeChildren.empty()) {
        chooseDecisionNodeSuccessorBasedOnVisitDifference(node);
    }

    if(bestDecisionNodeChildren.empty()) {
        chooseDecisionNodeSuccessorBasedOnUCTFormula(node);
    }

    assert(!bestDecisionNodeChildren.empty());

    //int randNum = rand();
    //int index = (int)(randNum % bestDecisionNodeChildren.size());
    chosenActionIndex = bestDecisionNodeChildren[rand() % bestDecisionNodeChildren.size()];
    chosenChild = node->children[chosenActionIndex];
}

inline void UCTSearch::chooseUnvisitedChild(UCTNode* node) {
    for(unsigned int i = 0; i < node->children.size(); ++i) {
        if(node->children[i] && node->children[i]->numberOfVisits == 0) {
            bestDecisionNodeChildren.push_back(i);
        }
    }
}

inline void UCTSearch::chooseDecisionNodeSuccessorBasedOnVisitDifference(UCTNode* node) {
    bestDecisionNodeChildren.push_back(0);
    smallestNumVisits = node->children[0]->numberOfVisits;
    highestNumVisits = node->children[0]->numberOfVisits;

    for(unsigned int i = 1; i < node->children.size(); ++i) {
        if(node->children[i]) {
            if(MathUtils::doubleIsSmaller(node->children[i]->numberOfVisits,smallestNumVisits)) {
                bestDecisionNodeChildren.clear();
                bestDecisionNodeChildren.push_back(i);
                smallestNumVisits = node->children[i]->numberOfVisits;
            } else if(MathUtils::doubleIsEqual(node->children[i]->numberOfVisits,smallestNumVisits)) {
                bestDecisionNodeChildren.push_back(i);
            } else if(MathUtils::doubleIsGreater(node->children[i]->numberOfVisits, highestNumVisits))  {
                highestNumVisits = node->children[i]->numberOfVisits;
            }
        }
    }

    if(50*smallestNumVisits >= highestNumVisits) {
        bestDecisionNodeChildren.clear();
    }
}

inline void UCTSearch::chooseDecisionNodeSuccessorBasedOnUCTFormula(UCTNode* node) {
    if(node->numberOfVisits == 0) {
        magicConstant = 0.0;
    } else {
        magicConstant = magicConstantScaleFactor * abs(node->expectedRewardEstimate());
        if(MathUtils::doubleIsEqual(magicConstant,0.0)) {
            magicConstant = 100.0;
        }
    }

    assert(node->numberOfChildrenVisits != 0);

    bestUCTValue = -std::numeric_limits<double>::max();
    numberOfChildrenVisitsLog = log((double)node->numberOfChildrenVisits);

    for(unsigned int i = 0; i < node->children.size(); ++i) {
        if(node->children[i] != NULL) {
            visitPart = magicConstant * sqrt(numberOfChildrenVisitsLog / (double)node->children[i]->numberOfVisits);
            UCTValue = node->children[i]->expectedRewardEstimate() + visitPart;
            assert(!MathUtils::doubleIsMinusInfinity(UCTValue));

            if(MathUtils::doubleIsGreater(UCTValue,bestUCTValue)) {
                bestDecisionNodeChildren.clear();
                bestDecisionNodeChildren.push_back(i);
                bestUCTValue = UCTValue;
            } else if(MathUtils::doubleIsEqual(UCTValue,bestUCTValue)) {
                bestDecisionNodeChildren.push_back(i);
            }
        }
    }
}

/******************************************************************
             Initialization of Decision Node Children
******************************************************************/

void UCTSearch::initializeDecisionNode(UCTNode* node) {
    if(task->isARewardLock(currentState)) {
        node->isARewardLock = true;
        return;
    }

    node->children.resize(task->getNumberOfActions(),NULL);
    initializer->run(currentState);

    if(pruneWithInitialization) {
        for(unsigned int i = 0; i < node->children.size(); ++i) {
            if(!MathUtils::doubleIsMinusInfinity(initialRewards[i])) {
                initializeDecisionNodeChild(node, i, initialRewards[i]);
            }
        }
    } else {
        task->setActionsToExpand(currentState, actionsToExpand);

        for(unsigned int i = 0; i < node->children.size(); ++i) {
            assert(!MathUtils::doubleIsMinusInfinity(initialRewards[i]));
            if(actionsToExpand[i] == i) {
                initializeDecisionNodeChild(node, i, initialRewards[i]);
            }
        }
    }
}

inline void UCTSearch::initializeDecisionNodeChild(UCTNode* node, unsigned int const& index, double const& initialReward) {
    node->children[index] = getUCTNode();
    node->children[index]->numberOfVisits = numberOfInitialVisits;
    node->children[index]->accumulatedReward = (double)numberOfInitialVisits * (double)currentState.remainingSteps() * initialReward;
    node->numberOfChildrenVisits += numberOfInitialVisits;
}

/******************************************************************
                      Root State Analysis
******************************************************************/

inline int UCTSearch::getUniquePolicy() {
    initializeDecisionNode(currentRootNode);
    if(currentRootNode->isARewardLock) {
        //TODO: When we add dynamic state action constraints, return
        //an applicable action (we must also change initializeDecisionNode
        //then, as it doesn't even create the children in reward locks atm)
        outStream << "Current root state is a reward lock state!" << endl;
        return 0;
    }

    int applicableOp = -1;
    for(unsigned int i = 0; i < currentRootNode->children.size(); ++i) {
        if(currentRootNode->children[i]) {
            if(applicableOp != -1) {
                //there is more than one applicable action
                return -1;
            }
            applicableOp = i;
        }
    }

    //there is exactly one applicable op (as there must be at least one, and there is not more than one)
    assert(applicableOp != -1);
    outStream << "Only one reasonable action in current root state!" << endl;
    return applicableOp;
}

/******************************************************************
                        Memory Management
******************************************************************/

UCTNode* UCTSearch::getUCTNode() {
    assert(lastUsedNodePoolIndex < nodePool.size());

    UCTNode* res = nodePool[lastUsedNodePoolIndex];
    if(res) {
        res->accumulatedReward = 0.0;
        res->numberOfVisits = 0;
        res->numberOfChildrenVisits = 0;
        res->children.clear();
        res->isARewardLock = false;
    } else {
        res = new UCTNode();
        nodePool[lastUsedNodePoolIndex] = res;
    }

    ++lastUsedNodePoolIndex;
    return res;
}

void UCTSearch::resetNodePool() {
    for(unsigned int i = 0; i < nodePool.size(); ++i) {
        if(nodePool[i]) {
            if(nodePool[i]->children.size() > 0) {
                std::vector<UCTNode*> tmp;
                nodePool[i]->children.swap(tmp);
            }
        } else {
            break;
        }
    }
    lastUsedNodePoolIndex = 0;
}

/******************************************************************
                     Printer and Statistics
******************************************************************/

void UCTSearch::resetStats() {
    numberOfRuns = 0;

    initializer->resetStats();
}

void UCTSearch::print() {
    SearchEngine::print();
    initializer->print();
}

void UCTSearch::printStats(std::string indent) {
    SearchEngine::printStats(indent);
    if(currentRollout > 0) {
        outStream << "Performed rollouts: " << currentRollout << endl;
        outStream << "Created UCTNodes: " << lastUsedNodePoolIndex << endl;
    }
    outStream << "Initialization: ";
    initializer->printStats(indent + "  ");
}
