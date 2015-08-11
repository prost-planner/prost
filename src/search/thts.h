#ifndef THTS_H
#define THTS_H

#include "uniform_evaluation_search.h"
#include "utils/timer.h"

// THTS, Trial-based Heuristic Tree Search, is the implementation of the
// abstract framework described in the ICAPS 2013 paper "Trial-based Heuristic
// Tree Search for Finite Horizon MDPs" (Keller & Helmert). The described
// ingredients can be implemented in the abstract functions

// 1. int selectAction(SearchNode*): return the index of the selected action

// 2. SearchNode* selectOutcome(SearchNode*, PDSTate&, State&, int&): return the
// node that corresponds to the selected outcome and ADDITIONALLY set the state
// accordingly

// 3. bool continueTrial(SearchNode*): return false to start the backup phase,
// and true otherwise. The baseline implementation of this checks if the number
// of previously unvisited decision nodes that was encountered in this trial is
// equal to a parameter that is set to the horizon by default (i.e., if nothing
// is changed all trials only finish in goal states)

// 4. void initializeDecisionNode(SearchNode*): implement *how* to use the
// heuristic, not *which* heuristic to use (that is done on the command line
// with the parameter "-i"). The baseline implementation is an action-value
// initialization that calls void initializeDecisionNodeChild(SearchNode*,
// unsigned int const&, double const&) for each child that is supposed to be
// initialized.

// 5a. void backupDecisionNodeLeaf(SearchNode*, double const&, double const&):
// is called on leaf (not tip!) nodes.

// 5b. backupDecisionNode(SearchNode*, double const&, double const&): is called
// to backup non-leaf decision nodes

// 5c. backupChanceNode(SearchNode*, double const&): is called to backup chance
// nodes

// 6. recommendAction(vector<int>&): is called to derive the decision which
// action is actually executed

// SearchNode must be a class with the following public functions and members:

// 1. A member variable std::vector<SearchNode*> children to represent the tree

// 2. A function getExpectedFutureRewardEstimate() that returns a double, the
// expected reward in that node WITHOUT the immediate reward

// 3. A function bool isSolved() const that returns a bool indicating if the
// node has been labeled as solved

// 4. A function bool isARewardLock() const that returns a bool indicating if
// the node is a reward lock

// 5. A function void setRewardLock(bool const&) that sets the bool that is
// returned in (4.)


template <class SearchNode>
class THTS : public ProbabilisticSearchEngine {
public:
    enum TerminationMethod {
        TIME, //stop after timeout sec
        NUMBER_OF_TRIALS, //stop after maxNumberOfTrials trials
        TIME_AND_NUMBER_OF_TRIALS //stop after timeout sec or maxNumberOfTrials trials, whichever comes first
    };

    // Parameter setter
    virtual bool setValueFromString(std::string& param, std::string& value);

    // Disables caching because memory becomes sparse.
    void disableCaching();

    // Learns parameter values from a random training set.
    virtual void learn();

    // Start the search engine as main search engine
    bool estimateBestActions(State const& _rootState,
                             std::vector<int>& bestActions);

    // Start the search engine for Q-value estimation
    bool estimateQValues(State const& /*_rootState*/,
                         std::vector<int> const& /*actionsToExpand*/,
                         std::vector<double>& /*qValues*/) {
        assert(false);
        return false;
    }

    // Parameter setter
    virtual void setMaxSearchDepth(int _maxSearchDepth) {
        SearchEngine::setMaxSearchDepth(_maxSearchDepth);

        if (initializer) {
            initializer->setMaxSearchDepth(_maxSearchDepth);
        }
    }

    virtual void setTerminationMethod(
            THTS<SearchNode>::TerminationMethod _terminationMethod) {
        terminationMethod = _terminationMethod;
    }

    virtual void setMaxNumberOfTrials(int _maxNumberOfTrials) {
        maxNumberOfTrials = _maxNumberOfTrials;
    }

    virtual void setInitializer(SearchEngine* _initializer) {
        if (initializer) {
            delete initializer;
        }
        initializer = _initializer;
    }

    virtual void setNumberOfNewDecisionNodesPerTrial(
            int _numberOfNewDecisionNodesPerTrial) {
        numberOfNewDecisionNodesPerTrial = _numberOfNewDecisionNodesPerTrial;
    }

    virtual void setMaxNumberOfNodes(int _maxNumberOfNodes) {
        maxNumberOfNodes = _maxNumberOfNodes;
        // Resize the node pool and give it a "safety net" of 20000 nodes (this
        // is because the termination criterion is checked only at the root and
        // not in the middle of a trial)
        nodePool.resize(maxNumberOfNodes + 20000, NULL);
    }

    virtual void setSelectMostVisited(bool _selectMostVisited) {
        selectMostVisited = _selectMostVisited;
    }

    // Used only for testing
    void setCurrentRootNode(SearchNode* node) {
        currentRootNode = node;
    }

    // Printer
    virtual void print(std::ostream& out);
    virtual void printStats(std::ostream& out, bool const& printRoundtats,
            std::string indent = "") const;

protected:
    THTS<SearchNode>(std::string _name) :
        ProbabilisticSearchEngine(_name),
        backupLock(false),
        maxLockDepth(0),
        currentRootNode(NULL),
        chosenOutcome(NULL),
        states(SearchEngine::horizon + 1),
        stepsToGoInCurrentState(SearchEngine::horizon),
        stepsToGoInNextState(SearchEngine::horizon - 1),
        actions(SearchEngine::horizon, -1),
        currentActionIndex(stepsToGoInNextState),
        currentTrial(0),
        initializer(NULL),
        initialQValues(SearchEngine::numberOfActions, 0.0),
        initializedDecisionNodes(0),
        lastUsedNodePoolIndex(0),
        terminationMethod(THTS<SearchNode>::TIME),
        maxNumberOfTrials(0),
        numberOfNewDecisionNodesPerTrial(SearchEngine::horizon + 1),
        selectMostVisited(false),
        numberOfRuns(0),
        cacheHits(0),
        accumulatedNumberOfRemainingStepsInFirstSolvedRootState(0),
        firstSolvedFound(false),
        accumulatedNumberOfTrialsInRootState(0),
        accumulatedNumberOfSearchNodesInRootState(0),
        skippedBackups(0) {
        setMaxNumberOfNodes(24000000);
        setTimeout(1.0);
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
    virtual SearchNode* selectOutcome(SearchNode* node, PDState& nextState,
            int& varIndex) = 0;

    // Trial length determinization
    virtual bool continueTrial(SearchNode* /*node*/) {
        return initializedDecisionNodes < numberOfNewDecisionNodesPerTrial;
    }

    // Initialization of nodes
    virtual void initializeDecisionNode(SearchNode* node);
    virtual void initializeDecisionNodeChild(SearchNode* node,
            unsigned int const& actionIndex,
            double const& initialQValue) = 0;

    // Backup functions
    virtual void backupDecisionNodeLeaf(SearchNode* node,
            double const& immReward,
            double const& futReward) = 0;
    virtual void backupDecisionNode(SearchNode* node, double const& immReward,
            double const& accReward) = 0;
    virtual void backupChanceNode(SearchNode* node,
            double const& accReward) = 0;

    // Recommendation function
    virtual void recommendAction(std::vector<int>& bestActions);

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
    virtual SearchNode* getDummyNode() {
        return getSearchNode();
    }
    void resetNodePool();

    // The number of remaining steps until the max search depth for
    // this state is reached
    int const& remainingConsideredSteps() const {
        return stepsToGoInCurrentState;
    }

    // The action that was selected (use only in backup phase)
    int const& selectedActionIndex() const {
        return actions[stepsToGoInCurrentState - 1];
    }

    SearchNode const* getCurrentRootNode() const {
        return currentRootNode;
    }

    // Locks backup-phase, i.e. only updates visits, but not Qvalues, e.g.
    // dp_uct would always backup the old future reward
    bool backupLock;

    // Backup lock won't apply at and beyond this depth.
    int maxLockDepth;

private:
    // Search nodes used in trials
    SearchNode* currentRootNode;
    SearchNode* chosenOutcome;

    // States used in trials
    std::vector<PDState> states;
    int stepsToGoInCurrentState;
    int stepsToGoInNextState;

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

    // This is used to make sure that at least one chance node is created
    // between two decision nodes (this is because it causes problems in the UCT
    // variants with the numberOfVisits counter of nodes that are both a parent
    // and a child node)
    bool transitionIsDeterministic;

    // Search engine that estimates Q-values for initialization of
    // decison node children
    SearchEngine* initializer;
    std::vector<double> initialQValues;

    // Counter for number of initialized decision nodes in the current
    // trial
    int initializedDecisionNodes;

    // Memory management (nodePool)
    int lastUsedNodePoolIndex;
    std::vector<SearchNode*> nodePool;

    // The timer used for timeout check
    Timer timer;

protected:    
    // Parameter
    THTS<SearchNode>::TerminationMethod terminationMethod;
    int maxNumberOfTrials;
    int numberOfNewDecisionNodesPerTrial;
    int maxNumberOfNodes;
    bool selectMostVisited;

    // Statistics
    int numberOfRuns;
    int cacheHits;
    int accumulatedNumberOfRemainingStepsInFirstSolvedRootState;
    bool firstSolvedFound;
    int accumulatedNumberOfTrialsInRootState;
    int accumulatedNumberOfSearchNodesInRootState;
    // Statistic variable to count skipped backups.
    size_t skippedBackups;

    // Tests which access private members
    friend class THTSTest;
};

/******************************************************************
                     Search Engine Creation
******************************************************************/

template <class SearchNode>
bool THTS<SearchNode>::setValueFromString(std::string& param,
        std::string& value) {
    if (param == "-T") {
        if (value == "TIME") {
            setTerminationMethod(THTS<SearchNode>::TIME);
            return true;
        } else if (value == "TRIALS") {
            setTerminationMethod(THTS<SearchNode>::NUMBER_OF_TRIALS);
            return true;
        } else if (value == "TIME_AND_TRIALS") {
            setTerminationMethod(THTS<SearchNode>::TIME_AND_NUMBER_OF_TRIALS);
            return true;
        } else {
            return false;
        }
    } else if (param == "-r") {
        setMaxNumberOfTrials(atoi(value.c_str()));
        return true;
    } else if (param == "-i") {
        setInitializer(SearchEngine::fromString(value));
        return true;
    } else if (param == "-ndn") {
        setNumberOfNewDecisionNodesPerTrial(atoi(value.c_str()));
        return true;
    } else if (param == "-mnn") {
        setMaxNumberOfNodes(atoi(value.c_str()));
        return true;
    } else if (param == "-mv") {
        setSelectMostVisited(atoi(value.c_str()));
        return true;
    }

    return SearchEngine::setValueFromString(param, value);
}

/******************************************************************
                 Search Engine Administration
******************************************************************/

template <class SearchNode>
void THTS<SearchNode>::disableCaching() {
    if (initializer) {
        initializer->disableCaching();
    }
    SearchEngine::disableCaching();
}

template <class SearchNode>
void THTS<SearchNode>::learn() {
    if (initializer) {
        initializer->learn();
    }
    std::cout << name << ": learning..." << std::endl;

    if (initializer->getMaxSearchDepth() == 0) {
        UniformEvaluationSearch* _initializer = new UniformEvaluationSearch();
        setInitializer(_initializer);
        std::cout << "Aborted initialization as search depth is too low!"
                  << std::endl;
    }
    std::cout << name << ": ...finished" << std::endl;
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
    PDState rootState(_rootState);
    // Adjust maximal search depth and set root state
    if (rootState.stepsToGo() > maxSearchDepth) {
        ignoredSteps = rootState.stepsToGo() - maxSearchDepth;
        maxSearchDepthForThisStep = maxSearchDepth;
        states[maxSearchDepthForThisStep].setTo(rootState);
        states[maxSearchDepthForThisStep].stepsToGo() =
            maxSearchDepthForThisStep;
    } else {
        ignoredSteps = 0;
        maxSearchDepthForThisStep = rootState.stepsToGo();
        states[maxSearchDepthForThisStep].setTo(rootState);
    }
    assert(states[maxSearchDepthForThisStep].stepsToGo() == maxSearchDepthForThisStep);

    stepsToGoInCurrentState = maxSearchDepthForThisStep;
    stepsToGoInNextState = maxSearchDepthForThisStep - 1;
    states[stepsToGoInNextState].reset(stepsToGoInNextState);

    // Reset step dependent counter
    currentTrial = 0;
    cacheHits = 0;

    // Reset search nodes and create root node
    resetNodePool();
    currentRootNode = getRootNode();

    outStream << name << ": Maximal search depth set to "
              << maxSearchDepthForThisStep << std::endl << std::endl;
}

template <class SearchNode>
inline void THTS<SearchNode>::initTrial() {
    // Reset states and steps-to-go counter
    stepsToGoInCurrentState = maxSearchDepthForThisStep;
    stepsToGoInNextState = maxSearchDepthForThisStep - 1;
    states[stepsToGoInNextState].reset(stepsToGoInNextState);

    // Reset trial dependent variables
    initializedDecisionNodes = 0;
    backupLock = false;
    maxLockDepth = remainingConsideredSteps();
}

template <class SearchNode>
inline void THTS<SearchNode>::initTrialStep() {
    --stepsToGoInCurrentState;
    --stepsToGoInNextState;
    states[stepsToGoInNextState].reset(stepsToGoInNextState);
}

/******************************************************************
                       Main Search Functions
******************************************************************/

template <class SearchNode>
bool THTS<SearchNode>::estimateBestActions(State const& _rootState,
                                           std::vector<int>& bestActions) {
    timer.reset();

    assert(bestActions.empty());

    // Init round (if this is the first call in a round)
    if (_rootState.stepsToGo() == SearchEngine::horizon) {
        initRound();
    }

    // Init step (this function is currently only called once per step) TODO:
    // maybe we should call initRound, initStep and printStats from "outside"
    // such that we can also use this as a heuristic without generating too much
    // output
    initStep(_rootState);

    // Check if there is an obviously optimal policy (as, e.g., in the last step
    // or in a reward lock)
    int uniquePolicyOpIndex = getUniquePolicy();
    if (uniquePolicyOpIndex != -1) {
        outStream << "Returning unique policy: ";
        SearchEngine::actionStates[uniquePolicyOpIndex].printCompact(outStream);
        outStream << std::endl << std::endl;
        bestActions.push_back(uniquePolicyOpIndex);
        currentRootNode = NULL;
        printStats(outStream, (_rootState.stepsToGo() == 1));
        return true;
    }

    // Start the main loop that starts trials until some termination criterion
    // is fullfilled
    while (moreTrials()) {
        // std::cout << "---------------------------------------------------------" << std::endl;
        // std::cout << "TRIAL " << (currentTrial+1) << std::endl;
        // std::cout << "---------------------------------------------------------" << std::endl;
        initTrial();
        visitDecisionNode(currentRootNode);
        ++currentTrial;
        // for(unsigned int i = 0; i < currentRootNode->children.size(); ++i) {
        //     if(currentRootNode->children[i]) {
        //         SearchEngine::actionStates[i].print(std::cout);
        //         std::cout << std::endl;
        //         currentRootNode->children[i]->print(std::cout, "  ");
        //     }
        // }
        // assert(currentTrial != 100);
    }

    recommendAction(bestActions);

    // Update statistics
    ++numberOfRuns;

    if (currentRootNode->isSolved() && !firstSolvedFound) {
        // TODO: This is the first root state that was solved, so everything
        // that could happen in the future is also solved. We should (at least
        // in this case) make sure that we keep the tree and simply follow the
        // optimal policy.
        firstSolvedFound = true;
        accumulatedNumberOfRemainingStepsInFirstSolvedRootState += _rootState.stepsToGo();
    }

    if (_rootState.stepsToGo() == SearchEngine::horizon) {
        accumulatedNumberOfTrialsInRootState += currentTrial;
        accumulatedNumberOfSearchNodesInRootState += lastUsedNodePoolIndex;
    }

    // Print statistics
    outStream << "Search time: " << timer << std::endl;
    printStats(outStream, (_rootState.stepsToGo() == 1));

    return !bestActions.empty();
}

template <class SearchNode>
void THTS<SearchNode>::recommendAction(std::vector<int>& bestActions) {
    double stateValue = -std::numeric_limits<double>::max();

    std::vector<SearchNode*> const& actNodes = currentRootNode->children;
    bool solvedChildExists = false;
    for (unsigned int actInd = 0; actInd < actNodes.size(); ++actInd) {
        if (actNodes[actInd] && actNodes[actInd]->isSolved()) {
            solvedChildExists = true;
            break;
        }
    }

    // Write best action indices to the result vector
    for (unsigned int actInd = 0; actInd < actNodes.size(); ++actInd) {
        if (actNodes[actInd]) {
            double reward = 0.0;
            if (selectMostVisited && !solvedChildExists) {
                // We use the number of visits as reward, which is OK if this is
                // the toplevel search engine but not if this is used as
                // initializer!
                reward = actNodes[actInd]->getNumberOfVisits();
            } else {
                reward = actNodes[actInd]->getExpectedRewardEstimate();
            }
            if (MathUtils::doubleIsGreater(reward, stateValue)) {
                stateValue = reward;
                bestActions.clear();
                bestActions.push_back(actInd);
            } else if (MathUtils::doubleIsEqual(reward, stateValue)) {
                bestActions.push_back(actInd);
            }
        }
    }
}

template <class SearchNode>
bool THTS<SearchNode>::moreTrials() {
    // Check memory constraints and solvedness
    if (currentRootNode->isSolved() ||
        (lastUsedNodePoolIndex >= maxNumberOfNodes)) {
        return false;
    }

    if (currentTrial == 0) {
        return true;
    }

    // Check selected termination criterion
    switch (terminationMethod) {
    case THTS<SearchNode>::TIME:
        if (MathUtils::doubleIsGreater(timer(), timeout)) {
            return false;
        }
        break;
    case THTS<SearchNode>::NUMBER_OF_TRIALS:
        if (currentTrial == maxNumberOfTrials) {
            return false;
        }
        break;
    case THTS<SearchNode>::TIME_AND_NUMBER_OF_TRIALS:
        if (MathUtils::doubleIsGreater(timer(), timeout) ||
            (currentTrial == maxNumberOfTrials)) {
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

    if (node != currentRootNode) {
        calcReward(states[stepsToGoInCurrentState], actions[currentActionIndex],
                reward);
        // std::cout << "Reward is " << reward << std::endl;

        if (stepsToGoInNextState == 1) {
            // This node is a leaf (the last action is optimally calculated in
            // the planning task)

            calcOptimalFinalReward(states[1], futureReward);
            // std::cout << "Final reward is " << futureReward << std::endl;
            backupDecisionNodeLeaf(node, reward, futureReward);
            return reward + futureReward;
        } else if (ProbabilisticSearchEngine::stateValueCache.find(states[stepsToGoInNextState]) !=
                   ProbabilisticSearchEngine::stateValueCache.end()) {
            // This state has already been solved before

            node->children.clear();
            futureReward =
                ProbabilisticSearchEngine::stateValueCache[states[stepsToGoInNextState]];
            // std::cout << "Cached reward is " << futureReward << std::endl;
            backupDecisionNodeLeaf(node, reward, futureReward);
            ++cacheHits;
            return reward + futureReward;
        }

        // Continue trial (i.e., set next state to be the current)
        initTrialStep();
    }

    // std::cout << std::endl << std::endl << "Current state is: " << std::endl;
    // states[stepsToGoInCurrentState].printCompact(std::cout);

    // Call initialization if necessary
    if (node->children.empty()) {
        initializeDecisionNode(node);
    }

    // Check if this is a reward lock (this is not checked before
    // initialization because we only compute it once and remember the
    // result in the nodes)
    if (node->isARewardLock()) {
        calcReward(states[stepsToGoInCurrentState], 0, futureReward);
        futureReward *= (ignoredSteps + stepsToGoInCurrentState);
        backupDecisionNodeLeaf(node, reward, futureReward);

        ++stepsToGoInCurrentState;
        return reward + futureReward;
    }

    // Check if we continue with this trial
    if (continueTrial(node)) {
        // Select the action that is simulated
        actions[currentActionIndex] = selectAction(node);
        assert(node->children[actions[currentActionIndex]]);
        assert(!node->children[actions[currentActionIndex]]->isSolved());

        // std::cout << "Chosen action is: ";
        // SearchEngine::actionStates[actions[currentActionIndex]].printCompact(std::cout);
        // std::cout << std::endl;

        // Sample successor state
        calcSuccessorState(states[stepsToGoInCurrentState], actions[currentActionIndex], states[stepsToGoInNextState]);

        // std::cout << "Sampled PDState is " << std::endl;
        // task->printPDState(std::cout, pdStates[stepsToGoInNextState]);

        // Start outcome selection with the first probabilistic variable
        chanceNodeVarIndex = 0;
        transitionIsDeterministic = true;

        // Continue trial with chance nodes
        futureReward = visitChanceNode(node->children[actions[currentActionIndex]]);
    } else {
        // We finish the trial
        actions[currentActionIndex] = -1;
    }

    // std::cout << "reward is " << reward << " and fut reward is " << futureReward << std::endl;

    // Backup this node
    backupDecisionNode(node, reward, futureReward);

    // If the backup function labeled the node as solved, we store the
    // result for the associated state in case we encounter it
    // somewhere else in the tree in the future
    if (node->isSolved()) {
        // std::cout << "solved a state with rem steps " << stepsToGoInCurrentState << " in trial " << currentTrial << std::endl;
        if (cachingEnabled &&
            ProbabilisticSearchEngine::stateValueCache.find(states[
                        stepsToGoInCurrentState
                    ]) == ProbabilisticSearchEngine::stateValueCache.end()) {
            ProbabilisticSearchEngine::stateValueCache[states[stepsToGoInCurrentState
                                                       ]] =
                node->getExpectedFutureRewardEstimate();
        }
    }

    ++stepsToGoInCurrentState;
    return reward + futureReward;
}

template <class SearchNode>
double THTS<SearchNode>::visitChanceNode(SearchNode* node) {
    if(chanceNodeVarIndex == State::numberOfProbabilisticStateFluents) {
        State::calcStateFluentHashKeys(states[stepsToGoInNextState]);
        State::calcStateHashKey(states[stepsToGoInNextState]);

        if(transitionIsDeterministic) {
            // This state transition is deterministic -> we are in a dummy
            // chance node, and need to continue with the single decision node
            // that is the child of this node
            if(node->children.empty()) {
                // The dummy chance node doesn't exist yet
                node->children.resize(1, NULL);

                SearchNode* dummyNode = getDummyNode();
                node->children[0] = dummyNode;
            }
            assert(node->children.size() == 1);
            double futureReward = visitDecisionNode(node->children[0]);
            backupChanceNode(node, futureReward);
            return futureReward;
        } else {
            return visitDecisionNode(node);
        }
    } else if(states[stepsToGoInNextState].probabilisticStateFluentAsPD(chanceNodeVarIndex).isDeterministic()) {
        states[stepsToGoInNextState].probabilisticStateFluent(chanceNodeVarIndex) = states[stepsToGoInNextState].probabilisticStateFluentAsPD(chanceNodeVarIndex).values[0];
        ++chanceNodeVarIndex;
        return visitChanceNode(node);
        } else {
        // Select outcome (and set the variable in next state accordingly)
        chosenOutcome = selectOutcome(node, states[stepsToGoInNextState], 
                                      chanceNodeVarIndex);

        // std::cout << "Chosen Outcome of variable " << chanceNodeVarIndex 
        //           << " is " << states[stepsToGoInNextState][chanceNodeVarIndex] 
        //           << std::endl;

        ++chanceNodeVarIndex;
        transitionIsDeterministic = false;

        double futureReward = visitChanceNode(chosenOutcome);

        backupChanceNode(node, futureReward);

        return futureReward;
    }
}

/******************************************************************
                     Initialization of Nodes
******************************************************************/

template <class SearchNode>
void THTS<SearchNode>::initializeDecisionNode(SearchNode* node) {
    if (isARewardLock(states[stepsToGoInCurrentState])) {
        node->setRewardLock(true);
        return;
    }
    node->children.resize(SearchEngine::numberOfActions, NULL);

    // Always backpropagate results up to newly initialized nodes
    if (maxLockDepth == maxSearchDepthForThisStep) {
        maxLockDepth = remainingConsideredSteps();
    }

    // std::cout << "initializing state: " << std::endl;
    // task->printState(std::cout, states[stepsToGoInCurrentState]);

    std::vector<int> actionsToExpand =
        getApplicableActions(states[stepsToGoInCurrentState]);

    initializer->estimateQValues(states[stepsToGoInCurrentState],
                                 actionsToExpand,
                                 initialQValues);

    for (unsigned int i = 0; i < node->children.size(); ++i) {
        if (actionsToExpand[i] == i) {
            // std::cout << "Initialized child ";
            // SearchEngine::actionStates[i].printCompact(std::cout);
            // std::cout << " with " << initialQValues[i] << std::endl;
            initializeDecisionNodeChild(node, i, initialQValues[i]);
        } //  else {
          //     std::cout << "Inapplicable: ";
          //     SearchEngine::actionStates[i].printCompact(std::cout);
          //     std::cout << std::endl;
          // }
    }

    if (node != currentRootNode) {
        ++initializedDecisionNodes;
    }
}

/******************************************************************
                      Root State Analysis
******************************************************************/

template <class SearchNode>
int THTS<SearchNode>::getUniquePolicy() {
    if (stepsToGoInCurrentState == 1) {
        outStream << "Returning the optimal last action!" << std::endl;
        return getOptimalFinalActionIndex(states[1]);
    }

    std::vector<int> actionsToExpand =
        getApplicableActions(states[stepsToGoInCurrentState]);

    if (isARewardLock(states[stepsToGoInCurrentState])) {
        outStream << "Current root state is a reward lock state!" << std::endl;
        states[stepsToGoInCurrentState].print(outStream);
        for (unsigned int i = 0; i < actionsToExpand.size(); ++i) {
            if (actionsToExpand[i] == i) {
                return i;
            }
        }

        assert(false);
    }

    std::vector<int> applicableActionIndices = getIndicesOfApplicableActions(states[stepsToGoInCurrentState]);
    assert(!applicableActionIndices.empty());

    if (applicableActionIndices.size() == 1) {
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

    if (res) {
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
    for (unsigned int i = 0; i < nodePool.size(); ++i) {
        if (nodePool[i]) {
            if (nodePool[i]->children.size() > 0) {
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
    if (initializer) {
        initializer->print(out);
    }
}

template <class SearchNode>
void THTS<SearchNode>::printStats(std::ostream& out,
                                  bool const& printRoundStats,
                                  std::string indent) const {
    SearchEngine::printStats(out, printRoundStats, indent);

    if (currentTrial > 0) {
        out << indent << "Performed trials: " << currentTrial << std::endl;
        out << indent << "Created SearchNodes: " << lastUsedNodePoolIndex <<
        std::endl;
        out << indent << "Cache Hits: " << cacheHits << std::endl;
        out << indent << "Skipped backups: " << skippedBackups << std::endl;
    }
    if (initializer) {
        out << "Initialization: " << std::endl;
        initializer->printStats(out, printRoundStats, indent + "  ");
    }

    if (currentRootNode) {
        out << std::endl << indent << "Root Node: " << std::endl;
        currentRootNode->print(out);
        out << std::endl << "Q-Value Estimates: " << std::endl;
        for (unsigned int i = 0; i < currentRootNode->children.size(); ++i) {
            if (currentRootNode->children[i]) {
                out << indent;
                SearchEngine::actionStates[i].printCompact(out);
                out << ": ";
                currentRootNode->children[i]->print(out);
            }
        }
    }

    if (printRoundStats) {
        out << std::endl << indent << "ROUND FINISHED" << std::endl;
        out << indent <<
        "Accumulated number of remaining steps in first solved root state: " <<
        accumulatedNumberOfRemainingStepsInFirstSolvedRootState << std::endl;
        out << indent << "Accumulated number of trials in root state: " <<
        accumulatedNumberOfTrialsInRootState << std::endl;
        out << indent <<
        "Accumulated number of search nodes in root state: " <<
        accumulatedNumberOfSearchNodesInRootState << std::endl;
    }
}

#endif
