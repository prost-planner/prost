#ifndef THTS_H
#define THTS_H

#include "random_search.h"
#include "utils/timer.h"

template <class SearchNode>
class THTS : public SearchEngine {
public:
    enum TerminationMethod {
        TIME, //stop after timeout sec
        NUMBER_OF_TRIALS, //stop after maxNumberOfTrials trials
        TIME_AND_NUMBER_OF_TRIALS //stop after timeout sec or maxNumberOfTrials trials, whichever comes first
    };

    // Search engine creation
    virtual bool setValueFromString(std::string& param, std::string& value);

    // Learning
    virtual bool learn(std::vector<State> const& trainingSet);

    // Start the search engine
    void estimateBestActions(State const& _rootState, std::vector<int>& result);
    void estimateQValues(State const& /*_rootState*/, std::vector<double>& /*result*/, const bool& /*pruneResult*/) {
        assert(false);
    }

    // Parameter setters: overwrites
    virtual void setMaxSearchDepth(int _maxSearchDepth) {
        SearchEngine::setMaxSearchDepth(_maxSearchDepth);

        if(initializer) {
            initializer->setMaxSearchDepth(_maxSearchDepth);
        }
    }

    // Parameter setters: new parameters
    virtual void setTerminationMethod(THTS<SearchNode>::TerminationMethod _terminationMethod) {
        terminationMethod = _terminationMethod;
    }

    virtual void setTimeout(double _timeout) {
        timeout = _timeout;
    }

    virtual void setMaxNumberOfTrials(int _maxNumberOfTrials) {
        maxNumberOfTrials = _maxNumberOfTrials;
    }

    virtual void setInitializer(SearchEngine* _initializer) {
        if(initializer) {
            delete initializer;
        }
        initializer = _initializer;
    }

    // Print
    virtual void print(std::ostream& out);
    virtual void printStats(std::ostream& out, bool const& printRoundStats, std::string indent = "");

protected:
    THTS<SearchNode>(std::string _name, ProstPlanner* _planner, PlanningTask* _task) :
        SearchEngine(_name, _planner, _task), 
        currentRootNode(NULL),
        chosenChild(NULL),
        states(task->getHorizon()+1, State(task->getStateSize(), -1, task->getNumberOfStateFluentHashKeys())),
        currentStateIndex(task->getHorizon()),
        nextStateIndex(task->getHorizon()-1),
        actions(task->getHorizon(), -1),
        currentActionIndex(nextStateIndex),
        currentTrial(0),
        varIndex(-1),
        initializer(NULL),
        initialQValues(task->getNumberOfActions(),0.0),
        initializedDecisionNodes(0),
        terminationMethod(THTS<SearchNode>::TIME), 
        timeout(2.0),
        maxNumberOfTrials(0),
        numberOfRuns(0),
        cacheHits(0),
        accumulatedNumberOfRemainingStepsInFirstSolvedRootState(0),
        firstSolvedFound(false),
        accumulatedNumberOfTrialsInRootState(0),
        accumulatedNumberOfSearchNodesInRootState(0) {
        nodePool.resize(18000000,NULL);
    }

    // Main search functions
    double visitDecisionNode(SearchNode* node);
    double visitChanceNode(SearchNode* node);

    // Initialization of different search phases
    void initRound();
    void initStep(State const& _rootState);
    void initTrial();
    void initTrialStep();

    // Action selection (and trial termination)
    virtual void selectAction(SearchNode* node) = 0;

    // Outcome selection
    virtual void selectOutcome(SearchNode* node) = 0;

    // Initialization of nodes
    void initializeDecisionNode(SearchNode* node);
    virtual void initializeDecisionNodeChild(SearchNode* node, unsigned int const& actionIndex, double const& initialQValue) = 0;

    // Backup functions
    virtual void backupDecisionNodeLeaf(SearchNode* node, double const& immReward, double const& futReward) = 0;
    virtual void backupDecisionNode(SearchNode* node, double const& immReward, double const& accReward) = 0;
    virtual void backupChanceNode(SearchNode* node, double const& accReward) = 0;

    // If the root state is a reward lock or has only one reasonable
    // action, noop or the only reasonable action is returned
    int getUniquePolicy();

    // Determine if the termination criterion is fullfilled
    bool continueTrial();

    // Memory management
    SearchNode* getSearchNode();
    virtual SearchNode* getRootNode() {
        return getSearchNode();
    }
    void resetNodePool();

    // Search nodes used in trials
    SearchNode* currentRootNode;
    SearchNode* chosenChild;

    // States used in trials
    std::vector<State> states;
    int currentStateIndex;
    int nextStateIndex;

    // Actions used in trials
    std::vector<int> actions;
    int& currentActionIndex;

    // Counter for the number of trials
    int currentTrial;

    // Max search depth for the current step
    int maxSearchDepthForThisStep;

    // The number of steps that are "cut off" if a limited search
    // depth is used
    int ignoredSteps;

    // Index of variable that is currently processed
    int varIndex;

    // Search engine that estimates Q-values for initialization of
    // decison node children
    SearchEngine* initializer;
    std::vector<double> initialQValues;

    // Counter for number of initialized decision nodes in the current
    // trial
    int initializedDecisionNodes;

    // Memory management (nodePool)
    int  lastUsedNodePoolIndex;
    std::vector<SearchNode*> nodePool;

    // The timer used for timeout check
    Timer timer;

    // Parameter
    THTS<SearchNode>::TerminationMethod terminationMethod;
    double timeout;
    int maxNumberOfTrials;

    // Statistics
    int numberOfRuns;
    int cacheHits;
    int accumulatedNumberOfRemainingStepsInFirstSolvedRootState;
    bool firstSolvedFound;
    int accumulatedNumberOfTrialsInRootState;
    int accumulatedNumberOfSearchNodesInRootState;
};

/******************************************************************
                     Search Engine Creation
******************************************************************/

template <class SearchNode>
bool THTS<SearchNode>::setValueFromString(std::string& param, std::string& value) {
    if(param == "-T") {
        if(value == "TIME") {
            setTerminationMethod(THTS<SearchNode>::TIME);
            return true;
        } else if(value == "TRIALS") {
            setTerminationMethod(THTS<SearchNode>::NUMBER_OF_TRIALS);
            return true;
        } else if(value == "TIME_AND_TRIALS") {
            setTerminationMethod(THTS<SearchNode>::TIME_AND_NUMBER_OF_TRIALS);
            return true;
        } else {
            return false;
        }
    } else if(param == "-t") {
        setTimeout(atof(value.c_str()));
        return true;
    } else if(param == "-r") {
        setMaxNumberOfTrials(atoi(value.c_str()));
        return true;
    } else if(param == "-i") {
        setInitializer(SearchEngine::fromString(value, planner));
        return true;
    }

    return SearchEngine::setValueFromString(param, value);
}

/******************************************************************
                            Learning
******************************************************************/

template <class SearchNode>
bool THTS<SearchNode>::learn(std::vector<State> const& trainingSet) {
    if(!initializer->learningFinished() || !task->learningFinished()) {
        return false;
    }
    std::cout << name << ": learning..." << std::endl;

    if(initializer->getMaxSearchDepth() <= 2) {
        RandomSearch* _initializer = new RandomSearch(planner);
        setInitializer(_initializer);
        std::cout << "Aborted initialization as search depth is too low!" << std::endl;
    }
    std::cout << name << ": ...finished" << std::endl;
    return LearningComponent::learn(trainingSet);
}

/******************************************************************
                 Initialization of search phases
******************************************************************/

template <class SearchNode>
void THTS<SearchNode>::initRound() {
    firstSolvedFound = false;
}

template <class SearchNode>
void THTS<SearchNode>::initStep(State const& _rootState) {
    // Adjust maximal search depth and set root state
    if(_rootState.remainingSteps() > maxSearchDepth) {
        ignoredSteps = _rootState.remainingSteps() - maxSearchDepth;
        maxSearchDepthForThisStep = maxSearchDepth;
        states[maxSearchDepthForThisStep].setTo(_rootState);
        states[maxSearchDepthForThisStep].remainingSteps() = maxSearchDepthForThisStep;
    } else {
        ignoredSteps = 0;
        maxSearchDepthForThisStep = _rootState.remainingSteps();
        states[maxSearchDepthForThisStep].setTo(_rootState);
    }
    assert(states[maxSearchDepthForThisStep].remainingSteps() == maxSearchDepthForThisStep);

    currentStateIndex = maxSearchDepthForThisStep;
    nextStateIndex = maxSearchDepthForThisStep - 1;
    states[nextStateIndex].reset(nextStateIndex);

    // Reset step dependent counter
    currentTrial = 0;
    cacheHits = 0;

    // Reset search nodes and create root node
    resetNodePool();
    currentRootNode = getRootNode();

    outStream << name << ": Maximal search depth set to " << maxSearchDepthForThisStep << std::endl << std::endl;
}

template <class SearchNode>
void THTS<SearchNode>::initTrial() {
    currentStateIndex = maxSearchDepthForThisStep;
    nextStateIndex = maxSearchDepthForThisStep - 1;
    states[nextStateIndex].reset(nextStateIndex);

    // Reset trial dependent counter
    initializedDecisionNodes = 0;
}

template <class SearchNode>
void THTS<SearchNode>::initTrialStep() {
    --currentStateIndex;
    --nextStateIndex;
    states[nextStateIndex].reset(nextStateIndex);
}

/******************************************************************
                       Main Search Functions
******************************************************************/

template <class SearchNode>
void THTS<SearchNode>::estimateBestActions(State const& _rootState, std::vector<int>& result) {
    assert(result.empty());

    // Init round (if this is the first call in a round)
    if(_rootState.remainingSteps() == task->getHorizon()) {
        initRound();
    }

    // Init step (this function is currently only called once per
    // step) TODO: maybe we should call initRound, initStep and
    // printStats from "outside" such that we can also use this as a
    // heuristic without generating too much output
    initStep(_rootState);

    // Check if there is an obviously optimal policy (as, e.g., in the
    // last step or in a reward lock)
    int uniquePolicyOpIndex = getUniquePolicy();
    if(uniquePolicyOpIndex != -1) {
        outStream << "Returning unique policy: ";
        task->printAction(outStream, uniquePolicyOpIndex);
        outStream << std::endl << std::endl;
        result.push_back(uniquePolicyOpIndex);
        printStats(outStream, (_rootState.remainingSteps() == 1));
        return;
    }

    timer.reset();

    // Start the main loop that starts trials until some termination
    // criterion is fullfilled
    while(continueTrial()) {
        initTrial();
        visitDecisionNode(currentRootNode);
        ++currentTrial;
    }

    // Write best action indices to the result vector
    for(unsigned int i = 0; i < currentRootNode->children.size(); ++i) {
        if(currentRootNode->children[i]) {
            if(result.empty()) {
                result.push_back(i);
            } else if(MathUtils::doubleIsGreater(currentRootNode->children[i]->getExpectedRewardEstimate(), 
                                                 currentRootNode->children[result[0]]->getExpectedRewardEstimate())) {
                result.clear();
                result.push_back(i);
            } else if(MathUtils::doubleIsEqual(currentRootNode->children[i]->getExpectedRewardEstimate(),
                                               currentRootNode->children[result[0]]->getExpectedRewardEstimate())) {
                result.push_back(i);
            }
        }
    }

    // Update statistics
    ++numberOfRuns;

    if(currentRootNode->isSolved() && !firstSolvedFound) {
        // TODO: This is the first root state that was solved, so
        // everything that could happen in the future is also solved.
        // We should (at least in this case) make sure that we keep
        // the tree and simply follow the optimal policy.
        firstSolvedFound = true;
        accumulatedNumberOfRemainingStepsInFirstSolvedRootState += _rootState.remainingSteps();
    }

    if(_rootState.remainingSteps() == task->getHorizon()) {
        accumulatedNumberOfTrialsInRootState += currentTrial;
        accumulatedNumberOfSearchNodesInRootState += lastUsedNodePoolIndex;
    }

    // Print statistics
    outStream << "Search time: " << timer << std::endl;
    printStats(outStream, (_rootState.remainingSteps() == 1));
}

template <class SearchNode>
bool THTS<SearchNode>::continueTrial() {
    // Check memory constraints and solvedness
    if(currentRootNode->isSolved() || (lastUsedNodePoolIndex >= 15000000)) {
        return false;
    }

    // Check selected termination criterion
    switch(terminationMethod) {
    case THTS<SearchNode>::TIME:
        if(MathUtils::doubleIsGreater(timer(), timeout)) {
            return false;
        }
        break;
    case THTS<SearchNode>::NUMBER_OF_TRIALS:
        if(currentTrial == maxNumberOfTrials) {
            return false;
        }
        break;
    case THTS<SearchNode>::TIME_AND_NUMBER_OF_TRIALS:
        if(MathUtils::doubleIsGreater(timer(), timeout) || (currentTrial == maxNumberOfTrials)) {
            return false;
        }
        break;
    }

    return true;
}

template <class SearchNode>
double THTS<SearchNode>::visitDecisionNode(SearchNode* node) {
    double reward = 0.0;
    double futureReward = 0.0;

    if(node != currentRootNode) {
        task->calcReward(states[currentStateIndex], actions[currentActionIndex], reward);

        if(nextStateIndex == 1) {
            // This node is a leaf (the last action is optimally
            // calculated in the planning task)

            task->calcOptimalFinalReward(states[1], futureReward);
            backupDecisionNodeLeaf(node, reward, futureReward);
            return (reward + futureReward);
        } else if(task->stateValueCache.find(states[nextStateIndex]) != task->stateValueCache.end()) {
            // This state has already been solved before

            node->children.clear();
            futureReward = task->stateValueCache[states[nextStateIndex]];
            backupDecisionNodeLeaf(node, reward, futureReward);
            ++cacheHits;
            return (reward + futureReward);
        }

        // Continue trial (i.e., set next state to be the current)
        initTrialStep();
    }

    // Call initialization if necessary
    if(node->children.empty()) {
        initializeDecisionNode(node);
    }

    // Check if this is a reward lock (this is not checked before
    // initialization because we only compute it once and remember the
    // result in the nodes)
    if(node->isARewardLock()) {
        task->calcReward(states[currentStateIndex], 0, futureReward);
        futureReward *= (ignoredSteps + currentStateIndex);
        backupDecisionNodeLeaf(node, reward, futureReward);

        ++currentStateIndex;
        return (reward + futureReward);
    }

    // Select the action that is simulated
    selectAction(node);

    // If chosenChild is NULL, we have reached the end of this trial
    if(chosenChild) {
        // Sample successor state
        task->calcSuccessorAsProbabilityDistribution(states[currentStateIndex], actions[currentActionIndex], states[nextStateIndex]);
        varIndex = task->getFirstProbabilisticVarIndex();

        // Continue with the chance nodes
        futureReward = visitChanceNode(chosenChild);
    }

    // Backup this node
    backupDecisionNode(node, reward, futureReward);

    // If the backup function labeled the node as solved, we store the
    // result for the associated state in case we encounter it
    // somewhere else in the tree in the future
    if(node->isSolved()) {
        //std::cout << "solved a state with rem steps " << currentStateIndex << " in trial " << currentTrial << std::endl;
        if(cachingEnabled && task->stateValueCache.find(states[currentStateIndex]) == task->stateValueCache.end()) {
            task->stateValueCache[states[currentStateIndex]] = node->getExpectedReward();
        }
    }

    ++currentStateIndex;
    return (reward + futureReward);
}

template <class SearchNode>
double THTS<SearchNode>::visitChanceNode(SearchNode* node) {
    double futureReward;
    //TODO: Make sure this also works in deterministic domains
    assert(varIndex < task->getStateSize());

    // select outcome (and set variable in next state accordingly)
    selectOutcome(node);

    ++varIndex;
    if(varIndex == task->getStateSize()) {
        task->calcStateFluentHashKeys(states[nextStateIndex]);
        task->calcStateHashKey(states[nextStateIndex]);

        futureReward = visitDecisionNode(chosenChild);
    } else {
        futureReward = visitChanceNode(chosenChild);
    }

    backupChanceNode(node, futureReward);

    return futureReward;
}

/******************************************************************
                     Initialization of Nodes
******************************************************************/

template <class SearchNode>
void THTS<SearchNode>::initializeDecisionNode(SearchNode* node) {
    if(task->isARewardLock(states[currentStateIndex])) {
        node->isARewardLock() = true;
        return;
    }

    node->children.resize(task->getNumberOfActions(),NULL);

    //cout << "initializing state: " << endl;
    //task->printState(cout, states[currentStateIndex]);

    if(task->isPruningEquivalentToDeterminization()) {
        initializer->estimateQValues(states[currentStateIndex], initialQValues, true);
        for(unsigned int i = 0; i < node->children.size(); ++i) {
            if(!MathUtils::doubleIsMinusInfinity(initialQValues[i])) {
                initializeDecisionNodeChild(node, i, initialQValues[i]);
            }
        }
    } else {
        initializer->estimateQValues(states[currentStateIndex], initialQValues, false);
        std::vector<int> actionsToExpand = task->getApplicableActions(states[currentStateIndex], true);

        for(unsigned int i = 0; i < node->children.size(); ++i) {
            if(actionsToExpand[i] == i) {
                assert(!MathUtils::doubleIsMinusInfinity(initialQValues[i]));
                initializeDecisionNodeChild(node, i, initialQValues[i]);
            }
        }
    }

    
    if(node != currentRootNode) {
       ++initializedDecisionNodes;
    }
}

/******************************************************************
                      Root State Analysis
******************************************************************/

template <class SearchNode>
int THTS<SearchNode>::getUniquePolicy() {
    if(currentStateIndex == 1) {
        outStream << "Returning the optimal last action!" << std::endl;
        return task->getOptimalFinalActionIndex(states[1]);
    }

    std::vector<int> actionsToExpand = task->getApplicableActions(states[currentStateIndex], true);

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
                        Memory management
******************************************************************/

template <class SearchNode>
SearchNode* THTS<SearchNode>::getSearchNode() {
    assert(lastUsedNodePoolIndex < nodePool.size());

    SearchNode* res = nodePool[lastUsedNodePoolIndex];

    if(res) {
        res->reset();
    } else {
        res = new SearchNode();
        nodePool[lastUsedNodePoolIndex] = res;
    }

    ++lastUsedNodePoolIndex;
    return res;
}

template <class SearchNode>
void THTS<SearchNode>::resetNodePool() {
    for(unsigned int i = 0; i < nodePool.size(); ++i) {
        if(nodePool[i]) {
            if(nodePool[i]->children.size() > 0) {
                std::vector<SearchNode*> tmp;
                nodePool[i]->children.swap(tmp);
            }
        } else {
            break;
        }
    }
    lastUsedNodePoolIndex = 0;
}

/******************************************************************
                            Print
******************************************************************/

template <class SearchNode>
void THTS<SearchNode>::print(std::ostream& out) {
    SearchEngine::print(out);
    initializer->print(out);
}

template <class SearchNode>
void THTS<SearchNode>::printStats(std::ostream& out, bool const& printRoundStats, std::string indent) {
    SearchEngine::printStats(out, printRoundStats, indent);
    if(currentTrial > 0) {
        out << "Performed trials: " << currentTrial << std::endl;
        out << "Created SearchNodes: " << lastUsedNodePoolIndex << std::endl;
    }
    out << "Initialization: ";
    initializer->printStats(out, printRoundStats, indent + "  ");

    if(currentRootNode) {
        out << std::endl << "Q-Value Estimates: " << std::endl;
        for(unsigned int i = 0; i < currentRootNode->children.size(); ++i) {
            if(currentRootNode->children[i]) {
                task->printAction(out, i);
                out << ": " << currentRootNode->children[i]->getExpectedRewardEstimate()
                    << " (in " << currentRootNode->children[i]->numberOfVisits << " visits)" << std::endl;
            }
        }
    }

    if(printRoundStats) {
        out << std::endl << indent << "ROUND FINISHED" << std::endl;
        out << indent << "Accumulated number of remaining steps in first solved root state: " << 
            accumulatedNumberOfRemainingStepsInFirstSolvedRootState << std::endl;
        out << indent << "Accumulated number of trials in root state: " <<
            accumulatedNumberOfTrialsInRootState << std::endl;
        out << indent << "Accumulated number of search nodes in root state: " <<
            accumulatedNumberOfSearchNodesInRootState << std::endl;
    }
}

#endif
