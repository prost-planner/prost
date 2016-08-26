#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

// This is the abstract interface all search engines are based on. It implements
// a basic method to estimate best actions and state values by calling the pure
// virtual function estimateQValues. Furthermore, it provides all the methods
// that are necessary to compute state transitions (e.g., applicable actions,
// rewards, successor states) and provides support for concepts that are common
// among all search engines like caching or learning. There are also two derived
// classes in this file: ProbabilisticSearchEngine and
// DeterministicSearchEngine. These implement the state transition functions
// correspondingly.

#include "evaluatables.h"

#include <fdd.h>

class SearchEngine {
public:
    enum FinalRewardCalculationMethod {
        NOOP,
        FIRST_APPLICABLE,
        BEST_OF_CANDIDATE_SET
    };

    /*****************************************************************
                           Search engine creation
    *****************************************************************/

    virtual ~SearchEngine() {}

    // Create a SearchEngine
    static SearchEngine* fromString(std::string& desc);

    // Set parameters from command line
    virtual bool setValueFromString(std::string& param, std::string& value);

    // This is called when caching is disabled because memory becomes sparse.
    // Overwrite this if your search engine uses another component that caches
    // stuff and make sure caching is disabled everywhere.
    virtual void disableCaching() {
        cachingEnabled = false;
    }

    // TODO: For now, this is only here to set the timeout from ProstPlanner
    // (necessary for IPPC 2014). Generally, I'd like a TerminationManager class
    // that administrates termination criteria for each kind of search engine.
    virtual void setTimeout(double _timeout) {
        timeout = _timeout;
    }

    virtual void setUseRewardLockDetection(bool _useRewardLockDetection) {
        useRewardLockDetection = _useRewardLockDetection;
    }

    virtual void setCacheRewardLocks(bool _cacheRewardLocks) {
        cacheRewardLocks = _cacheRewardLocks;
    }

    bool usesBDDs() const {
        return useRewardLockDetection && cacheRewardLocks;
    }

protected:
    SearchEngine(std::string _name)
        : name(_name),
          cachingEnabled(true),
          useRewardLockDetection(SearchEngine::rewardLockDetected),
          cacheRewardLocks(true),
          maxSearchDepth(horizon),
          timeout(1.0) {}

    /*****************************************************************
                         Main search functions
    *****************************************************************/
public:
    // This is called initially to learn parameter values from a training set
    virtual void learn() {}

    // Start the search engine to calculate best actions
    virtual void estimateBestActions(State const& _rootState,
                                     std::vector<int>& bestActions);

    // Start the search engine for state value estimation
    virtual void estimateStateValue(State const& _rootState,
                                    double& stateValue);

    // Start the search engine to estimate the Q-value of a single action
    virtual void estimateQValue(State const& state, int actionIndex,
                                double& qValue) = 0;

    // Start the search engine to estimate the Q-values of all applicable
    // actions
    virtual void estimateQValues(State const& _rootState,
                                 std::vector<int> const& actionsToExpand,
                                 std::vector<double>& qValues) = 0;

    // Methods for action applicability and pruning
    virtual std::vector<int> getApplicableActions(State const& state) const = 0;

    std::vector<int> getIndicesOfApplicableActions(State const& state) const {
        std::vector<int> applicableActions = getApplicableActions(state);
        std::vector<int> result;
        for (size_t index = 0; index < applicableActions.size(); ++index) {
            if (applicableActions[index] == index) {
                result.push_back(index);
            }
        }
        return result;
    }

    /*****************************************************************
                        Calculation of reward
    *****************************************************************/

protected:
    // Calculate the reward (since the reward must be deteriministic, this is
    // identical for probabilistic and deterministic search engines)
    void calcReward(State const& current, int const& actionIndex,
                    double& reward) const {
        rewardCPF->evaluate(reward, current, actionStates[actionIndex]);
    }

    // As we are currently assuming that the reward is independent of the
    // successor state the (optimal) last reward can be calculated by applying
    // all applicable actions and returning the highest reward
    void calcOptimalFinalReward(State const& current, double& reward) const {
        switch (finalRewardCalculationMethod) {
        case NOOP:
            calcReward(current, 0, reward);
            break;
        case FIRST_APPLICABLE:
            calcOptimalFinalRewardWithFirstApplicableAction(current, reward);
            break;
        case BEST_OF_CANDIDATE_SET:
            calcOptimalFinalRewardAsBestOfCandidateSet(current, reward);
            break;
        }
    }

    // Return the index of the optimal last action
    int getOptimalFinalActionIndex(State const& current) const;

private:
    // Methods to calculate the final reward
    void calcOptimalFinalRewardWithFirstApplicableAction(State const& current,
                                                         double& reward) const;
    void calcOptimalFinalRewardAsBestOfCandidateSet(State const& current,
                                                    double& reward) const;

    /*****************************************************************
                 Calculation of applicable actions
    *****************************************************************/

protected:
    bool actionIsApplicable(ActionState const& action,
                            State const& current) const {
        double res = 0.0;
        for (DeterministicEvaluatable* precond : action.actionPreconditions) {
            precond->evaluate(res, current, action);
            if (MathUtils::doubleIsEqual(res, 0.0)) {
                return false;
            }
        }
        return true;
    }

    /*****************************************************************
                                 Parameter
    *****************************************************************/

public:
    // Don't confuse setCachingEnabled with the disabling of caching during a
    // run: this method is only called if the planner parameter enables or
    // disables caching
    virtual void setCachingEnabled(bool _cachingEnabled) {
        cachingEnabled = _cachingEnabled;
    }

    virtual bool const& cachingIsEnabled() const {
        return cachingEnabled;
    }

    virtual void setMaxSearchDepth(int _maxSearchDepth) {
        maxSearchDepth = _maxSearchDepth;
    }

    virtual int const& getMaxSearchDepth() const {
        return maxSearchDepth;
    }

    /*****************************************************************
                             Member variables
    *****************************************************************/

public:
    // The name of this task (this is equivalent to the instance name)
    static std::string taskName;

    // Random set of reachable states (these are used for learning)
    static std::vector<State> trainingSet;

    // Action fluents and action states
    static std::vector<ActionFluent*> actionFluents;
    static std::vector<ActionState> actionStates;

    // State fluents
    static std::vector<StateFluent*> stateFluents;

    // Transition functions of state fluents
    static std::vector<Evaluatable*> allCPFs;
    static std::vector<DeterministicCPF*> deterministicCPFs;
    static std::vector<ProbabilisticCPF*> probabilisticCPFs;

    // Determinized transition functions of probabilistic state fluents
    static std::vector<DeterministicCPF*> determinizedCPFs;

    // The reward formula
    static RewardFunction* rewardCPF;

    // The action preconditions
    static std::vector<DeterministicEvaluatable*> actionPreconditions;

    // Is true if this planning task is deterministic
    static bool taskIsDeterministic;

    // The initial state (the one given in the problem description,
    // this is not updated)
    static State initialState;

    // The problem horizon
    static int horizon;

    // The problem's discount factor (TODO: This is ignored at the moment)
    static double discountFactor;

    // The number of actions (this is equal to actionStates.size())
    static int numberOfActions;

    // Since the reward is independent from the successor state, we can
    // calculate the final reward as the maximum of applying all actions in the
    // current state. Since the set of actions that could maximize the final
    // reward can be a subset of all actions, we distinguish between several
    // different methods to speed up this calculation.
    static FinalRewardCalculationMethod finalRewardCalculationMethod;
    static std::vector<int> candidatesForOptimalFinalAction;

    // Is true if applicable actions should be cached
    static bool cacheApplicableActions;

    // Is true if a reward lock was detected in the training phase
    static bool rewardLockDetected;

    // The index of this action is used to check if a state is a goal
    static int goalTestActionIndex;

    // The BDDs where dead ends and goals are cached
    static bdd cachedDeadEnds;
    static bdd cachedGoals;

    typedef std::unordered_map<State, double, State::HashWithRemSteps,
                               State::EqualWithRemSteps>
        StateValueHashMap;
    typedef std::unordered_map<State, std::vector<int>,
                               State::HashWithoutRemSteps,
                               State::EqualWithoutRemSteps>
        ActionHashMap;

protected:
    // Used for debug output only
    std::string name;

    // Parameter
    bool cachingEnabled;
    bool useRewardLockDetection;
    bool cacheRewardLocks;
    int maxSearchDepth;
    double timeout;

    /*****************************************************************
                           Printing and statistics
    *****************************************************************/

public:
    // Reset statistic variables
    virtual void resetStats() {}

    // Print
    virtual void printStats(std::ostream& out, bool const& printRoundStats,
                            std::string indent = "") const;

    static void printDeadEndBDD() {
        bdd_printdot(cachedDeadEnds);
    }

    static void printGoalBDD() {
        bdd_printdot(cachedGoals);
    }

    // Print task
    static void printTask(std::ostream& out);
    static void printEvaluatableInDetail(std::ostream& out, Evaluatable* eval);
    static void printDeterministicCPFInDetail(std::ostream& out,
                                              int const& index);
    static void printProbabilisticCPFInDetail(std::ostream& out,
                                              int const& index);
    static void printRewardCPFInDetail(std::ostream& out);
    static void printActionPreconditionInDetail(std::ostream& out,
                                                int const& index);
};

/*****************************************************************
                   PROBABILISTIC SEARCH ENGINE
*****************************************************************/

class ProbabilisticSearchEngine : public SearchEngine {
public:
    ProbabilisticSearchEngine(std::string _name) : SearchEngine(_name) {}

    // Is true if unreasonable actions where detected during learning
    static bool hasUnreasonableActions;

    // Cache for state values of solved states
    static StateValueHashMap stateValueCache;

    // Cache for applicable reasonable actions
    static ActionHashMap applicableActionsCache;

    /*****************************************************************
                 Calculation of applicable actions
    *****************************************************************/

    // Returns a vector ("res") that encodes applicable and reasonable actions.
    // If res[i] = i, the action with index i is applicable, and if res[i] = -1
    // it is not. Otherwise, the action with index i is unreasonable as the
    // action with index res[i] leads to the same distribution over successor
    // states (this is only checked if pruneUnreasonableActions is true).
    std::vector<int> getApplicableActions(State const& state) const {
        std::vector<int> res(numberOfActions, 0);

        ActionHashMap::iterator it = applicableActionsCache.find(state);
        if (it != applicableActionsCache.end()) {
            assert(it->second.size() == res.size());
            for (size_t i = 0; i < res.size(); ++i) {
                res[i] = it->second[i];
            }
        } else {
            if (hasUnreasonableActions) {
                std::map<PDState, int, PDState::PDStateCompare> childStates;

                for (size_t index = 0; index < numberOfActions; ++index) {
                    if (actionIsApplicable(actionStates[index], state)) {
                        // This action is applicable
                        PDState nxt(state.stepsToGo() - 1);
                        calcSuccessorState(state, index, nxt);

                        if (childStates.find(nxt) == childStates.end()) {
                            // This action is reasonable
                            childStates[nxt] = index;
                            res[index] = index;
                        } else {
                            // This action is not reasonable
                            res[index] = childStates[nxt];
                        }
                    } else {
                        // This action is not appicable
                        res[index] = -1;
                    }
                }
            } else {
                for (size_t index = 0; index < numberOfActions; ++index) {
                    if (actionIsApplicable(actionStates[index], state)) {
                        res[index] = index;
                    } else {
                        res[index] = -1;
                    }
                }
            }

            if (cacheApplicableActions) {
                applicableActionsCache[state] = res;
            }
        }

        return res;
    }

protected:
    /*****************************************************************
                    Calculation of state transition
    *****************************************************************/

    // Apply action 'actionIndex' to 'current', resulting in 'next'
    void calcSuccessorState(State const& current, int const& actionIndex,
                            PDState& next) const {
        for (int index = 0; index < State::numberOfDeterministicStateFluents;
             ++index) {
            deterministicCPFs[index]->evaluate(
                next.deterministicStateFluent(index), current,
                actionStates[actionIndex]);
        }

        for (int index = 0; index < State::numberOfProbabilisticStateFluents;
             ++index) {
            probabilisticCPFs[index]->evaluate(
                next.probabilisticStateFluentAsPD(index), current,
                actionStates[actionIndex]);
        }
    }

    /*****************************************************************
                 Calculation of Kleene state transition
    *****************************************************************/

    // Calculate successor in Kleene logic
    void calcKleeneSuccessor(KleeneState const& current, int const& actionIndex,
                             KleeneState& next) const {
        for (size_t i = 0; i < KleeneState::stateSize; ++i) {
            allCPFs[i]->evaluateToKleene(next[i], current,
                                         actionStates[actionIndex]);
        }
    }

    // Calulate the reward in Kleene logic
    void calcKleeneReward(KleeneState const& current, int const& actionIndex,
                          std::set<double>& reward) const {
        rewardCPF->evaluateToKleene(reward, current, actionStates[actionIndex]);
    }

    /*****************************************************************
                           Reward lock detection
    *****************************************************************/

    // Checks if current is a reward lock (actually, currently this checks only
    // if it is a dead end or a goal, i.e., a reward lock with minimal or
    // maximal reward).
    bool isARewardLock(State const& current) const;

private:
    // Methods for reward lock detection
    bool checkDeadEnd(KleeneState const& state) const;
    bool checkGoal(KleeneState const& state) const;

    // BDD related methods
    bdd stateToBDD(KleeneState const& state) const;
    bdd stateToBDD(State const& state) const;
    bool BDDIncludes(bdd BDD, KleeneState const& state) const;
};

/*****************************************************************
                   DETERMINISTIC SEARCH ENGINE
*****************************************************************/

class DeterministicSearchEngine : public SearchEngine {
public:
    DeterministicSearchEngine(std::string _name) : SearchEngine(_name) {}

    // Is true if unreasonable actions where detected during learning
    static bool hasUnreasonableActions;

    // Cache for state values of solved states
    static StateValueHashMap stateValueCache;

    // Cache for applicable reasonable actions
    static ActionHashMap applicableActionsCache;

protected:
    /*****************************************************************
                    Calculation of state transition
    *****************************************************************/

    // Apply action 'actionIndex' in the determinization to 'current', get state
    // 'next' and yield reward 'reward'
    void calcStateTransition(State const& current, int const& actionIndex,
                             State& next, double& reward) const {
        calcSuccessorState(current, actionIndex, next);
        calcReward(current, actionIndex, reward);
    }

    // Apply action 'actionIndex' in the determinization to 'current', resulting
    // in 'next'.
    void calcSuccessorState(State const& current, int const& actionIndex,
                            State& next) const {
        for (size_t index = 0; index < State::numberOfDeterministicStateFluents;
             ++index) {
            deterministicCPFs[index]->evaluate(
                next.deterministicStateFluent(index), current,
                actionStates[actionIndex]);
        }

        for (size_t index = 0; index < State::numberOfProbabilisticStateFluents;
             ++index) {
            determinizedCPFs[index]->evaluate(
                next.probabilisticStateFluent(index), current,
                actionStates[actionIndex]);
        }

        State::calcStateFluentHashKeys(next);
        State::calcStateHashKey(next);
    }

    /*****************************************************************
                 Calculation of applicable actions
    *****************************************************************/

    // Returns a vector ("res") that encodes applicable and reasonable actions.
    // If res[i] = i, the action with index i is applicable, and if res[i] = -1
    // it is not. Otherwise, the action with index i is unreasonable as the
    // action with index res[i] leads to the same distribution over successor
    // states (this is only checked if pruneUnreasonableActions is true).
    std::vector<int> getApplicableActions(State const& state) const {
        std::vector<int> res(numberOfActions, 0);

        ActionHashMap::iterator it = applicableActionsCache.find(state);
        if (it != applicableActionsCache.end()) {
            assert(it->second.size() == res.size());
            for (size_t i = 0; i < res.size(); ++i) {
                res[i] = it->second[i];
            }
        } else {
            if (hasUnreasonableActions) {
                std::map<State, int, State::CompareIgnoringStepsToGo>
                    childStates;

                for (size_t actionIndex = 0; actionIndex < numberOfActions;
                     ++actionIndex) {
                    if (actionIsApplicable(actionStates[actionIndex], state)) {
                        // This action is applicable
                        State nxt;
                        calcSuccessorState(state, actionIndex, nxt);
                        State::calcStateHashKey(nxt);

                        if (childStates.find(nxt) == childStates.end()) {
                            // This action is reasonable
                            childStates[nxt] = actionIndex;
                            res[actionIndex] = actionIndex;
                        } else {
                            // This action is not reasonable
                            res[actionIndex] = childStates[nxt];
                        }
                    } else {
                        // This action is not appicable
                        res[actionIndex] = -1;
                    }
                }
            } else {
                for (size_t actionIndex = 0; actionIndex < numberOfActions;
                     ++actionIndex) {
                    if (actionIsApplicable(actionStates[actionIndex], state)) {
                        res[actionIndex] = actionIndex;
                    } else {
                        res[actionIndex] = -1;
                    }
                }
            }

            if (cacheApplicableActions) {
                applicableActionsCache[state] = res;
            }
        }
        return res;
    }
};

#endif
