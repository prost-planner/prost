#ifndef PLANNING_TASK_H
#define PLANNING_TASK_H

#include <cstdlib>
#include <fdd.h>
#include <list>

#include "caching_component.h"
#include "learning_component.h"
#include "kleene_state.h"
#include "pd_state.h"
#include "actions.h"
#include "unprocessed_planning_task.h"
#include "logical_expressions.h"
#include "conditional_probability_function.h"

class ProstPlanner;
class ActionFluent;

class PlanningTask : public CachingComponent, public LearningComponent {
public:
    // This is only to sort transition functions by their name to
    // ensure deterministic behaviour
    struct TransitionFunctionSort {
        bool operator() (ConditionalProbabilityFunction* const& lhs, ConditionalProbabilityFunction* const& rhs) const {
            return lhs->head->name < rhs->head->name;
        }
    };

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
        kleeneStateHashingPossible(false),
        pdStateHashingPossible(false),
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
        kleeneStateHashingPossible(other.kleeneStateHashingPossible),
        pdStateHashingPossible(other.pdStateHashingPossible),
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
                    int _horizon, double _discountFactor, std::map<std::string,int>& stateVariableIndices,
                    std::vector<std::vector<std::string> >& stateVariableValues);

    PlanningTask* determinizeMostLikely(UnprocessedPlanningTask* task);

    bool learn(std::vector<State> const& trainingSet);

    State getState(std::vector<double> const& stateVec, int const& remainingSteps) const {
        State res(stateVec, remainingSteps, numberOfStateFluentHashKeys);
        calcStateFluentHashKeys(res);
        calcStateHashKey(res);
        return res;
    }

    PDState getPDState(int remainingSteps) const {
        return PDState(getStateSize(), remainingSteps);
    }

    // Calculates the probabiltity distribution that results from applying
    // action with index actionIndex in the state current
    void calcSuccessorState(State const& current, int const& actionIndex, PDState& nextAsProbDistr) const {
        for(int i = 0; i < stateSize; ++i) {
            CPFs[i]->evaluate(nextAsProbDistr[i], current, actionStates[actionIndex]);
        }
    }

    // Sample a successor state from a state given as a probability distribution
    // (and overwrite the state)
    void sampleSuccessorStateFromProbabilityDistribution(State& next) const {
        // Sample all probabilistic variables according to their probability
        // distribution
        for(int i = firstProbabilisticVarIndex; i < stateSize; ++i) {
            if(MathUtils::doubleIsEqual(next[i], 0.0) || MathUtils::doubleIsEqual(next[i], 1.0)) {
                continue;
            } else {
                next[i] = (MathUtils::doubleIsSmaller(MathUtils::generateRandomNumber(), next[i]) ? 1.0 : 0.0);
            }
        }
    }

    // Sample a successor state from a state given as a probability distribution
    // (and write to another state)
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

    // Samples the successor state
    void sampleSuccessorState(State const& current, int const& actionIndex, State& next) const {
        PDState pdNext = getPDState(next.remainingSteps());
        calcSuccessorState(current, actionIndex, pdNext);
        for(unsigned int varIndex = 0; varIndex < stateSize; ++varIndex) {
            next[varIndex] = sampleVariable(pdNext[varIndex]);
        }
        calcStateFluentHashKeys(next);
        calcStateHashKey(next);
    }    

    // Samples the whole state transition
    void sampleStateTransition(State const& current, int const& actionIndex, State& next, double& reward) const {
        sampleSuccessorState(current, actionIndex, next);
        calcReward(current, actionIndex, reward);
    }

    // Calulate the reward
    void calcReward(State const& current, int const& actionIndex, double& reward) const {
        DiscretePD tmp;
        rewardCPF->evaluate(tmp, current, actionStates[actionIndex]);
        assert(tmp.isDeterministic());
        reward = tmp.values[0];
    }

    // Calculate reward and optimal action of last state transition
    void calcOptimalFinalReward(State const& current, double& reward);

    // Get index of optimal action in last state transition
    int getOptimalFinalActionIndex(State const& current);

    // Calculate successor in Kleene logic
    void calcKleeneSuccessor(KleeneState const& current, int const& actionIndex, KleeneState& next) const {
        for(unsigned int i = 0; i < stateSize; ++i) {
            CPFs[i]->evaluateToKleeneOutcome(next[i], current, actionStates[actionIndex]);
        }
    }

    // Merge two states in Kleene logic
    void mergeKleeneStates(KleeneState const& state, KleeneState& res) const {
        assert(state.state.size() == res.state.size());

        for(unsigned int i = 0; i < stateSize; ++i) {
            res[i].insert(state[i].begin(), state[i].end());
        }
    }

    // Calulate the reward in Kleene logic
    void calcKleeneReward(KleeneState const& current, int const& actionIndex, std::set<double>& reward) const {
        rewardCPF->evaluateToKleeneOutcome(reward, current, actionStates[actionIndex]);
    }

    // Calculate the hash key for each state fluent in a State
    void calcStateFluentHashKeys(State& state) const {
        for(unsigned int i = 0; i < stateSize; ++i) {
            if(MathUtils::doubleIsGreater(state[i], 0.0)) {
                for(unsigned int j = 0; j < indexToStateFluentHashKeyMap[i].size(); ++j) {
                    assert(state.stateFluentHashKeys.size() > indexToStateFluentHashKeyMap[i][j].first);
                    state.stateFluentHashKeys[indexToStateFluentHashKeyMap[i][j].first] += ((int)state[i]) * indexToStateFluentHashKeyMap[i][j].second;
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

    // Calculate the hash key of a State
    void calcStateHashKey(State& state) const {
        if(stateHashingPossible) {
            state.hashKey = 0;
            for(int i = 0; i < stateSize; ++i) {
                state.hashKey += (((int)state[i]) * CPFs[i]->hashKeyBase);
            }
        } else {
            assert(state.hashKey == -1);
        }
    }

    // Calculate the hash key of a KleeneState
    void calcKleeneStateHashKey(KleeneState& state) const {
        if(kleeneStateHashingPossible) {
            state.hashKey = 0;
            for(int i = 0; i < stateSize; ++i) {
                int multiplier = 0;
                for(std::set<double>::iterator it = state[i].begin(); it != state[i].end(); ++it) {
                    multiplier |= 1<<((int)*it);
                }
                --multiplier;

                state.hashKey += (multiplier * CPFs[i]->kleeneHashKeyBase);
            }
        } else {
            assert(state.hashKey == -1);
        }
    }

    // Calculate the hash key of a PDState
    // void calcPDStateHashKey(State& /*state*/) const {
    //     //REPAIR
    //     // if(pdStateHashingPossible) {
    //     //     state.hashKey = 0;

    //     //     for(int index = 0; index < stateSize; ++index) {
    //     //         CPFs[index]->addProbHashKeyPart(state[index], state.hashKey);
    //     //     }
    //     // } else {
    //     //     assert(state.hashKey == -1);
    //     // }
    // }

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
    void printKleeneState(std::ostream& out, KleeneState const& state) const;
    void printPDState(std::ostream& out, PDState const& state) const;
    void printAction(std::ostream& out, int const& actionIndex) const;

    void printActionInDetail(std::ostream& out, int const& index) const;

    void printCPFInDetail(std::ostream& out, int const& index) const;
    void printRewardCPFInDetail(std::ostream& out) const;
    void printDynamicSACInDetail(std::ostream& out, int const& index) const;
    void printStaticSACInDetail(std::ostream& out, int const& index) const;
    void printStateInvariantInDetail(std::ostream& out, int const& index) const;

    void printDeadEndBDD() const {
        bdd_printdot(cachedDeadEnds);
    }

    void printGoalBDD() const {
        bdd_printdot(cachedGoals);
    }

    // Contains state values for solved states.
    std::map<State, double, State::CompareConsideringRemainingSteps> stateValueCache;

private:
    void initializeSACs(std::vector<StateActionConstraint*>& _SACs);
    void initializeCPFs(std::vector<ConditionalProbabilityFunction*>& _CPFs);
    void initializeActions(std::vector<ActionFluent*>& _actionFluents);
    void calcPossiblyLegalActionStates(int actionsToSchedule, std::list<std::vector<int> >& result,
                                       std::vector<int> addTo = std::vector<int>());

    void initializeDomains();

    void initializeStateFluentHashKeys();
    void initializeStateHashKeys();
    void initializeKleeneStateHashKeys();
    void initializeHashKeysOfStatesAsProbabilityDistributions();

    void determinePruningEquivalence();
    void initializeRewardDependentVariables();

    // functions for action applicability and pruning
    void setApplicableReasonableActions(State const& state, std::vector<int>& res) const;
    void setApplicableActions(State const& state, std::vector<int>& res) const;

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

    // Is true if hashing of States (not state fluent hashing) is possible
    bool stateHashingPossible;

    // Is true if hashing of KleeneStates is possible
    bool kleeneStateHashingPossible;

    // Is true if hashing of PDStates is possible
    bool pdStateHashingPossible;

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

    // The Evaluatable with index indexToStateFluentHashKeyMap[i][j].first
    // depends on variable i, and is updated with
    // indexToStateFluentHashKeyMap[i][j].second
    std::vector<std::vector<std::pair<int,long> > > indexToStateFluentHashKeyMap;
    std::vector<std::vector<std::pair<int,long> > > indexToKleeneStateFluentHashKeyMap;
};

#endif
