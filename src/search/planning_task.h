#ifndef PLANNING_TASK_H
#define PLANNING_TASK_H

#include <cstdlib>
#include <fdd.h>
#include <list>

#include "evaluatables.h"

class ProstPlanner;
class ActionFluent;
class ActionState;

class PlanningTask {
public:
    PlanningTask(std::string _name,
                 std::vector<ActionFluent*>& _actionFluents,
                 std::vector<ActionState>& _actionStates,
                 std::vector<StateFluent*>& stateFluents,
                 std::vector<ConditionalProbabilityFunction*>& _CPFs,
                 RewardFunction* _rewardCPF,
                 std::vector<Evaluatable*>& _actionPreconditions, 
                 State& _initialState,
                 int _horizon,
                 double _discountFactor,
                 bool _noopIsOptimalFinalAction,
                 bool _rewardFormulaAllowsRewardLockDetection,
                 std::vector<std::vector<long> > const& _stateHashKeys,
                 std::vector<long> const& _kleeneHashKeyBases,
                 std::vector<std::vector<std::pair<int,long> > > const& _indexToStateFluentHashKeyMap,
                 std::vector<std::vector<std::pair<int,long> > > const& _indexToKleeneStateFluentHashKeyMap);

    void learn(std::vector<State> const& trainingSet);

    // TODO: Make State constructor private and only allow the construction via this class.
    State getState(std::vector<double> const& stateVec, int const& remainingSteps) const {
        State res(stateVec, remainingSteps, numberOfStateFluentHashKeys);
        calcStateFluentHashKeys(res);
        calcStateHashKey(res);
        return res;
    }

/*****************************************************************
                Calculation of state transition
*****************************************************************/

    // Apply action 'actionIndex' to 'current', resulting in 'next'
    void calcSuccessorState(State const& current, int const& actionIndex, PDState& next) const {
        for(int index = 0; index < stateSize; ++index) {
            CPFs[index]->evaluateToPD(next[index], current, actionStates[actionIndex]);
        }
    }

    // Calculate the reward 'reward' of applying action 'actionIndex' in
    // 'current'
    void calcReward(State const& current, int const& actionIndex, double& reward) const {
        rewardCPF->evaluate(reward, current, actionStates[actionIndex]);
    }

    // As we are currently assuming that the reward is independent of the
    // successor state the (optimal) last reward can be calculated by applying
    // all applicable actions and returning the highest reward
    void calcOptimalFinalReward(State const& current, double& reward, bool const& useDeterminization);

    // Return the index of the optimal last action
    int getOptimalFinalActionIndex(State const& current, bool const& useDeterminization);

    // Apply action 'actionIndex' in the determinization to 'current', get state
    // 'next' and yield reward 'reward'
    void calcStateTransitionInDeterminization(State const& current, int const& actionIndex, State& next, double& reward) const {
        calcSuccessorStateInDeterminization(current, actionIndex, next);
        calcReward(current, actionIndex, reward);
    }

    // Apply action 'actionIndex' in the determinization to 'current', resulting
    // in 'next'.
    void calcSuccessorStateInDeterminization(State const& current, int const& actionIndex, State& next) const {
        for(int index = 0; index < stateSize; ++index) {
            CPFs[index]->evaluate(next[index], current, actionStates[actionIndex]);
        }
        calcStateFluentHashKeys(next);
        calcStateHashKey(next);
    }

/*****************************************************************
                Sampling of state transition
*****************************************************************/

    // Apply action 'actionIndex' to 'current', sample one outcome in 'next' and
    // yield reward 'reward'
    void sampleStateTransition(State const& current, int const& actionIndex, State& next, double& reward) const {
        sampleSuccessorState(current, actionIndex, next);
        calcReward(current, actionIndex, reward);
    }

    // Apply action 'actionIndex' to 'current' and sample one outcome in 'next'
    void sampleSuccessorState(State const& current, int const& actionIndex, State& next) const {
        PDState pdNext(stateSize, next.remainingSteps());
        calcSuccessorState(current, actionIndex, pdNext);
        for(unsigned int varIndex = 0; varIndex < stateSize; ++varIndex) {
            next[varIndex] = sampleVariable(pdNext[varIndex]);
        }
        calcStateFluentHashKeys(next);
        calcStateHashKey(next);
    }

    // Return the outcome of sampling variable 'pd'
    double sampleVariable(DiscretePD const& pd) const {
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
    void calcKleeneSuccessor(KleeneState const& current, int const& actionIndex, KleeneState& next) const {
        for(unsigned int i = 0; i < stateSize; ++i) {
            CPFs[i]->evaluateToKleene(next[i], current, actionStates[actionIndex]);
        }
    }

    // Calulate the reward in Kleene logic
    void calcKleeneReward(KleeneState const& current, int const& actionIndex, std::set<double>& reward) const {
        rewardCPF->evaluateToKleene(reward, current, actionStates[actionIndex]);
    }

/*****************************************************************
                 Calculation of state hash keys
*****************************************************************/

    // Calculate the hash key of a State
    void calcStateHashKey(State& state) const {
        if(stateHashingPossible) {
            state.hashKey = 0;
            for(unsigned int index = 0; index < stateSize; ++index) {
                state.hashKey += stateHashKeys[index][(int)state[index]];
            }
        } else {
            assert(state.hashKey == -1);
        }
    }

    // Calculate the hash key of a KleeneState
    void calcKleeneStateHashKey(KleeneState& state) const {
        if(kleeneStateHashingPossible) {
            state.hashKey = 0;
            for(unsigned int index = 0; index < stateSize; ++index) {
                int multiplier = 0;
                for(std::set<double>::iterator it = state[index].begin(); it != state[index].end(); ++it) {
                    multiplier |= 1<<((int)*it);
                }
                --multiplier;

                state.hashKey += (multiplier * kleeneHashKeyBases[index]);
            }
        } else {
            assert(state.hashKey == -1);
        }
    }

    // Calculate the hash key of a PDState
    // void calcPDStateHashKey(State& /*state*/) const {
    //     REPAIR
    // }

/*****************************************************************
             Calculation of state fluent hash keys
*****************************************************************/

    // Calculate the hash key for each state fluent in a State
    void calcStateFluentHashKeys(State& state) const {
        for(unsigned int i = 0; i < stateSize; ++i) {
            if(MathUtils::doubleIsGreater(state[i], 0.0)) {
                for(unsigned int j = 0; j < indexToStateFluentHashKeyMap[i].size(); ++j) {
                    assert(state.stateFluentHashKeys.size() > indexToStateFluentHashKeyMap[i][j].first);
                    state.stateFluentHashKeys[indexToStateFluentHashKeyMap[i][j].first] += ((int)state[i]) * indexToStateFluentHashKeyMap[i][j].second;
                    //state.stateFluentHashKeys[indexToStateFluentHashKeyMap[i][j].first] += indexToStateFluentHashKeyMap[i][j].second[(int)state[i]];
                }                
            }
        }
    }

    // Calculate the hash key for each state fluent in a KleeneState
    void calcKleeneStateFluentHashKeys(KleeneState& state) const {
        for(unsigned int i = 0; i < stateSize; ++i) {
            int multiplier = 0;
            for(std::set<double>::iterator it = state[i].begin(); it != state[i].end(); ++it) {
                multiplier |= 1<<((int)*it);
            }
           --multiplier;
           if(multiplier > 0) {
               for(unsigned int j = 0; j < indexToKleeneStateFluentHashKeyMap[i].size(); ++j) {
                   assert(state.stateFluentHashKeys.size() > indexToKleeneStateFluentHashKeyMap[i][j].first);
                   state.stateFluentHashKeys[indexToKleeneStateFluentHashKeyMap[i][j].first] += (multiplier * indexToKleeneStateFluentHashKeyMap[i][j].second);
               }
           }
        }
    }

    // Calculate the hash key for each state fluent in a PDState
    // void calcPDStateFluentHashKeys(PDState& state) const {
    //     REPAIR
    // }

/*****************************************************************
             Calculation of applicable actions
*****************************************************************/

    // Returns a vector ("res") that encodes applicable and reasonable actions.
    // If res[i] = i, the action with index i is applicable, and if res[i] = -1
    // it is not. Otherwise, the action with index i is unreasonable as the
    // action with index res[i] leads to the same distribution over successor
    // states (this is only checked if pruneUnreasonableActions is true).
    std::vector<int> getApplicableActions(State const& state, bool const& useDeterminization) {
        if(useDeterminization && hasUnreasonableActionsInDeterminization) {
            return setApplicableReasonableActionsInDeterminization(state);
        } else if(!useDeterminization && hasUnreasonableActions) {
            return setApplicableReasonableActions(state);
        }
        return setApplicableActions(state);
    }

    std::vector<int> getIndicesOfApplicableActions(State const& state, bool const& useDeterminization) {
        std::vector<int> applicableActions = getApplicableActions(state, useDeterminization);
        std::vector<int> result;
        for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
            if(applicableActions[actionIndex] == actionIndex) {
                result.push_back(actionIndex);
            }
        }
        return result;
    }

/*****************************************************************
           Getter functions for properties of the task
*****************************************************************/

    ActionState const& actionState(int const& index) const {
        assert(index < actionStates.size());
        return actionStates[index];
    }

    int const& getNumberOfStateFluentHashKeys() const {
        return numberOfStateFluentHashKeys;
    }

    int const& getNumberOfActions() const {
        return numberOfActions;
    }

    bool isMinReward(double const& rew) const {
        return MathUtils::doubleIsEqual(rew,rewardCPF->getMinVal());
    }

    double const& getMinReward() const {
        return rewardCPF->getMinVal();
    }

    bool isMaxReward(double const& rew) const {
        return MathUtils::doubleIsEqual(rew,rewardCPF->getMaxVal());
    }

    double const& getMaxReward() const {
        return rewardCPF->getMaxVal();
    }

    State const& getInitialState() const {
        return initialState;
    }

    int const& getHorizon() const {
        return horizon;
    }

    int const& getFirstProbabilisticVarIndex() const {
        return firstProbabilisticVarIndex;
    }

    int const& getStateSize() const {
        return stateSize;
    }

    // bool const& isPruningEquivalentToDeterminization() const {
    //     return isPruningEquivalentToDet;
    // }

    bool const& isDeterministic() const {
        return deterministic;
    }

    int getDomainSizeOfCPF(unsigned int const& index) const {
        return CPFs[index]->getDomainSize();
    }

    // Checks if current is a reward lock (actually, currently this checks only
    // if it is a dead end or a goal, i.e., a reward lock with minimal or
    // maximal reward).
    bool isARewardLock(State const& current);

    // If caching is disabled due to exceeding memory consumption, this is
    // called (this disables caching of all kinds)
    void disableCaching();

    // Printer functions
    void print(std::ostream& out) const;

    void printState(std::ostream& out, State const& state) const;
    void printKleeneState(std::ostream& out, KleeneState const& state) const;
    void printPDState(std::ostream& out, PDState const& state) const;
    void printAction(std::ostream& out, int const& actionIndex) const;

    void printActionInDetail(std::ostream& out, int const& index) const;

    void printCPFInDetail(std::ostream& out, int const& index) const;
    void printRewardCPFInDetail(std::ostream& out) const;
    void printActionPreconditionInDetail(std::ostream& out, int const& index) const;

    void printDeadEndBDD() const {
        bdd_printdot(cachedDeadEnds);
    }

    void printGoalBDD() const {
        bdd_printdot(cachedGoals);
    }

    // The name of this task (this is equivalent to the instance name)
    std::string name;

    // Contains state values for solved states.
    std::map<State, double, State::CompareConsideringRemainingSteps> stateValueCache;
    std::map<State, double, State::CompareConsideringRemainingSteps> stateValueCacheInDeterminization;

private:
    //void determinePruningEquivalence();

    // functions for action applicability and pruning
    std::vector<int> setApplicableReasonableActions(State const& state);
    std::vector<int> setApplicableReasonableActionsInDeterminization(State const& state);
    std::vector<int> setApplicableActions(State const& state);
    bool actionIsApplicable(ActionState const& action, State const& current) const;

    // functions for reward lock detection
    bool checkDeadEnd(KleeneState const& state);
    bool checkGoal(KleeneState const& state);

    //BDD related functions
    bdd stateToBDD(KleeneState const& state) const;
    bdd stateToBDD(State const& state) const;
    bool BDDIncludes(bdd BDD, KleeneState const& state) const;

    // Printer functions
    void printEvaluatableInDetail(std::ostream& out, Evaluatable* eval) const;

    // Action fluents and action states
    std::vector<ActionFluent*> actionFluents;
    std::vector<ActionState> actionStates;

    // State fluents
    std::vector<StateFluent*> stateFluents;

    // The CPFs
    std::vector<ConditionalProbabilityFunction*> CPFs;

    // The reward formula
    RewardFunction* rewardCPF;

    // The action preconditions
    std::vector<Evaluatable*> actionPreconditions;

    // Is true if this planning task is deterministic
    bool deterministic;

    // The index of this action is used to check if a state is a goal
    int goalTestActionIndex;

    // Is true if this task's determinization is equivalent w.r.t. to
    // reasonable action pruning
    // bool isPruningEquivalentToDet;

    // The initial state (the one given in the problem description,
    // this is not updated)
    State initialState;

    // The problem horizon
    int horizon;

    // The problem's discount factor (TODO: This is ignored at the moment)
    double discountFactor;

    // The number of actions (this is equal to actionStates.size())
    int numberOfActions;

    // The number of state fluents (this is equal to CPFs.size())
    int stateSize;

    // The number of variables that have a state fluent hash key
    int numberOfStateFluentHashKeys;

    // The index of the first probabilistic variable (variables are
    // ordered s.t. all deterministic ones come first)
    int firstProbabilisticVarIndex;

    // Is true if hashing of States (not state fluent hashing) is possible
    bool stateHashingPossible;

    // Is true if hashing of KleeneStates is possible
    bool kleeneStateHashingPossible;

    // Is true if hashing of PDStates is possible
    //bool pdStateHashingPossible;

    // Is true if noop is always optimal as last action. Therefore, it
    // must not be forbidden by static SACs, there must not be dynamic
    // SACs and the reward CPF may not contain positive action fluents.
    bool noopIsOptimalFinalAction;

    // Is true if reward lock detection is used
    bool useRewardLockDetection;
    bool useBDDCaching;

    // The BDDs where dead ends and goals are cached
    bdd cachedDeadEnds;
    bdd cachedGoals;

    // Is true if applicable actions should be cached
    bool cacheApplicableActions;

    // Is true if unreasonable actions where detected during learning
    bool hasUnreasonableActions;
    bool hasUnreasonableActionsInDeterminization;

    // These are used to calculate hash keys of States and KleeneStates
    std::vector<std::vector<long> > stateHashKeys;
    std::vector<long> kleeneHashKeyBases;

    // The Evaluatable with index indexToStateFluentHashKeyMap[i][j].first
    // depends on the CPF with index i, and is updated by multiplication with
    // indexToStateFluentHashKeyMap[i][j].second
    std::vector<std::vector<std::pair<int, long> > > indexToStateFluentHashKeyMap;
    std::vector<std::vector<std::pair<int,long> > > indexToKleeneStateFluentHashKeyMap;

    // These caches are used to save the results of getApplicableActions(). One
    // isn't sufficient as reasonable action pruning is sometimes desired and
    // sometimes it isn't.
    std::map<State, std::vector<int>, State::CompareIgnoringRemainingSteps> applicableActionsCache;
    std::map<State, std::vector<int>, State::CompareIgnoringRemainingSteps> applicableReasonableActionsCache;
    std::map<State, std::vector<int>, State::CompareIgnoringRemainingSteps> applicableReasonableActionsCacheInDeterminization;
};

#endif
