#ifndef THTS_H
#define THTS_H

#include "search_engine.h"

#include "utils/timer.h"

#include <sstream>

class ActionSelection;
class OutcomeSelection;
class BackupFunction;
class Initializer;

// THTS, Trial-based Heuristic Tree Search, is the implementation of the
// abstract framework described in the ICAPS 2013 paper "Trial-based Heuristic
// Tree Search for Finite Horizon MDPs" (Keller & Helmert). The described
// ingredients are implemented in four classes (1-4) or as functions in the
// THTS class (5-6)

// 1. ActionSelection

// 2. Outcome Selection

// 3. BackupFunction

// 4. Initializer

// 5. continueTrial()

// 6. recommendAction()

struct SearchNode {
    SearchNode(double const& _prob, int const& _stepsToGo) :
        children(),
        immediateReward(0.0),
        prob(_prob),
        stepsToGo(_stepsToGo),
        futureReward(-std::numeric_limits<double>::max()),
        numberOfVisits(0),
        initialized(false),
        solved(false) {}

    ~SearchNode() {
        for (unsigned int i = 0; i < children.size(); ++i) {
            if (children[i]) {
                delete children[i];
            }
        }
    }

    void reset(double const& _prob, int const& _stepsToGo) {
        children.clear();
        immediateReward = 0.0;
        prob = _prob;
        stepsToGo = _stepsToGo;
        futureReward = -std::numeric_limits<double>::max();
        numberOfVisits = 0;
        initialized = false;
        solved = false;
    }

    double getExpectedRewardEstimate() const {
        return immediateReward + futureReward;
    }

    double getExpectedFutureRewardEstimate() const {
        return futureReward;
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

    std::vector<SearchNode*> children;

    double immediateReward;
    double prob;
    int stepsToGo;
    
    double futureReward;
    int numberOfVisits;

    // This is used in two ways: in decision nodes, it is true if all children
    // are initialized; and in chance nodes that represent an action (i.e., in
    // children of decision nodes), it is true if an initial value has been
    // assigned to the node.
    bool initialized;

    // A node is solved if futureReward is equal to the true future reward
    bool solved;
};

class THTS : public ProbabilisticSearchEngine {
public:
    enum TerminationMethod {
        TIME, //stop after timeout sec
        NUMBER_OF_TRIALS, //stop after maxNumberOfTrials trials
        TIME_AND_NUMBER_OF_TRIALS //stop after timeout sec or maxNumberOfTrials trials, whichever comes first
    };

    THTS(std::string _name) :
        ProbabilisticSearchEngine(_name),
        actionSelection(nullptr),
        outcomeSelection(nullptr),
        backupFunction(nullptr),
        initializer(nullptr),
        currentRootNode(nullptr),
        chosenOutcome(nullptr),
        tipNodeOfTrial(nullptr),
        states(SearchEngine::horizon + 1),
        stepsToGoInCurrentState(SearchEngine::horizon),
        stepsToGoInNextState(SearchEngine::horizon - 1),
        appliedActionIndex(-1),
        trialReward(0.0),
        currentTrial(0),
        initializedDecisionNodes(0),
        lastUsedNodePoolIndex(0),
        terminationMethod(THTS::TIME),
        maxNumberOfTrials(0),
        numberOfNewDecisionNodesPerTrial(1),
        selectMostVisited(false),
        numberOfRuns(0),
        cacheHits(0),
        accumulatedNumberOfStepsToGoInFirstSolvedRootState(0),
        firstSolvedFound(false),
        accumulatedNumberOfTrialsInRootState(0),
        accumulatedNumberOfSearchNodesInRootState(0) {
        setMaxNumberOfNodes(24000000);
        setTimeout(1.0);
    }

    // Parameter setter
    virtual bool setValueFromString(std::string& param, std::string& value);

    // Disables caching because memory becomes sparse.
    void disableCaching();

    // Learns parameter values from a random training set.
    virtual void learn();

    // Start the search engine as main search engine
    void estimateBestActions(State const& _rootState,
                             std::vector<int>& bestActions) override;

    // Start the search engine to estimate the Q-value of a single action
    void estimateQValue(State const& /*state*/, int /*actionIndex*/,
                        double& /*qValue*/) override {
        assert(false);
    }
    
    // Start the search engine to estimate the Q-values of all applicable
    // actions
    void estimateQValues(State const& /*state*/,
                         std::vector<int> const& /*actionsToExpand*/,
                         std::vector<double>& /*qValues*/) override {
        assert(false);
    }

    // Parameter setter
    virtual void setActionSelection(ActionSelection* _actionSelection) {
        actionSelection = _actionSelection;
    }

    virtual void setOutcomeSelection(OutcomeSelection* _outcomeSelection) {
        outcomeSelection = _outcomeSelection;
    }

    virtual void setBackupFunction(BackupFunction* _backupFunction) {
        backupFunction = _backupFunction;
    }
    
    virtual void setMaxSearchDepth(int _maxSearchDepth);
    virtual void setTerminationMethod(
            THTS::TerminationMethod _terminationMethod) {
        terminationMethod = _terminationMethod;
    }

    virtual void setMaxNumberOfTrials(int _maxNumberOfTrials) {
        maxNumberOfTrials = _maxNumberOfTrials;
    }

    virtual void setInitializer(Initializer* _initializer) {
        assert(!initializer);
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
        nodePool.resize(maxNumberOfNodes + 20000, nullptr);
    }

    virtual void setSelectMostVisited(bool _selectMostVisited) {
        selectMostVisited = _selectMostVisited;
    }

    // Used only for testing
    void setCurrentRootNode(SearchNode* node) {
        currentRootNode = node;
    }

    // Getter methods for classes that implement ingredients
    SearchNode* getRootNode();
    SearchNode* getDecisionNode(double const& _prob);
    SearchNode* getChanceNode(double const& _prob);

    SearchNode const* getCurrentRootNode() const {
        return currentRootNode;
    }

    SearchNode const* getTipNodeOfTrial() const {
        return tipNodeOfTrial;
    }

    // Print
    virtual void printStats(std::ostream& out,
                            bool const& printRoundStats,
                            std::string indent = "") const override;

    // Stream for nicer (and better timed) printing
    mutable std::stringstream outStream;

protected:
    // Main search functions
    void visitDecisionNode(SearchNode* node);
    void visitChanceNode(SearchNode* node);
    void visitDummyChanceNode(SearchNode* node);

    // Initialization of different search phases
    void initRound();
    void initStep(State const& _rootState);
    void initTrial();
    void initTrialStep();

    // Trial length determinization
    virtual bool continueTrial(SearchNode* /*node*/) {
        return initializedDecisionNodes < numberOfNewDecisionNodesPerTrial;
    }

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

private:
    // Ingredients that are implemented externally
    ActionSelection* actionSelection;
    OutcomeSelection* outcomeSelection;
    BackupFunction* backupFunction;
    Initializer* initializer;

    // Search nodes used in trials
    SearchNode* currentRootNode;
    SearchNode* chosenOutcome;

    // The tip node of a trial is the first node that is encountered that
    // requires initialization of a child
    SearchNode* tipNodeOfTrial;

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
    THTS::TerminationMethod terminationMethod;
    int maxNumberOfTrials;
    int numberOfNewDecisionNodesPerTrial;
    int maxNumberOfNodes;
    bool selectMostVisited;

    // Statistics
    int numberOfRuns;
    int cacheHits;
    int accumulatedNumberOfStepsToGoInFirstSolvedRootState;
    bool firstSolvedFound;
    int accumulatedNumberOfTrialsInRootState;
    int accumulatedNumberOfSearchNodesInRootState;

    // Tests which access private members
    friend class THTSTest;
    friend class BFSTestSearch;
    friend class MCUCTTestSearch;
    friend class UCTBaseTestSearch;
};

#endif
