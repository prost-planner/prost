#ifndef THTS_H
#define THTS_H

#include "planning_task.h"
#include "uniform_evaluation_search.h"
#include "utils/timer.h"

// THTS, Trial-based Heuristic Tree Search, is the implementation of
// the abstract framework described in the ICAPS 2013 paper (Thomas
// Keller and Malte Helmert: Trial-based Heuristic Tree Search for
// Finite Horizon MDPs). The described ingredients can be implemented
// in the abstract functions 

// 1. int selectAction(SearchNode*): return the index of the selected
// action

// 2. SearchNode* selectOutcome(SearchNode*, State&, int&): return the
// node that corresponds to the selected outcome and ADDITIONALLY set
// the state accordingly

// 3. bool continueTrial(SearchNode*): return false to start the
// backup phase, and true otherwise. The baseline implementation of
// this checks if the number of previously unvisited decision nodes
// that was encountered in this trial is equal to a parameter that is
// set to the horizon by default (i.e., if nothing is changed all
// trials only finish in goal states)

// 4. void initializeDecisionNode(SearchNode*): implement *how* to use
// the heuristic, not *which* heuristic to use (that is done on the
// command line with the parameter "-i"). The baseline implementation
// is an action-value initialization that calls void
// initializeDecisionNodeChild(SearchNode*, unsigned int const&,
// double const&) for each child that is supposed to be initialized.

// 5a. void backupDecisionNodeLeaf(SearchNode*, double const&, double
// const&): is called on leaf (not tip!) nodes.

// 5b. backupDecisionNode(SearchNode*, double const&, double const&):
// is called to backup non-leaf decision nodes

// 5c. backupChanceNode(SearchNode*, double const&): is called to
// backup chance nodes


// SearchNode must be a class with the following public functions and
// members:

// 1. A member variable std::vector<SearchNode> children to represent
// the tree

// 2. A function getExpectedFutureRewardEstimate() that returns a
// double, the expected reward in that node WITHOUT the immediate
// reward

// 3. A function bool isSolved() const that returns a bool indicating
// if the node has been labeled as solved

// 4. A function bool isARewardLock() const that returns a bool
// indicating if the node is a reward lock

// 5. A function void setRewardLock(bool const&) that sets the bool
// that is returned in (4.)


template <class SearchNode>
class THTS : public SearchEngine {
public:
    enum TerminationMethod {
        TIME, //stop after timeout sec
        NUMBER_OF_TRIALS, //stop after maxNumberOfTrials trials
        TIME_AND_NUMBER_OF_TRIALS //stop after timeout sec or maxNumberOfTrials trials, whichever comes first
    };

    // Set parameters from command line
    virtual bool setValueFromString(std::string& param, std::string& value);

    // Learn parameter values from a training set
    virtual bool learn(std::vector<State> const& trainingSet);

    // Start the search engine as main search engine
    bool estimateBestActions(State const& _rootState, std::vector<int>& bestActions);

    // Start the search engine for Q-value estimation
    bool estimateQValues(State const& /*_rootState*/, std::vector<int> const& /*actionsToExpand*/, std::vector<double>& /*qValues*/) {
        assert(false);
        return false;
    }

    // Parameter setter
    virtual void setMaxSearchDepth(int _maxSearchDepth) {
        SearchEngine::setMaxSearchDepth(_maxSearchDepth);

        if(initializer) {
            initializer->setMaxSearchDepth(_maxSearchDepth);
        }
    }

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

    virtual void setNumberOfNewDecisionNodesPerTrial(int _numberOfNewDecisionNodesPerTrial) {
        numberOfNewDecisionNodesPerTrial = _numberOfNewDecisionNodesPerTrial;
    }

    // Printer
    virtual void print(std::ostream& out);
    virtual void printStats(std::ostream& out, bool const& printRoundStats, std::string indent = "");

protected:
    THTS<SearchNode>(std::string _name, ProstPlanner* _planner) :
        SearchEngine(_name, _planner, true), 
        currentRootNode(NULL),
        chosenOutcome(NULL),
        states(successorGenerator->getHorizon()+1, State(successorGenerator->getStateSize(), -1, successorGenerator->getNumberOfStateFluentHashKeys())),
        currentStateIndex(successorGenerator->getHorizon()),
        nextStateIndex(successorGenerator->getHorizon()-1),
        actions(successorGenerator->getHorizon(), -1),
        currentActionIndex(nextStateIndex),
        currentTrial(0),
        initializer(NULL),
        initialQValues(successorGenerator->getNumberOfActions(),0.0),
        initializedDecisionNodes(0),
        terminationMethod(THTS<SearchNode>::TIME), 
        timeout(1.0),
        maxNumberOfTrials(0),
        numberOfNewDecisionNodesPerTrial(successorGenerator->getHorizon()+1),
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

    // Action selection
    virtual int selectAction(SearchNode* node) = 0;

    // Outcome selection
    virtual SearchNode* selectOutcome(SearchNode* node, State& stateAsProbDistr, int& varIndex) = 0;

    // Trial length determination
    virtual bool continueTrial(SearchNode* /*node*/) {
        return (initializedDecisionNodes < numberOfNewDecisionNodesPerTrial);
    }

    // Initialization of nodes
    virtual void initializeDecisionNode(SearchNode* node);
    virtual void initializeDecisionNodeChild(SearchNode* node, unsigned int const& actionIndex, double const& initialQValue) = 0;

    // Backup functions
    virtual void backupDecisionNodeLeaf(SearchNode* node, double const& immReward, double const& futReward) = 0;
    virtual void backupDecisionNode(SearchNode* node, double const& immReward, double const& accReward) = 0;
    virtual void backupChanceNode(SearchNode* node, double const& accReward) = 0;

    // If the root state is a reward lock or has only one reasonable
    // action, noop or the only reasonable action is returned
    int getUniquePolicy();

    // Determine if the termination criterion is fullfilled or if we
    // want another trial
    bool moreTrials();

    // Memory management
    SearchNode* getSearchNode();
    virtual SearchNode* getRootNode() {
        return getSearchNode();
    }
    void resetNodePool();

    // The number of remaining steps until the max search depth for
    // this state is reached
    int const& remainingConsideredSteps() const {
        return currentStateIndex;
    }

    // The action that was selected (use only in backup phase)
    int const& selectedActionIndex() const {
        return actions[currentStateIndex-1];
    }

private:
    // Search nodes used in trials
    SearchNode* currentRootNode;
    SearchNode* chosenOutcome;

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

    // Variable used to navigate through chance node layers
    int chanceNodeVarIndex;

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
    int numberOfNewDecisionNodesPerTrial;

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
    } else if(param == "-ndn") {
        setNumberOfNewDecisionNodesPerTrial(atoi(value.c_str()));
        return true;
    }

    return SearchEngine::setValueFromString(param, value);
}

/******************************************************************
                            Learning
******************************************************************/

template <class SearchNode>
bool THTS<SearchNode>::learn(std::vector<State> const& trainingSet) {
    if(!initializer->learningFinished() || !successorGenerator->learningFinished() || !applicableActionGenerator->learningFinished()) {
        return false;
    }
    std::cout << name << ": learning..." << std::endl;

    if(initializer->getMaxSearchDepth() == 0) {
        UniformEvaluationSearch* _initializer = new UniformEvaluationSearch(planner);
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
bool THTS<SearchNode>::estimateBestActions(State const& _rootState, std::vector<int>& bestActions) {
    assert(bestActions.empty());

    // Init round (if this is the first call in a round)
    if(_rootState.remainingSteps() == successorGenerator->getHorizon()) {
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
        successorGenerator->printAction(outStream, uniquePolicyOpIndex);
        outStream << std::endl << std::endl;
        bestActions.push_back(uniquePolicyOpIndex);
        currentRootNode = NULL;
        printStats(outStream, (_rootState.remainingSteps() == 1));
        return true;
    }

    timer.reset();

    // Start the main loop that starts trials until some termination
    // criterion is fullfilled
    while(moreTrials()) {
        //std::cout << "Trial " << (currentTrial+1) << std::endl;
        initTrial();
        visitDecisionNode(currentRootNode);
        ++currentTrial;
    }

    double stateValue = -std::numeric_limits<double>::max();

    // Write best action indices to the result vector
    for(unsigned int actionIndex = 0; actionIndex < currentRootNode->children.size(); ++actionIndex) {
        if(currentRootNode->children[actionIndex]) {
            if(MathUtils::doubleIsGreater(currentRootNode->children[actionIndex]->getExpectedRewardEstimate(), stateValue)) {
                stateValue = currentRootNode->children[actionIndex]->getExpectedRewardEstimate();
                bestActions.clear();
                bestActions.push_back(actionIndex);
            } else if(MathUtils::doubleIsEqual(currentRootNode->children[actionIndex]->getExpectedRewardEstimate(), stateValue)) {
                bestActions.push_back(actionIndex);
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

    if(_rootState.remainingSteps() == successorGenerator->getHorizon()) {
        accumulatedNumberOfTrialsInRootState += currentTrial;
        accumulatedNumberOfSearchNodesInRootState += lastUsedNodePoolIndex;
    }

    // Print statistics
    outStream << "Search time: " << timer << std::endl;
    printStats(outStream, (_rootState.remainingSteps() == 1));

    return !bestActions.empty();
}

template <class SearchNode>
bool THTS<SearchNode>::moreTrials() {
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
        successorGenerator->calcReward(states[currentStateIndex], actions[currentActionIndex], reward);

        if(nextStateIndex == 1) {
            // This node is a leaf (the last action is optimally
            // calculated in the planning task)

            successorGenerator->calcOptimalFinalReward(states[1], futureReward);
            backupDecisionNodeLeaf(node, reward, futureReward);
            return (reward + futureReward);
        } else if(successorGenerator->stateValueCache.find(states[nextStateIndex]) != successorGenerator->stateValueCache.end()) {
            // This state has already been solved before

            node->children.clear();
            futureReward = successorGenerator->stateValueCache[states[nextStateIndex]];
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
        successorGenerator->calcReward(states[currentStateIndex], 0, futureReward);
        futureReward *= (ignoredSteps + currentStateIndex);
        backupDecisionNodeLeaf(node, reward, futureReward);

        ++currentStateIndex;
        return (reward + futureReward);
    }

    // Check if we continue with this trial
    if(continueTrial(node)) {
        // Select the action that is simulated
        actions[currentActionIndex] = selectAction(node);
        assert(node->children[actions[currentActionIndex]]);
        assert(!node->children[actions[currentActionIndex]]->isSolved());

        // cout << "Chosen action is ";
        // successorGenerator->printAction(cout, actions[currentActionIndex]);
        // cout << endl;

        // Sample successor state
        successorGenerator->calcSuccessorAsProbabilityDistribution(states[currentStateIndex], actions[currentActionIndex], states[nextStateIndex]);
        chanceNodeVarIndex = successorGenerator->getFirstProbabilisticVarIndex();

        if(successorGenerator->isDeterministic()) {
            // This task is deterministic -> there are no chance nodes
            successorGenerator->calcStateFluentHashKeys(states[nextStateIndex]);
            successorGenerator->calcStateHashKey(states[nextStateIndex]);
            futureReward = visitDecisionNode(node->children[actions[currentActionIndex]]);
        } else {
            // Continue with chance nodes
            futureReward = visitChanceNode(node->children[actions[currentActionIndex]]);
        }
    } else {
        // No action was chosen in this decision node as we stop the
        // trial
        actions[currentActionIndex] = -1;
    }

    // Backup this node
    backupDecisionNode(node, reward, futureReward);

    // If the backup function labeled the node as solved, we store the
    // result for the associated state in case we encounter it
    // somewhere else in the tree in the future
    if(node->isSolved()) {
        //std::cout << "solved a state with rem steps " << currentStateIndex << " in trial " << currentTrial << std::endl;
        if(cachingEnabled && successorGenerator->stateValueCache.find(states[currentStateIndex]) == successorGenerator->stateValueCache.end()) {
            successorGenerator->stateValueCache[states[currentStateIndex]] = node->getExpectedFutureRewardEstimate();
        }
    }

    ++currentStateIndex;
    return (reward + futureReward);
}

template <class SearchNode>
double THTS<SearchNode>::visitChanceNode(SearchNode* node) {
    double futureReward;
    //TODO: Make sure this also works in deterministic domains
    assert(chanceNodeVarIndex < successorGenerator->getStateSize());

    // select outcome (and set the variable in next state accordingly)
    chosenOutcome = selectOutcome(node, states[nextStateIndex], chanceNodeVarIndex);
    assert(MathUtils::doubleIsEqual(states[nextStateIndex][chanceNodeVarIndex],0.0) || 
           MathUtils::doubleIsEqual(states[nextStateIndex][chanceNodeVarIndex],1.0));

    ++chanceNodeVarIndex;
    if(chanceNodeVarIndex == successorGenerator->getStateSize()) {
        successorGenerator->calcStateFluentHashKeys(states[nextStateIndex]);
        successorGenerator->calcStateHashKey(states[nextStateIndex]);

        futureReward = visitDecisionNode(chosenOutcome);
    } else {
        futureReward = visitChanceNode(chosenOutcome);
    }

    backupChanceNode(node, futureReward);

    return futureReward;
}

/******************************************************************
                     Initialization of Nodes
******************************************************************/

template <class SearchNode>
void THTS<SearchNode>::initializeDecisionNode(SearchNode* node) {
    if(successorGenerator->isARewardLock(states[currentStateIndex])) {
        node->setRewardLock(true);
        return;
    }

    node->children.resize(applicableActionGenerator->getNumberOfActions(), NULL);

    //cout << "initializing state: " << endl;
    //successorGenerator->printState(cout, states[currentStateIndex]);

    std::vector<int> actionsToExpand = applicableActionGenerator->getApplicableActions(states[currentStateIndex]);
    initializer->estimateQValues(states[currentStateIndex], actionsToExpand, initialQValues);

    for(unsigned int i = 0; i < node->children.size(); ++i) {
        if(actionsToExpand[i] == i) {
            initializeDecisionNodeChild(node, i, initialQValues[i]);
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
        return successorGenerator->getOptimalFinalActionIndex(states[1]);
    }

    std::vector<int> actionsToExpand = applicableActionGenerator->getApplicableActions(states[currentStateIndex]);

    if(successorGenerator->isARewardLock(states[currentStateIndex])) {
        outStream << "Current root state is a reward lock state!" << std::endl;

        for(unsigned int i = 0; i < actionsToExpand.size(); ++i) {
            if(actionsToExpand[i] == i) {
                return i;
            }
        }

        assert(false);
    }

    std::vector<int> applicableActionIndices = applicableActionGenerator->getIndicesOfApplicableActions(states[currentStateIndex]);
    assert(!applicableActionIndices.empty());

    if(applicableActionIndices.size() == 1) {
        outStream << "Only one reasonable action in current root state!" << std::endl;
        return applicableActionIndices[0];
    }

    // There is more than one applicable action
    return -1;
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
    if(initializer) {
        initializer->print(out);
    }
}

template <class SearchNode>
void THTS<SearchNode>::printStats(std::ostream& out, bool const& printRoundStats, std::string indent) {
    SearchEngine::printStats(out, printRoundStats, indent);

    if(currentTrial > 0) {
        out << "Performed trials: " << currentTrial << std::endl;
        out << "Created SearchNodes: " << lastUsedNodePoolIndex << std::endl;
        out << indent << "Cache Hits: " << cacheHits << std::endl;
    }
    if(initializer) {
        out << "Initialization: " << std::endl;
        initializer->printStats(out, printRoundStats, indent + "  ");
    }

    if(currentRootNode) {
        out << std::endl << indent << "Root Node: " << std::endl;
        currentRootNode->print(out);
        out << std::endl << "Q-Value Estimates: " << std::endl;
        for(unsigned int i = 0; i < currentRootNode->children.size(); ++i) {
            if(currentRootNode->children[i]) {
                out << indent;
                successorGenerator->printAction(out, i);
                out << ": ";
                currentRootNode->children[i]->print(out);
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
