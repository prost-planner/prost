#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

// This is the abstract interface all search engines are based on. It
// only implements a basic method to estimate best actions by calling
// estimateQValues, and defines a couple of member variables.

#include "planning_task.h"
#include "evaluatables.h"

#include <sstream>
#include <vector>
#include <fdd.h>

class SearchEngine {
public:

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

/*****************************************************************
                     Main search functions
*****************************************************************/

    // This is called initially to learn parameter values from a training set
    virtual void learn() {}

    // Start the search engine to calculate best actions
    virtual bool estimateBestActions(State const& _rootState, std::vector<int>& bestActions);

    // Start the search engine for state value estimation
    virtual bool estimateStateValue(State const& _rootState, double& stateValue);

    // Start the search engine for Q-value estimation
    virtual bool estimateQValues(State const& _rootState, std::vector<int> const& actionsToExpand, std::vector<double>& qValues) = 0;

/*****************************************************************
                             Parameter
*****************************************************************/

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
                       Printing and statistics
*****************************************************************/

    // Reset statistic variables
    virtual void resetStats() {}

    // Printer
    virtual void print(std::ostream& out);
    virtual void printStats(std::ostream& out, bool const& printRoundStats, std::string indent = "");

    static void printDeadEndBDD() {
        bdd_printdot(cachedDeadEnds);
    }

    static void printGoalBDD() {
        bdd_printdot(cachedGoals);
    }

protected:
    SearchEngine(std::string _name, bool _useDeterminization) :
        name(_name),
        myUseDeterminization(_useDeterminization),
        cachingEnabled(true),
        maxSearchDepth(PlanningTask::horizon),
        generalLearningFinished(false) {}

/*****************************************************************
                     Member variables
*****************************************************************/

    // Used for debug output only
    std::string name;

    // Gives whether the determinization or the original probabilistic task is
    // used
    bool myUseDeterminization;

    // Parameter
    bool cachingEnabled;
    int maxSearchDepth;

    // Stream for nicer (and better timed) printing
    std::stringstream outStream;

public:

/*****************************************************************
                Calculation of state transition
*****************************************************************/

    // Apply action 'actionIndex' to 'current', resulting in 'next'
    void calcSuccessorState(State const& current, int const& actionIndex, PDState& next) {
        for(int index = 0; index < State::stateSize; ++index) {
            PlanningTask::CPFs[index]->evaluateToPD(next[index], current, PlanningTask::actionStates[actionIndex]);
        }
    }

    // Calculate the reward 'reward' of applying action 'actionIndex' in
    // 'current'
    void calcReward(State const& current, int const& actionIndex, double& reward) {
        PlanningTask::rewardCPF->evaluate(reward, current, PlanningTask::actionStates[actionIndex]);
    }

    // As we are currently assuming that the reward is independent of the
    // successor state the (optimal) last reward can be calculated by applying
    // all applicable actions and returning the highest reward
    void calcOptimalFinalReward(State const& current, double& reward);

    // Return the index of the optimal last action
    int getOptimalFinalActionIndex(State const& current);

    // Apply action 'actionIndex' in the determinization to 'current', get state
    // 'next' and yield reward 'reward'
    void calcStateTransitionInDeterminization(State const& current, int const& actionIndex, State& next, double& reward) {
        calcSuccessorStateInDeterminization(current, actionIndex, next);
        calcReward(current, actionIndex, reward);
    }

    // Apply action 'actionIndex' in the determinization to 'current', resulting
    // in 'next'.
    void calcSuccessorStateInDeterminization(State const& current, int const& actionIndex, State& next) {
        for(unsigned int index = 0; index < State::stateSize; ++index) {
            PlanningTask::CPFs[index]->evaluate(next[index], current, PlanningTask::actionStates[actionIndex]);
        }
        State::calcStateFluentHashKeys(next);
        State::calcStateHashKey(next);
    }

/*****************************************************************
                Sampling of state transition
*****************************************************************/

    // Apply action 'actionIndex' to 'current', sample one outcome in 'next' and
    // yield reward 'reward'
    void sampleStateTransition(State const& current, int const& actionIndex, State& next, double& reward) {
        sampleSuccessorState(current, actionIndex, next);
        calcReward(current, actionIndex, reward);
    }

    // Apply action 'actionIndex' to 'current' and sample one outcome in 'next'
    void sampleSuccessorState(State const& current, int const& actionIndex, State& next) {
        PDState pdNext(State::stateSize, next.remainingSteps());
        calcSuccessorState(current, actionIndex, pdNext);
        for(unsigned int varIndex = 0; varIndex < State::stateSize; ++varIndex) {
            next[varIndex] = sampleVariable(pdNext[varIndex]);
        }
        State::calcStateFluentHashKeys(next);
        State::calcStateHashKey(next);
    }

    // Return the outcome of sampling variable 'pd'
    double sampleVariable(DiscretePD const& pd) {
        assert(pd.isWellDefined());
        double randNum = MathUtils::generateRandomNumber();
        double probSum = 0.0;
        for(unsigned int index = 0; index < pd.probabilities.size(); ++index) {
            probSum += pd.probabilities[index];
            if(MathUtils::doubleIsSmaller(randNum, probSum)) {
                return pd.values[index];
            }
        }
        assert(false);
        return 0.0;
    }

/*****************************************************************
             Calculation of Kleene state transition
*****************************************************************/

    // Calculate successor in Kleene logic
    void calcKleeneSuccessor(KleeneState const& current, int const& actionIndex, KleeneState& next) {
        for(unsigned int i = 0; i < KleeneState::stateSize; ++i) {
            PlanningTask::CPFs[i]->evaluateToKleene(next[i], current, PlanningTask::actionStates[actionIndex]);
        }
    }

    // Calulate the reward in Kleene logic
    void calcKleeneReward(KleeneState const& current, int const& actionIndex, std::set<double>& reward) {
        PlanningTask::rewardCPF->evaluateToKleene(reward, current, PlanningTask::actionStates[actionIndex]);
    }

/*****************************************************************
                       Reward lock detection
*****************************************************************/

    // Checks if current is a reward lock (actually, currently this checks only
    // if it is a dead end or a goal, i.e., a reward lock with minimal or
    // maximal reward).
    bool isARewardLock(State const& current);

/*****************************************************************
                    Static member variables
*****************************************************************/

    // Is true if applicable actions should be cached
    static bool cacheApplicableActions;

    // Is true if reward lock detection is used
    static bool useRewardLockDetection;

    // The index of this action is used to check if a state is a goal
    static int goalTestActionIndex;

    // Should be a parameter (TODO!) that is used to decide if reward locks are
    // cached
    static bool useBDDCaching;

    // The BDDs where dead ends and goals are cached
    static bdd cachedDeadEnds;
    static bdd cachedGoals;

protected:

    // These caches are used to save the results of getApplicableActions(). One
    // isn't sufficient as reasonable action pruning is sometimes desired and
    // sometimes it isn't.
    static std::map<State, std::vector<int>, State::CompareIgnoringRemainingSteps> applicableActionsCache;
    static std::map<State, std::vector<int>, State::CompareIgnoringRemainingSteps> applicableReasonableActionsCache;
    static std::map<State, std::vector<int>, State::CompareIgnoringRemainingSteps> applicableReasonableActionsCacheInDeterminization;

    // Methods for action applicability and pruning
    virtual std::vector<int> getApplicableActions(State const& state) = 0;
    std::vector<int> setApplicableActions(State const& state);
    bool actionIsApplicable(ActionState const& action, State const& current);

    // This is made true after the general learning run was performed to make
    // sure it doesn't run more than once
    bool generalLearningFinished;

private:
    // Methods for reward lock detection
    bool checkDeadEnd(KleeneState const& state);
    bool checkGoal(KleeneState const& state);

    // BDD related methods
    bdd stateToBDD(KleeneState const& state);
    bdd stateToBDD(State const& state);
    bool BDDIncludes(bdd BDD, KleeneState const& state);

    // Methods to calculate the final reward
    void calcOptimalFinalRewardWithFirstApplicableAction(State const& current, double& reward);
    void calcOptimalFinalRewardAsBestOfCandidateSet(State const& current, double& reward);
};

class ProbabilisticSearchEngine : public SearchEngine {
public:
    ProbabilisticSearchEngine(std::string _name) :
        SearchEngine(_name, false) {}

    // This is called initially to learn parameter values from a training set
    void learn();

    // We only implement this to create a search engine for learing the general stuff
    virtual bool estimateQValues(State const& /*_rootState*/, std::vector<int> const& /*actionsToExpand*/, std::vector<double>& /*qValues*/) {
        assert(false);
        return false;
    }

    // Is true if unreasonable actions where detected during learning
    static bool hasUnreasonableActions;

protected:
    // Contains state values for solved states.
    static std::map<State, double, State::CompareConsideringRemainingSteps> stateValueCache;

/*****************************************************************
             Calculation of applicable actions
*****************************************************************/

    // Returns a vector ("res") that encodes applicable and reasonable actions.
    // If res[i] = i, the action with index i is applicable, and if res[i] = -1
    // it is not. Otherwise, the action with index i is unreasonable as the
    // action with index res[i] leads to the same distribution over successor
    // states (this is only checked if pruneUnreasonableActions is true).
    std::vector<int> getApplicableActions(State const& state) {
        if(ProbabilisticSearchEngine::hasUnreasonableActions) {
            return ProbabilisticSearchEngine::setApplicableReasonableActions(state);
        }
        return SearchEngine::setApplicableActions(state);
    }

    std::vector<int> getIndicesOfApplicableActions(State const& state) {
        std::vector<int> applicableActions = getApplicableActions(state);
        std::vector<int> result;
        for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
            if(applicableActions[actionIndex] == actionIndex) {
                result.push_back(actionIndex);
            }
        }
        return result;
    }

    std::vector<int> setApplicableReasonableActions(State const& state);
};

class DeterministicSearchEngine : public SearchEngine {
public:
    DeterministicSearchEngine(std::string _name) :
        SearchEngine(_name, true) {}

    // This is called initially to learn parameter values from a training set
    void learn();

    // We only implement this to create a search engine for learing the general stuff
    virtual bool estimateQValues(State const& /*_rootState*/, std::vector<int> const& /*actionsToExpand*/, std::vector<double>& /*qValues*/) {
        assert(false);
        return false;
    }

    // Is true if unreasonable actions where detected during learning
    static bool hasUnreasonableActions;

protected:
    // Contains state values for solved states.
    static std::map<State, double, State::CompareConsideringRemainingSteps> stateValueCache;

/*****************************************************************
             Calculation of applicable actions
*****************************************************************/

    // Returns a vector ("res") that encodes applicable and reasonable actions.
    // If res[i] = i, the action with index i is applicable, and if res[i] = -1
    // it is not. Otherwise, the action with index i is unreasonable as the
    // action with index res[i] leads to the same distribution over successor
    // states (this is only checked if pruneUnreasonableActions is true).
    std::vector<int> getApplicableActions(State const& state) {
        if(DeterministicSearchEngine::hasUnreasonableActions) {
            return DeterministicSearchEngine::setApplicableReasonableActions(state);
        }
        return SearchEngine::setApplicableActions(state);
    }

    std::vector<int> getIndicesOfApplicableActions(State const& state) {
        std::vector<int> applicableActions = DeterministicSearchEngine::getApplicableActions(state);
        std::vector<int> result;
        for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
            if(applicableActions[actionIndex] == actionIndex) {
                result.push_back(actionIndex);
            }
        }
        return result;
    }

    std::vector<int> setApplicableReasonableActions(State const& state);
};

#endif
