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
// with the parameter "-i").

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

// UPDATE (08/2015): Even though all classes now use the same search node class
// (THTSSearchNode, see below), we keep this class templated for now since there
// might be a reason to use an entirely different kind of search node. However,
// it is also possible that inheritance from THTSSearchNode suffices all needs,
// in which case we should use THTSSearchNode in the THTS class (rather than the
// template SearchNode).

class THTSSearchNode {
public:
    THTSSearchNode(double const& _prob, int const& _remainingSteps) :
        children(),
        immediateReward(0.0),
        prob(_prob),
        remainingSteps(_remainingSteps),
        futureReward(-std::numeric_limits<double>::max()),
        numberOfVisits(0),
        solved(false) {}

    ~THTSSearchNode() {
        for (unsigned int i = 0; i < children.size(); ++i) {
            if (children[i]) {
                delete children[i];
            }
        }
    }

    void reset(double const& _prob, int const& _remainingSteps) {
        children.clear();
        immediateReward = 0.0;
        prob = _prob;
        remainingSteps = _remainingSteps;
        futureReward = -std::numeric_limits<double>::max();
        numberOfVisits = 0;
        solved = false;
    }

    double getExpectedRewardEstimate() const {
        return immediateReward + futureReward;
    }

    double getExpectedFutureRewardEstimate() const {
        return futureReward;
    }

    int const& getNumberOfVisits() const {
        return numberOfVisits;
    }

    bool isSolved() const {
        return solved;
    }

    void print(std::ostream& out, std::string indent = "") const {
        if (solved) {
            out << indent << "SOLVED with: " << getExpectedRewardEstimate() <<
            " (in " << numberOfVisits << " real visits)" << std::endl;
        } else {
            out << indent << getExpectedRewardEstimate() << " (in " <<
            numberOfVisits << " real visits)" << std::endl;
        }
    }

    std::vector<THTSSearchNode*> children;

    double immediateReward;
    double prob;
    int remainingSteps;
    
    double futureReward;
    int numberOfVisits;

    bool solved;
};

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

    virtual void setNumberOfInitialVisits(int _numberOfInitialVisits) {
        numberOfInitialVisits = _numberOfInitialVisits;
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
        nodePool.resize(maxNumberOfNodes + 20000, nullptr);
    }

    virtual void setSelectMostVisited(bool _selectMostVisited) {
        selectMostVisited = _selectMostVisited;
    }

    virtual void setHeuristicWeight(double _heuristicWeight) {
        heuristicWeight = _heuristicWeight;
    }

    // Used only for testing
    void setCurrentRootNode(SearchNode* node) {
        currentRootNode = node;
    }

    // Printer
    virtual void print(std::ostream& out);
    virtual void printStats(std::ostream& out,
                            bool const& printRoundStats,
                            std::string indent = "") const;

protected:
    THTS<SearchNode>(std::string _name) :
        ProbabilisticSearchEngine(_name),
        backupLock(false),
        maxLockDepth(0),
        currentRootNode(nullptr),
        chosenOutcome(nullptr),
        states(SearchEngine::horizon + 1),
        stepsToGoInCurrentState(SearchEngine::horizon),
        stepsToGoInNextState(SearchEngine::horizon - 1),
        appliedActionIndex(-1),
        trialReward(0.0),
        currentTrial(0),
        initializer(nullptr),
        initialQValues(SearchEngine::numberOfActions, 0.0),
        initializedDecisionNodes(0),
        lastUsedNodePoolIndex(0),
        terminationMethod(THTS<SearchNode>::TIME),
        maxNumberOfTrials(0),
        numberOfInitialVisits(5),
        numberOfNewDecisionNodesPerTrial(SearchEngine::horizon + 1),
        selectMostVisited(false),
        heuristicWeight(1.0),
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
    void visitDecisionNode(SearchNode* node);
    void visitChanceNode(SearchNode* node);
    void visitDummyChanceNode(SearchNode* node);

    // Initialization of different search phases
    void initRound();
    void initStep(State const& _rootState);
    void initTrial();
    void initTrialStep();

    // Action selection
    virtual int selectAction(SearchNode* node) = 0;

    // Outcome selection
    virtual SearchNode* selectOutcome(SearchNode* node, PDState& nextState,
                                      int const& varIndex, int const& lastProbVarIndex) = 0;

    // Trial length determinization
    virtual bool continueTrial(SearchNode* /*node*/) {
        return initializedDecisionNodes < numberOfNewDecisionNodesPerTrial;
    }

    // Initialization of nodes
    virtual void initializeDecisionNode(SearchNode* node);

    // Backup functions
    virtual void backupDecisionNodeLeaf(SearchNode* node,
                                        double const& futReward) = 0;
    virtual void backupDecisionNode(SearchNode* node,
                                    double const& accReward) = 0;
    virtual void backupChanceNode(SearchNode* node,
                                  double const& accReward) = 0;

    // Recommendation function
    virtual void recommendAction(std::vector<int>& bestActions);

    // Determines if the current state has been solved before or can be solved now
    bool currentStateIsSolved(SearchNode* node);

    // If the root state is a reward lock or has only one reasonable
    // action, noop or the only reasonable action is returned
    int getUniquePolicy();

    // Determine if the termination criterion is fullfilled or if we
    // want another trial
    bool moreTrials();

    // Memory management
    SearchNode* getRootNode();
    SearchNode* getDecisionNode(double const& _prob);
    SearchNode* getChanceNode(double const& _prob);

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

    // Currently simulated action
    int appliedActionIndex;

    // The accumulated reward that has been achieved in the current trial (the
    // rewards are accumulated in reverse order during the backup phase, such
    // that it reflects the future reward in each node when it is backed up)
    double trialReward;

    // Counter for the number of trials
    int currentTrial;

    // Max search depth for the current step
    int maxSearchDepthForThisStep;

    // The number of steps that are "cut off" if a limited search
    // depth is used
    int ignoredSteps;

    // Variable used to navigate through chance node layers
    int chanceNodeVarIndex;

    // Index of the last variable with non-deterministic outcome in the current transition
    int lastProbabilisticVarIndex;

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
    int numberOfInitialVisits;
    int numberOfNewDecisionNodesPerTrial;
    int maxNumberOfNodes;
    bool selectMostVisited;
    double heuristicWeight;

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
    } else if (param == "-iv") {
        setNumberOfInitialVisits(atoi(value.c_str()));
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
    } else if (param == "-hw") {
      setHeuristicWeight(atof(value.c_str()));
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
    trialReward = 0.0;
    backupLock = false;
    maxLockDepth = stepsToGoInCurrentState;
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
        currentRootNode = nullptr;
        printStats(outStream, (_rootState.stepsToGo() == 1));
        return true;
    }

    // Start the main loop that starts trials until some termination criterion
    // is fullfilled
    while (moreTrials()) {
        // std::cout << "---------------------------------------------------------" << std::endl;
        // std::cout << "TRIAL " << (currentTrial+1) << std::endl;
        // std::cout << "---------------------------------------------------------" << std::endl;
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
void THTS<SearchNode>::visitDecisionNode(SearchNode* node) {
    if (node == currentRootNode) {
        initTrial();
    } else {
        // Continue trial (i.e., set next state to be the current)
        initTrialStep();

        // Check if there is a "special" reason to stop this trial (currently,
        // this is the case if the state value of the current state is cached,
        // if it is a reward lock or if there is only one step left).
        if (currentStateIsSolved(node)) {
            return;
        }
    }

    // Initialize node if it wasn't visited before
    if (node->children.empty()) {
        initializeDecisionNode(node);
    }

    // std::cout << std::endl << std::endl << "Current state is: " << std::endl;
    // states[stepsToGoInCurrentState].printCompact(std::cout);
    // std::cout << "Reward is " << node->immediateReward << std::endl;

    // Determine if we continue with this trial
    if (continueTrial(node)) {
        // Select the action that is simulated
        appliedActionIndex = selectAction(node);
        assert(node->children[appliedActionIndex]);
        assert(!node->children[appliedActionIndex]->isSolved());

        // std::cout << "Chosen action is: ";
        // SearchEngine::actionStates[appliedActionIndex].printCompact(std::cout);
        // std::cout << std::endl;

        // Sample successor state
        calcSuccessorState(states[stepsToGoInCurrentState], appliedActionIndex, states[stepsToGoInNextState]);

        // std::cout << "Sampled PDState is " << std::endl;
        // states[stepsToGoInNextState].printPDStateCompact(std::cout);
        // std::cout << std::endl;

        lastProbabilisticVarIndex = - 1;
        for (unsigned int i = 0; i < State::numberOfProbabilisticStateFluents; ++i) {
            if (states[stepsToGoInNextState].probabilisticStateFluentAsPD(i).isDeterministic()) {
                states[stepsToGoInNextState].probabilisticStateFluent(i) = 
                    states[stepsToGoInNextState].probabilisticStateFluentAsPD(i).values[0];
            } else {
                lastProbabilisticVarIndex = i;
            }
        }

        // Start outcome selection with the first probabilistic variable
        chanceNodeVarIndex = 0;

        // Continue trial with chance nodes
        if (lastProbabilisticVarIndex < 0) {
            visitDummyChanceNode(node->children[appliedActionIndex]);
        } else {
            visitChanceNode(node->children[appliedActionIndex]);
        }

        // Backup this node
        backupDecisionNode(node, trialReward);
        trialReward += node->immediateReward;

        // If the backup function labeled the node as solved, we store the
        // result for the associated state in case we encounter it somewhere
        // else in the tree in the future
        if (node->isSolved()) {
            if (cachingEnabled &&
                ProbabilisticSearchEngine::stateValueCache.find(states[node->remainingSteps]) ==
                ProbabilisticSearchEngine::stateValueCache.end()) {
                ProbabilisticSearchEngine::stateValueCache[states[node->remainingSteps]] =
                    node->getExpectedFutureRewardEstimate();
            }
        }
    } else {
        // The trial is finished
        trialReward = node->getExpectedRewardEstimate();
    }
}

template <class SearchNode>
bool THTS<SearchNode>::currentStateIsSolved(SearchNode* node) {
    if (stepsToGoInCurrentState == 1) {
        // This node is a leaf (there is still a last decision, though, but that
        // is taken care of by calcOptimalFinalReward)

        calcOptimalFinalReward(states[1], trialReward);
        backupDecisionNodeLeaf(node, trialReward);
        trialReward += node->immediateReward;

        return true;
    } else if (ProbabilisticSearchEngine::stateValueCache.find(states[stepsToGoInCurrentState]) !=
               ProbabilisticSearchEngine::stateValueCache.end()) {
        // This state has already been solved before

        node->children.clear();
        trialReward =
            ProbabilisticSearchEngine::stateValueCache[states[stepsToGoInCurrentState]];
        backupDecisionNodeLeaf(node, trialReward);
        trialReward += node->immediateReward;

        ++cacheHits;
        return true;
    } else if (node->children.empty() && isARewardLock(states[stepsToGoInCurrentState])) {
        // This state is a reward lock, i.e. a goal or a state that is such that
        // no matter which action is applied we'll always get the same reward
        
        calcReward(states[stepsToGoInCurrentState], 0, trialReward);
        trialReward *= stepsToGoInCurrentState;
        backupDecisionNodeLeaf(node, trialReward);
        trialReward += node->immediateReward;

        if (cachingEnabled) {
            assert(ProbabilisticSearchEngine::stateValueCache.find(states[stepsToGoInCurrentState]) ==
                   ProbabilisticSearchEngine::stateValueCache.end());
            ProbabilisticSearchEngine::stateValueCache[states[stepsToGoInCurrentState]] =
                node->getExpectedFutureRewardEstimate();
        }
        return true;
    }
    return false;
}

template <class SearchNode>
void THTS<SearchNode>::visitChanceNode(SearchNode* node) {
    while (states[stepsToGoInNextState].probabilisticStateFluentAsPD(chanceNodeVarIndex).isDeterministic()) {
        ++chanceNodeVarIndex;
    }

    chosenOutcome = selectOutcome(node, states[stepsToGoInNextState],
                                  chanceNodeVarIndex, lastProbabilisticVarIndex);
    
    if (chanceNodeVarIndex == lastProbabilisticVarIndex) {
        State::calcStateFluentHashKeys(states[stepsToGoInNextState]);
        State::calcStateHashKey(states[stepsToGoInNextState]);

        visitDecisionNode(chosenOutcome);
    } else {
        ++chanceNodeVarIndex;
        visitChanceNode(chosenOutcome);
    }
    backupChanceNode(node, trialReward);
}

template <class SearchNode>
void THTS<SearchNode>::visitDummyChanceNode(SearchNode* node) {
    State::calcStateFluentHashKeys(states[stepsToGoInNextState]);
    State::calcStateHashKey(states[stepsToGoInNextState]);

    if(node->children.empty()) {
        node->children.resize(1, nullptr);
        node->children[0] = getDecisionNode(1.0);
    }
    assert(node->children.size() == 1);

    visitDecisionNode(node->children[0]);
    backupChanceNode(node, trialReward);
}

/******************************************************************
                     Initialization of Nodes
******************************************************************/

template <class SearchNode>
void THTS<SearchNode>::initializeDecisionNode(SearchNode* node) {
    node->children.resize(SearchEngine::numberOfActions, nullptr);

    // Always backpropagate results up to newly initialized nodes
    if (maxLockDepth == maxSearchDepthForThisStep) {
        maxLockDepth = stepsToGoInCurrentState;
    }

    // std::cout << "initializing state: " << std::endl;
    // task->printState(std::cout, states[stepsToGoInCurrentState]);

    std::vector<int> actionsToExpand =
        getApplicableActions(states[stepsToGoInCurrentState]);

    initializer->estimateQValues(states[stepsToGoInCurrentState],
                                 actionsToExpand,
                                 initialQValues);

    for (unsigned int index = 0; index < node->children.size(); ++index) {
        if (actionsToExpand[index] == index) {
            node->children[index] = getChanceNode(1.0);
            node->children[index]->futureReward =
                heuristicWeight * (double)stepsToGoInCurrentState * initialQValues[index];
            node->children[index]->numberOfVisits = numberOfInitialVisits;

            node->numberOfVisits += numberOfInitialVisits;
            node->futureReward =
                std::max(node->futureReward, node->children[index]->futureReward);
        }
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
SearchNode* THTS<SearchNode>::getRootNode() {
    for (SearchNode* node : nodePool) {
        if (node) {
            if(!node->children.empty()) {
            	std::vector<SearchNode*> tmp;
            	node->children.swap(tmp);
            }
	} else {
            break;
        }
    }

    SearchNode* res = nodePool[0];

    if (res) {
        res->reset(1.0, stepsToGoInCurrentState);
    } else {
        res = new SearchNode(1.0, stepsToGoInCurrentState);
        nodePool[0] = res;
    }
    res->immediateReward = 0.0;

    lastUsedNodePoolIndex = 1;
    return res;
}

template <class SearchNode>
SearchNode* THTS<SearchNode>::getDecisionNode(double const& prob) {
    assert(lastUsedNodePoolIndex < nodePool.size());

    SearchNode* res = nodePool[lastUsedNodePoolIndex];

    if (res) {
        res->reset(prob, stepsToGoInNextState);
    } else {
        res = new SearchNode(prob, stepsToGoInNextState);
        nodePool[lastUsedNodePoolIndex] = res;
    }
    calcReward(states[stepsToGoInCurrentState], appliedActionIndex, res->immediateReward);

    ++lastUsedNodePoolIndex;
    return res;
}

template <class SearchNode>
SearchNode* THTS<SearchNode>::getChanceNode(double const& prob) {
    assert(lastUsedNodePoolIndex < nodePool.size());

    SearchNode* res = nodePool[lastUsedNodePoolIndex];

    if (res) {
        res->reset(prob, stepsToGoInCurrentState);
    } else {
        res = new SearchNode(prob, stepsToGoInCurrentState);
        nodePool[lastUsedNodePoolIndex] = res;
    }

    ++lastUsedNodePoolIndex;
    return res;
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
