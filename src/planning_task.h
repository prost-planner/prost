#ifndef PLANNING_TASK_H
#define PLANNING_TASK_H

#include <cstdlib>
#include <bdd.h>
#include <list>

#include "caching_component.h"
#include "learning_component.h"
#include "state.h"
#include "actions.h"
#include "unprocessed_planning_task.h"
#include "logical_expressions.h"
#include "conditional_probability_function.h"

class ProstPlanner;
class ActionFluent;

class PlanningTask : public CachingComponent, public LearningComponent {
public:
    PlanningTask(ProstPlanner* _planner) :
        CachingComponent(_planner),
        LearningComponent(_planner),
        numberOfActions(-1),
        stateSize(-1),
        numberOfStateFluentHashKeys(0),
        deterministic(false),
        goalTestActionIndex(0),
        isPruningEquivalentToDet(false),
        firstProbabilisticVarIndex(-1),
        numberOfConcurrentActions(-1),
        horizon(-1),
        discountFactor(1.0),
        stateHashingPossible(false),
        stateHashingWithStatesAsProbabilityDistributionPossible(false),
        useRewardLockDetection(true),
        cachedDeadEnds(bddfalse),
        cachedGoals(bddfalse),
        cacheApplicableActions(true),
        hasUnreasonableActions(true),
        noopIsOptimalFinalAction(false) {}

    PlanningTask(PlanningTask const& other) :
        CachingComponent(other),
        LearningComponent(other),
        actionFluents(other.actionFluents),
        actionStates(other.actionStates),
        CPFs(other.CPFs),
        rewardCPF(other.rewardCPF),
        staticSACs(other.staticSACs),
        dynamicSACs(other.dynamicSACs),
        stateInvariants(other.stateInvariants),
        numberOfActions(other.numberOfActions),
        stateSize(other.stateSize),
        numberOfStateFluentHashKeys(other.numberOfStateFluentHashKeys),
        deterministic(other.deterministic),
        goalTestActionIndex(other.goalTestActionIndex),
        isPruningEquivalentToDet(other.isPruningEquivalentToDet),
        initialState(other.initialState),
        firstProbabilisticVarIndex(other.firstProbabilisticVarIndex),
        numberOfConcurrentActions(other.numberOfConcurrentActions),
        horizon(other.horizon),
        discountFactor(other.discountFactor),
        stateHashingPossible(other.stateHashingPossible),
        stateHashingWithStatesAsProbabilityDistributionPossible(other.stateHashingWithStatesAsProbabilityDistributionPossible),
        useRewardLockDetection(other.useRewardLockDetection),
        cachedDeadEnds(bddfalse),
        cachedGoals(bddfalse),
        cacheApplicableActions(true),
        hasUnreasonableActions(true),
        noopIsOptimalFinalAction(other.noopIsOptimalFinalAction),
        indexToStateFluentHashKeyMap(other.indexToStateFluentHashKeyMap),
        indexToKleeneStateFluentHashKeyMap(other.indexToKleeneStateFluentHashKeyMap) {}

    void initialize(std::vector<ActionFluent*>& _actionFluents, std::vector<ConditionalProbabilityFunction*>& _CPFs, 
                    std::vector<StateActionConstraint*>& _SACs, int _numberOfConcurrentActions,
                    int _horizon, double _discountFactor, std::map<std::string,int>& stateVariableIndices);

    PlanningTask* determinizeMostLikely(UnprocessedPlanningTask* task);

    bool learn(std::vector<State> const& trainingSet);

    State getState(std::vector<double> const& stateVec, int const& remainingSteps) const {
        State res(stateVec, remainingSteps, numberOfStateFluentHashKeys);
        calcStateFluentHashKeys(res);
        calcStateHashKey(res);
        return res;
    }

    // Calculates the probabiltity distribution that results from
    // applying action with index actionIndex in the state current
    void calcSuccessorAsProbabilityDistribution(State const& current, int const& actionIndex, State& nextAsProbDistr) const {
        for(int i = 0; i < stateSize; ++i) {
            CPFs[i]->evaluate(nextAsProbDistr[i], current, actionStates[actionIndex]);
        }
    }

    // Sample a successor state from a state given as a probability
    // distribution (and overwrite the state)
    void sampleSuccessorStateFromProbabilityDistribution(State& next) const {
        // Sample all probabilistic variables according to their
        // probability distribution
        for(int i = firstProbabilisticVarIndex; i < stateSize; ++i) {
            if(MathUtils::doubleIsEqual(next[i], 0.0) || MathUtils::doubleIsEqual(next[i], 1.0)) {
                continue;
            } else {
                next[i] = (MathUtils::doubleIsSmaller(MathUtils::generateRandomNumber(), next[i]) ? 1.0 : 0.0);
            }
        }
    }

    // Sample a successor state from a state given as a probability
    // distribution (and write to another state)
    void sampleSuccessorStateFromProbabilityDistribution(State const& nextAsProbDistr, State& next) const {
        // All deterministic variables are already 0 or 1
        for(int i = 0; i < firstProbabilisticVarIndex; ++i) {
            next[i] = nextAsProbDistr[i];
        }

        // Sample all probabilistic variables according to their
        // probability distribution
        for(int i = firstProbabilisticVarIndex; i < stateSize; ++i) {
            if(MathUtils::doubleIsEqual(nextAsProbDistr[i], 0.0) || MathUtils::doubleIsEqual(nextAsProbDistr[i], 1.0)) {
                next[i] = nextAsProbDistr[i];
                continue;
            } else {
                next[i] = (MathUtils::doubleIsSmaller(MathUtils::generateRandomNumber(), nextAsProbDistr[i]) ? 1.0 : 0.0);
            }
        }
    }

    // Samples a single variable within a state given as a probability distribution
    void sampleVariable(State& stateAsProbDistr, unsigned int const& varIndex) const {
        if(!MathUtils::doubleIsEqual(stateAsProbDistr[varIndex], 0.0) && !MathUtils::doubleIsEqual(stateAsProbDistr[varIndex], 1.0)) {
            stateAsProbDistr[varIndex] = (MathUtils::doubleIsSmaller(MathUtils::generateRandomNumber(), stateAsProbDistr[varIndex]) ? 1.0 : 0.0);
        }
    }

    // Calculate the whole state transition, including rewards
    void calcStateTransition(State const& current, int const& actionIndex, State& next, double& reward) const {
        calcSuccessorAsProbabilityDistribution(current, actionIndex, next);
        sampleSuccessorStateFromProbabilityDistribution(next);
        calcStateFluentHashKeys(next);
        calcStateHashKey(next);
        calcReward(current, actionIndex, reward);
    }

    // Calculate the successor state
    void calcSuccessorState(State const& current, int const& actionIndex, State& next) const {
        calcSuccessorAsProbabilityDistribution(current, actionIndex, next);
        sampleSuccessorStateFromProbabilityDistribution(next);
        calcStateFluentHashKeys(next);
        calcStateHashKey(next);
    }

    // Calulate the reward
    void calcReward(State const& current, int const& actionIndex, double& reward) const {
        rewardCPF->evaluate(reward, current, actionStates[actionIndex]);
    }

    // Calculate reward and optimal action of last state transition
    void calcOptimalFinalReward(State const& current, double& reward);

    // Get index of optimal action in last state transition
    int getOptimalFinalActionIndex(State const& current);

    // Calculate successor in Kleene logic
    void calcKleeneSuccessor(State const& current, int const& actionIndex, State& next) const {
        for(unsigned int i = 0; i < stateSize; ++i) {
            if(MathUtils::doubleIsMinusInfinity(current[i])) {
                next[i] = -std::numeric_limits<double>::max();
            } else {
                CPFs[i]->evaluateToKleeneOutcome(next[i], current, actionStates[actionIndex]);
            }
        }
    }

    // Merge two states in Kleene logic
    void mergeKleeneStates(State const& state, State& res) const {
        assert(state.state.size() == res.state.size());
        for(unsigned int i = 0; i < stateSize; ++i) {
            if(!MathUtils::doubleIsEqual(state[i], res[i])) {
                res[i] = -std::numeric_limits<double>::max();
            }
        }
    }

    // Create a Kleene state from a non-Kleene state
    State toKleeneState(State const& state) const {
        State res(state.state, state.remSteps, state.stateFluentHashKeys.size());
        calcKleeneStateFluentHashKeys(res);
        return res;
    }

    // Calulate the reward in Kleene logic
    void calcKleeneReward(State const& current, int const& actionIndex, double& reward) const {
        rewardCPF->evaluateToKleeneOutcome(reward, current, actionStates[actionIndex]);
    }

    // Calculate the state fluent hash key for each state fluent
    void calcStateFluentHashKeys(State& state) const {
        for(unsigned int i = 0; i < stateSize; ++i) {
            if(MathUtils::doubleIsEqual(state[i],1.0)) {
                for(unsigned int j = 0; j < indexToStateFluentHashKeyMap[i].size(); ++j) {
                    assert(state.stateFluentHashKeys.size() > indexToStateFluentHashKeyMap[i][j].first);
                    state.stateFluentHashKeys[indexToStateFluentHashKeyMap[i][j].first] += indexToStateFluentHashKeyMap[i][j].second;
                }                
            }
        }
    }

    // Calculate the Kleene state fluent hash key for each state
    // fluent
    void calcKleeneStateFluentHashKeys(State& state) const {
        for(unsigned int i = 0; i < stateSize; ++i) {
            if(MathUtils::doubleIsEqual(state[i],1.0)) {
                for(unsigned int j = 0; j < indexToKleeneStateFluentHashKeyMap[i].size(); ++j) {
                    assert(state.stateFluentHashKeys.size() > indexToKleeneStateFluentHashKeyMap[i][j].first);
                    state.stateFluentHashKeys[indexToKleeneStateFluentHashKeyMap[i][j].first] += indexToKleeneStateFluentHashKeyMap[i][j].second;
                }                
            } else if(MathUtils::doubleIsMinusInfinity(state[i])) {
                for(unsigned int j = 0; j < indexToKleeneStateFluentHashKeyMap[i].size(); ++j) {
                    assert(state.stateFluentHashKeys.size() > indexToKleeneStateFluentHashKeyMap[i][j].first);
                    state.stateFluentHashKeys[indexToKleeneStateFluentHashKeyMap[i][j].first] += (2*indexToKleeneStateFluentHashKeyMap[i][j].second);
                } 
            }
        }
    }

    // Calculate (bool) hash key and state fluent hash keys (if state
    // hashing is possible)
    void calcStateHashKey(State& state) const {
        if(stateHashingPossible) {
            state.hashKey = 0;
            for(int i = 0; i < stateSize; ++i) {
                if(MathUtils::doubleIsEqual(state[i],1.0)) {
                    state.hashKey += CPFs[i]->hashKeyBase;
                }
            }
        } else {
            assert(state.hashKey == -1);
        }
    }

    // Calculate hash key of states as probability distribution (if
    // state hashing is possible)
    void calcHashKeyOfProbabilityDistribution(State& state) const {
        if(stateHashingWithStatesAsProbabilityDistributionPossible) {
            state.hashKey = 0;
            // We differentiate between deterministic and
            // probabilistic variables here because we can omit a map
            // lookup for deterministic variables, and because the
            // probDomainMap is not set correctly in deterministic
            // tasks.
            for(unsigned int index = 0; index < firstProbabilisticVarIndex; ++index) {
                if(MathUtils::doubleIsEqual(state[index],1.0)) {
                    state.hashKey += CPFs[index]->hashKeyBase;
                }
            }

            for(int index = firstProbabilisticVarIndex; index < stateSize; ++index) {
                assert(CPFs[index]->probDomainMap.find(state[index]) != CPFs[index]->probDomainMap.end());
                state.hashKey += CPFs[index]->probDomainMap[state[index]];
            }
        } else {
            assert(state.hashKey == -1);
        }
    }

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
        return MathUtils::doubleIsEqual(rew,rewardCPF->minVal);
    }

    double const& getMinReward() const {
        return rewardCPF->minVal;
    }

    bool isMaxReward(double const& rew) const {
        return MathUtils::doubleIsEqual(rew,rewardCPF->maxVal);
    }

    double const& getMaxReward() const {
        return rewardCPF->maxVal;
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

    bool const& isPruningEquivalentToDeterminization() const {
        return isPruningEquivalentToDet;
    }

    bool const& isDeterministic() const {
        return deterministic;
    }

    // Returns a vector ("res") that encodes applicable and reasonable
    // actions. If res[i] = i, the action with index i is applicable,
    // and if res[i] = -1 it is not. Otherwise, the action with index
    // i is unreasonable as the action with index res[i] leads to the
    // same distribution over successor states (this is only checked
    // if pruneUnreasonableActions is true).
    std::vector<int> getApplicableActions(State const& state);
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

    // Checks if current is a reward lock (actually, currently this
    // checks only if it is a dead end or a goal, i.e., a reward lock
    // with minimal or maximal reward).
    bool isARewardLock(State const& current);

    // If caching is disabled due to exceeding memory consumption,
    // this is called (this disables caching of all kinds)
    void disableCaching();

    // Printer functions
    void print(std::ostream& out) const;

    void printState(std::ostream& out, State const& state) const;
    void printAction(std::ostream& out, int const& actionIndex) const;

    void printActionInDetail(std::ostream& out, int const& index) const;

    void printCPFInDetail(std::ostream& out, int const& index) const;
    void printRewardCPFInDetail(std::ostream& out) const;
    void printDynamicSACInDetail(std::ostream& out, int const& index) const;
    void printStaticSACInDetail(std::ostream& out, int const& index) const;
    void printStateInvariantInDetail(std::ostream& out, int const& index) const;

    // Contains state values for solved states.
    std::map<State, double, State::CompareConsideringRemainingSteps> stateValueCache;

private:
    void initializeSACs(std::vector<StateActionConstraint*>& _SACs);
    void initializeCPFs(std::vector<ConditionalProbabilityFunction*>& _CPFs);
    void initializeActions(std::vector<ActionFluent*>& _actionFluents);
    void calcPossiblyLegalActionStates(int actionsToSchedule, std::list<std::vector<int> >& result,
                                       std::vector<int> addTo = std::vector<int>());

    void initializeStateFluentHashKeys();
    void initializeStateHashKeys();
    void initializeHashKeysOfStatesAsProbabilityDistributions();

    void initializeDomainOfCPF(int const& index);
    std::set<double> calculateDomainOfCPF(ConditionalProbabilityFunction* cpf);

    void determinePruningEquivalence();
    void initializeRewardDependentVariables();

    // functions for action applicability and pruning
    void setApplicableReasonableActions(State const& state, std::vector<int>& res) const;
    void setApplicableActions(State const& state, std::vector<int>& res) const;

    // functions for reward lock detection
    bool checkDeadEnd(State const& state);
    bool checkGoal(State const& state);

    //BDD related functions
    void stateToBDD(State const& state, bdd& res);
    bool BDDIncludes(bdd BDD, State const& state);

    // Printer functions
    void printEvaluatableInDetail(std::ostream& out, Evaluatable* eval) const;

    // Action fluents and action states
    std::vector<ActionFluent*> actionFluents;
    std::vector<ActionState> actionStates;

    // The CPFs
    std::vector<ConditionalProbabilityFunction*> CPFs;
    ConditionalProbabilityFunction* rewardCPF;

    // The SACs
    std::vector<StateActionConstraint*> staticSACs;
    std::vector<StateActionConstraint*> dynamicSACs;
    std::vector<StateActionConstraint*> stateInvariants;

    // The number of actions (this is equal to actionStates.size())
    int numberOfActions;

    // The number of state fluents (this is equal to CPFs.size())
    int stateSize;

    // The number of variables that have a state fluent hash key
    int numberOfStateFluentHashKeys;

    // Is true if this planning task is deterministic
    bool deterministic;

    // The index of this action is used to check if a state is a goal
    int goalTestActionIndex;

    // Is true if this task's determinization is equivalent w.r.t. to
    // reasonable action pruning
    bool isPruningEquivalentToDet;

    // The initial state (the one given in the problem description,
    // this is not updated)
    State initialState;

    // The index of the first probabilistic variable (variables are
    // ordered s.t. all deterministic ones come first)
    int firstProbabilisticVarIndex;

    // The maximal number of concurrent actions
    int numberOfConcurrentActions;

    // The problem horizon
    int horizon;

    // The discount factor. TODO: This is not used anywhere at the
    // moment
    double discountFactor;

    // Is true if state hashing of complete states (not state fluent
    // hashing) is possible
    bool stateHashingPossible;

    // Is true if state hashing of states as probability distributions
    // is possible (again, not state fluent hashing)
    bool stateHashingWithStatesAsProbabilityDistributionPossible;

    // Is true if reward lock detection is used
    bool useRewardLockDetection;

    // The BDDs where dead ends and goals are cached
    bdd cachedDeadEnds;
    bdd cachedGoals;

    // Is true if applicable actions should be cached
    bool cacheApplicableActions;

    // Is true if unreasonable actions where detected during learning
    bool hasUnreasonableActions;

    // Is true if noop is always optimal as last action. Therefore, it
    // must not be forbidden by static SACs, there must not be dynamic
    // SACs and the reward CPF may not contain positive action fluents.
    bool noopIsOptimalFinalAction;

    // These caches are used to save the results of
    // getApplicableActions(). One isn't sufficient as reasonable
    // action pruning is sometimes desired and sometimes it isn't.
    std::map<State, std::vector<int>, State::CompareIgnoringRemainingSteps> applicableActionsCache;
    std::map<State, std::vector<int>, State::CompareIgnoringRemainingSteps> applicableReasonableActionsCache;

    // The Evaluatable with index
    // indexToStateFluentHashKeyMap[i][j].first depends on variable i,
    // and is updated with indexToStateFluentHashKeyMap[i][j].second
    std::vector<std::vector<std::pair<int,long> > > indexToStateFluentHashKeyMap;
    std::vector<std::vector<std::pair<int,long> > > indexToKleeneStateFluentHashKeyMap;
};

#endif
