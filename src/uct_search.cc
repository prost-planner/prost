#include "uct_search.h"

#include "prost_planner.h"
#include "logical_expressions.h"
#include "actions.h"
#include "conditional_probability_function.h"
#include "iterative_deepening_search.h"
#include "random_search.h"

#include "utils/timer.h"
#include "utils/math_utils.h"
#include "utils/system_utils.h"

#include <limits>
#include <iostream>

using namespace std;

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

UCTSearch::UCTSearch(ProstPlanner* _planner):
    SearchEngine("UCT", _planner, _planner->getProbabilisticTask()), 
    currentRootNode(NULL),
    chosenChild(NULL),
    states(task->getHorizon()+1, State(task->getStateSize(), -1, task->getNumberOfStateFluentHashKeys())),
    currentStateIndex(task->getHorizon()),
    nextStateIndex(task->getHorizon()-1),
    actions(task->getHorizon(), -1),
    currentActionIndex(nextStateIndex),
    ignoredSteps(0),
    initializer(NULL),
    initialRewards(task->getNumberOfActions(),0.0),
    timeoutMethod(UCTSearch::TIME), 
    timeout(2.0),
    maxNumberOfRollouts(0),
    numberOfInitialVisits(5),
    magicConstantScaleFactor(1.0),
    allowDecisionNodeSuccessorChoiceBasedOnVisitDifference(true),
    numberOfRuns(0) {

    nodePool.resize(18000000,NULL);
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
        setInitializer(SearchEngine::fromString(value, planner));
        return true;
    }

    return SearchEngine::setValueFromString(param, value);
}

void UCTSearch::setInitializer(SearchEngine* _initializer) {
    initializer = _initializer;
    if(dynamic_cast<RandomSearch*>(initializer)) {
        numberOfInitialVisits = 0;
    }
}

/******************************************************************
                            Learning
******************************************************************/

bool UCTSearch::learn(vector<State> const& trainingSet) {
    if(!initializer->learningFinished() || !task->learningFinished()) {
        return false;
    }
    cout << name << ": learning..." << endl;

    if(initializer->getMaxSearchDepth() <= 2) {
        delete initializer;
        RandomSearch* _initializer = new RandomSearch(planner);
        setInitializer(_initializer);
        cout << "Aborted initialization as search depth is too low!" << endl;
    }
    cout << name << ": ...finished" << endl;
    return LearningComponent::learn(trainingSet);
}

/******************************************************************
                 Initialization of search phases
******************************************************************/

void UCTSearch::initStep(State const& _rootState) {
    ignoredSteps = 0;
    maxSearchDepthForThisStep = _rootState.remainingSteps();

    if(_rootState.remainingSteps() > maxSearchDepth) {
        ignoredSteps = _rootState.remainingSteps() - maxSearchDepth;
        maxSearchDepthForThisStep = maxSearchDepth;
        states[maxSearchDepthForThisStep].setTo(_rootState);
        states[maxSearchDepthForThisStep].remainingSteps() = maxSearchDepthForThisStep;
    } else {
        states[maxSearchDepthForThisStep].setTo(_rootState);
    }

    assert(states[maxSearchDepthForThisStep].remainingSteps() == maxSearchDepthForThisStep);

    outStream << name << ": Setting search depth to " << maxSearchDepthForThisStep << endl;

    /*
    cacheIndexForThisStep = min(5, rootState.remainingSteps());
    if(cachingEnabled) {
        outStream << "UCTSearch: Setting cacheIndex to " << cacheIndexForThisStep << endl;
    }
    */
    outStream << endl;

    currentStateIndex = maxSearchDepthForThisStep;
    nextStateIndex = maxSearchDepthForThisStep-1;
    states[nextStateIndex].reset(nextStateIndex);

    currentRollout = 0;

    resetNodePool();
    currentRootNode = getUCTNode();
}

void UCTSearch::initRollout() {
    currentStateIndex = maxSearchDepthForThisStep;
    nextStateIndex = maxSearchDepthForThisStep-1;
    states[nextStateIndex].reset(nextStateIndex);
}

void UCTSearch::initRolloutStep() {
    --currentStateIndex;
    --nextStateIndex;
    states[nextStateIndex].reset(nextStateIndex);
}

/******************************************************************
                       Main Search Functions
******************************************************************/

void UCTSearch::estimateBestActions(State const& _rootState, std::vector<int>& result) {
    assert(result.empty());
    initStep(_rootState);

    /*
    if(bestActionCache[cacheIndexForThisStep].find(rootState) != bestActionCache[cacheIndexForThisStep].end()) {
        outStream << "Reusing policy!" << endl;
        result = bestActionCache[cacheIndexForThisStep][rootState];
        printStats(outStream);
        return;
    }
    */

    int uniquePolicyOpIndex = getUniquePolicy();
    if(uniquePolicyOpIndex != -1) {
        outStream << "Returning unique policy: ";
        task->printAction(outStream, uniquePolicyOpIndex);
        outStream << endl << endl;
        result.push_back(uniquePolicyOpIndex);
        printStats(outStream);
        return;
    }
    search();

    for(unsigned int i = 0; i < currentRootNode->children.size(); ++i) {
        if(currentRootNode->children[i]) {
            if(result.empty()) {
                result.push_back(i);
            } else if(MathUtils::doubleIsGreater(currentRootNode->children[i]->expectedRewardEstimate(), currentRootNode->children[result[0]]->expectedRewardEstimate())) {
                result.clear();
                result.push_back(i);
            } else if(MathUtils::doubleIsEqual(currentRootNode->children[i]->expectedRewardEstimate(), currentRootNode->children[result[0]]->expectedRewardEstimate())) {
                result.push_back(i);
            }
        }
    }

    /*
    if(cachingEnabled) {
        assert(bestActionCache[cacheIndexForThisStep].find(rootState) == bestActionCache[cacheIndexForThisStep].end());
        bestActionCache[cacheIndexForThisStep][rootState] = result;
    }
    */
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
            // cout << "rollout " << currentRollout << endl;
            initRollout();
            rolloutDecisionNode(currentRootNode);
            ++currentRollout;

            // currentRootNode->print(cout);
            // for(unsigned int i = 0; i < currentRootNode->children.size(); ++i) {
            //     if(currentRootNode->children[i]) {
            //         currentRootNode->children[i]->print(cout);
            //     }
            // }
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

    outStream << "Search time: " << timer << endl;
    printStats(outStream);
}

//static int counter = 0;

double UCTSearch::rolloutDecisionNode(UCTNode* node) {
    double reward = 0.0;

    if(node != currentRootNode) {
        task->calcReward(states[currentStateIndex], actions[currentActionIndex], reward);

        if(nextStateIndex == 1) {
            //this node is a leaf (the last action is optimally
            //calculated by the planning task)
            double finalReward = 0.0;
            task->calcOptimalFinalReward(states[1], finalReward);
            reward += finalReward;
            return reward;
        }

        //continue rollout (i.e., set next state to be the current))
        initRolloutStep();
    }

    if(node->children.empty()) {
        initializeDecisionNode(node);
    }

    if(node->isARewardLock) {
        double rewardLockReward = 0.0;
        task->calcReward(states[currentStateIndex], 0, rewardLockReward);
        rewardLockReward *= (ignoredSteps + currentStateIndex);
        reward += rewardLockReward;
    } else {
        chooseDecisionNodeSuccessor(node);
        assert(chosenChild != NULL);
        
        reward += rolloutChanceNodes(chosenChild);
    }

    node->accumulatedReward += reward;
    ++node->numberOfChildrenVisits;
    ++node->numberOfVisits;

    return reward;
}

double UCTSearch::rolloutChanceNodes(UCTNode* node) {
    assert(task->getFirstProbabilisticVarIndex() != task->getStateSize());

    //sample successor state
    task->calcSuccessorState(states[currentStateIndex], actions[currentActionIndex], states[nextStateIndex]);

    chosenChild = node;
    for(int i = task->getFirstProbabilisticVarIndex(); i < task->getStateSize(); ++i) {
        if(chosenChild->children.empty()) {
            chosenChild->children.resize(2,NULL);
        }

        if(!chosenChild->children[(unsigned int)states[nextStateIndex][i]]) {
            chosenChild->children[(unsigned int)states[nextStateIndex][i]] = getUCTNode();
        }
        chosenChild = chosenChild->children[(unsigned int)states[nextStateIndex][i]];
        // cout << "chosen outcome is " << states[nextStateIndex][i] << endl;
    }

    //update UCTNode (we only update those that are children of decision nodes, but that is sufficient)
    double reward = rolloutDecisionNode(chosenChild);
    node->accumulatedReward += reward;
    ++node->numberOfVisits;

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

    actions[currentActionIndex] = bestDecisionNodeChildren[rand() % bestDecisionNodeChildren.size()];
    chosenChild = node->children[actions[currentActionIndex]];

    // cout << "Chosen action is ";
    // task->printAction(cout, actions[currentActionIndex]);
    // cout << endl;
}

inline void UCTSearch::chooseUnvisitedChild(UCTNode* node) {
    for(unsigned int i = 0; i < node->children.size(); ++i) {
        if(node->children[i] && node->children[i]->numberOfVisits == 0) {
            bestDecisionNodeChildren.push_back(i);
        }
    }
}

inline void UCTSearch::chooseDecisionNodeSuccessorBasedOnVisitDifference(UCTNode* node) {
    unsigned int childIndex = 0;
    for(; childIndex < node->children.size(); ++childIndex) {
    	if(node->children[childIndex]) {
            bestDecisionNodeChildren.push_back(childIndex);
            smallestNumVisits = node->children[childIndex]->numberOfVisits;
            highestNumVisits = node->children[childIndex]->numberOfVisits;
            break;
        }
    }

    ++childIndex;

    for(; childIndex < node->children.size(); ++childIndex) {
        if(node->children[childIndex]) {
            if(MathUtils::doubleIsSmaller(node->children[childIndex]->numberOfVisits,smallestNumVisits)) {
                bestDecisionNodeChildren.clear();
                bestDecisionNodeChildren.push_back(childIndex);
                smallestNumVisits = node->children[childIndex]->numberOfVisits;
            } else if(MathUtils::doubleIsEqual(node->children[childIndex]->numberOfVisits,smallestNumVisits)) {
                bestDecisionNodeChildren.push_back(childIndex);
            } else if(MathUtils::doubleIsGreater(node->children[childIndex]->numberOfVisits, highestNumVisits))  {
                highestNumVisits = node->children[childIndex]->numberOfVisits;
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
        magicConstant = magicConstantScaleFactor * std::abs(node->expectedRewardEstimate());
        if(MathUtils::doubleIsEqual(magicConstant,0.0)) {
            magicConstant = 100.0;
        }
    }

    assert(node->numberOfChildrenVisits != 0);

    bestUCTValue = -std::numeric_limits<double>::max();
    numberOfChildrenVisitsLog = std::log((double)node->numberOfChildrenVisits);

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
    if(task->isARewardLock(states[currentStateIndex])) {
        node->isARewardLock = true;
        return;
    }

    node->children.resize(task->getNumberOfActions(),NULL);

    //cout << "initializing state: " << endl;
    //task->printState(cout, states[currentStateIndex]);

    if(task->isPruningEquivalentToDeterminization()) {
        initializer->estimateQValues(states[currentStateIndex], initialRewards, true);
        for(unsigned int i = 0; i < node->children.size(); ++i) {
            if(!MathUtils::doubleIsMinusInfinity(initialRewards[i])) {
                initializeDecisionNodeChild(node, i, initialRewards[i]);
            }
        }
    } else {
        initializer->estimateQValues(states[currentStateIndex], initialRewards, false);
        vector<int> actionsToExpand = task->getApplicableActions(states[currentStateIndex], true);

        for(unsigned int i = 0; i < node->children.size(); ++i) {
            if(actionsToExpand[i] == i) {
                assert(!MathUtils::doubleIsMinusInfinity(initialRewards[i]));
                initializeDecisionNodeChild(node, i, initialRewards[i]);
            }
        }
    }
}

inline void UCTSearch::initializeDecisionNodeChild(UCTNode* node, unsigned int const& index, double const& initialReward) {
    node->children[index] = getUCTNode();
    node->children[index]->numberOfVisits = numberOfInitialVisits;
    node->children[index]->accumulatedReward = (double)numberOfInitialVisits * (double)currentStateIndex * initialReward;
    node->numberOfChildrenVisits += numberOfInitialVisits;

    //cout << "initializing child ";
    //task->printAction(cout,index);
    //cout << " with " << initialReward << " leading to init of " << node->children[index]->accumulatedReward << endl;
}

/******************************************************************
                      Root State Analysis
******************************************************************/

inline int UCTSearch::getUniquePolicy() {
    if(currentStateIndex == 1) {
        outStream << "Returning the optimal last action!" << endl;
        return task->getOptimalFinalActionIndex(states[1]);
    }

    vector<int> actionsToExpand = task->getApplicableActions(states[currentStateIndex], true);

    if(task->isARewardLock(states[currentStateIndex])) {
        outStream << "Current root state is a reward lock state!" << std::endl;

        for(unsigned int i = 0; i < actionsToExpand.size(); ++i) {
            if(actionsToExpand[i] == i) {
                return i;
            }
        }

        assert(false);
    }

    int applicableOp = -1;
    for(unsigned int i = 0; i < actionsToExpand.size(); ++i) {
        if(actionsToExpand[i] == i) {
            if(applicableOp != -1) {
                //there is more than one applicable action
                return -1;
            }
            applicableOp = i;
        }
    }

    //there is exactly one applicable op (as there must be at least one, and there is not more than one)
    assert(applicableOp != -1);
    outStream << "Only one reasonable action in current root state!" << std::endl;
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

void UCTSearch::print(ostream& out) {
    SearchEngine::print(out);
    initializer->print(out);
}

void UCTSearch::printStats(ostream& out, string indent) {
    SearchEngine::printStats(out, indent);
    if(currentRollout > 0) {
        out << "Performed rollouts: " << currentRollout << endl;
        out << "Created UCTNodes: " << lastUsedNodePoolIndex << endl;
    }
    out << "Initialization: ";
    initializer->printStats(out, indent + "  ");

    if(currentRootNode) {
        out << endl << "Q-Value Estimates: " << endl;
        for(unsigned int i = 0; i < currentRootNode->children.size(); ++i) {
            if(currentRootNode->children[i]) {
                task->printAction(out, i);
                out << ": " << currentRootNode->children[i]->expectedRewardEstimate()
                    << " (in " << currentRootNode->children[i]->numberOfVisits << " visits)" << endl;
            }
        }
    }
}
