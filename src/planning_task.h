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
#include "conditional_probability_functions.h"

class ProstPlanner;
class ActionFluent;

class PlanningTask : public CachingComponent, public LearningComponent {
public:
    PlanningTask(ProstPlanner* _planner) :
        CachingComponent(_planner),
        LearningComponent(_planner),
        planner(_planner),
        numberOfActions(-1),
        isDeterministic(false),
        noopIsOptimalFinalAct(false),
        isPruningEquivalentToDet(false),
        firstProbabilisticVarIndex(-1),
        numberOfConcurrentActions(-1),
        horizon(-1),
        discountFactor(-1),
        stateSize(-1),
        stateHashingPoss(false),
        probStateHashingPoss(false),
        randNum(0.0),
        cachedDeadLocks(bddfalse),
        cachedGoals(bddfalse),
        useRewardLockDetection(false),
        actionsToExpandCache(),
        cacheActionsToExpand(true), //TODO: MAKE THIS A PARAMETER
        useReasonableActionPruning(true),
        stateValueCache() {}

    PlanningTask(PlanningTask const& other) :
        CachingComponent(other.planner),
        LearningComponent(other.planner),
        planner(other.planner),
        actionFluents(other.actionFluents),
        actionStates(other.actionStates),
        numberOfActions(other.numberOfActions),
        isDeterministic(other.isDeterministic),
        noopIsOptimalFinalAct(other.noopIsOptimalFinalAct),
        isPruningEquivalentToDet(other.isPruningEquivalentToDet),
        initialState(other.initialState),
        firstProbabilisticVarIndex(other.firstProbabilisticVarIndex),
        numberOfConcurrentActions(other.numberOfConcurrentActions),
        horizon(other.horizon),
        discountFactor(other.discountFactor),
        stateSize(other.stateSize),
        stateHashingPoss(other.stateHashingPoss),
        probStateHashingPoss(other.probStateHashingPoss),
        randNum(0.0),
        cachedDeadLocks(bddfalse),
        cachedGoals(bddfalse),
        useRewardLockDetection(true),
        actionsToExpandCache(),
        cacheActionsToExpand(true), //TODO: MAKE THIS A PARAMETER
        useReasonableActionPruning(true),
        CPFs(other.CPFs),
        rewardCPF(other.rewardCPF),
        SACs(other.SACs),
        indexToStateFluentHashKeyMap(other.indexToStateFluentHashKeyMap),
        indexToKleeneStateFluentHashKeyMap(other.indexToKleeneStateFluentHashKeyMap),
        stateValueCache() {}   

    void initialize(std::vector<ActionFluent*>& _actionFluents, std::vector<ConditionalProbabilityFunction*>& _CPFs, 
                    std::vector<StateActionConstraint*>& _SACs, int _numberOfConcurrentActions,
                    int _horizon, double _discountFactor, std::map<std::string,int>& stateVariableIndices);

    PlanningTask* determinizeMostLikely(UnprocessedPlanningTask* task);

    bool learn(std::vector<State> const& trainingSet);

    //Calculate the whole state transition, including rewards
    void calcStateTransition(State const& current, int const& actionIndex, State& next, double& reward) {
        calcSuccessorAsProbabilityDistribution(current, actionIndex, next);
        sampleSuccessorStateFromProbabilityDistribution(next);
        calcStateFluentHashKeys(next);
        calcStateHashKey(next);
        calcReward(current, actionIndex, next, reward);
    }

    //Calculate the successor state
    void calcSuccessorState(State const& current, int const& actionIndex, State& next) {
        calcSuccessorAsProbabilityDistribution(current, actionIndex, next);
        sampleSuccessorStateFromProbabilityDistribution(next);
        calcStateFluentHashKeys(next);
        calcStateHashKey(next);
    }

    //Calculate the whole state transition, including rewards and keep the used probability distribution
    void calcStateTransitionAndProbabilityDistribution(State const& current, int const& actionIndex, State& next, State& nextAsProbDistr, double& reward) {
        calcSuccessorAsProbabilityDistribution(current, actionIndex, nextAsProbDistr);
        sampleSuccessorStateFromProbabilityDistribution(nextAsProbDistr, next);
        calcStateFluentHashKeys(next);
        calcStateHashKey(next);
        calcReward(current, actionIndex, next, reward);
    }

    //calc probabiltity distribution that results in applying actions in current
    void calcSuccessorAsProbabilityDistribution(State const& current, int const& actionIndex, State& nextAsProbDistr) const {
        for(int i = 0; i < getStateSize(); ++i) {
            CPFs[i]->evaluate(nextAsProbDistr[i], current, nextAsProbDistr, actionStates[actionIndex]);
        }
    }

    //sample a successor distribution
    void sampleSuccessorStateFromProbabilityDistribution(State& next) const {
        for(int i = firstProbabilisticVarIndex; i < stateSize; ++i) {
            if(MathUtils::doubleIsEqual(next[i], 0.0) || MathUtils::doubleIsEqual(next[i], 1.0)) {
                continue;
            } else {
                generateRandomNumber(randNum);
                next[i] = (MathUtils::doubleIsSmaller(randNum, next[i]) ? 1.0 : 0.0);
            }
        }
    }

    void sampleSuccessorStateFromProbabilityDistribution(State const& nextAsProbDistr, State& next) const {
        for(int i = 0; i < firstProbabilisticVarIndex; ++i) {
            next[i] = nextAsProbDistr[i];
        }
        for(int i = firstProbabilisticVarIndex; i < stateSize; ++i) {
            if(MathUtils::doubleIsEqual(nextAsProbDistr[i], 0.0) || MathUtils::doubleIsEqual(nextAsProbDistr[i], 1.0)) {
                next[i] = nextAsProbDistr[i];
                continue;
            } else {
                generateRandomNumber(randNum);
                next[i] = (MathUtils::doubleIsSmaller(randNum, nextAsProbDistr[i]) ? 1.0 : 0.0);
            }
        }
    }

    //calulate the reward
    void calcReward(State const& current, int const& actionIndex, State const& next, double& reward) const {
        rewardCPF->evaluate(reward, current, next, actionStates[actionIndex]);
    }

    //calculate successor in Kleene logic
    void calcKleeneSuccessor(State const& current, int const& actionIndex, State& next) const {
        for(unsigned int i = 0; i < stateSize; ++i) {
            if(MathUtils::doubleIsMinusInfinity(current[i])) {
                next[i] = -std::numeric_limits<double>::max();
            } else {
                CPFs[i]->evaluateToKleeneOutcome(next[i], current, next, actionStates[actionIndex]);
            }
        }
    }

    //merge two states in Kleene logic
    void mergeKleeneStates(State const& state, State& res) {
        assert(state.state.size() == res.state.size());
        for(unsigned int i = 0; i < stateSize; ++i) {
            if(!MathUtils::doubleIsEqual(state[i], res[i])) {
                res[i] = -std::numeric_limits<double>::max();
            }
        }
    }

    //create a Kleene state from a non-Kleene state
    State toKleeneState(State const& state) {
        State res(state.state, state.remSteps);
        calcKleeneStateFluentHashKeys(res);
        return res;
    }

    //calulate the reward in Kleene logic
    void calcKleeneReward(State const& current, int const& actionIndex, State const& next, double& reward) const {
        rewardCPF->evaluateToKleeneOutcome(reward, current, next, actionStates[actionIndex]);
    }

    //calculate the state fluent hash key for each state fluent
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

    //calculate the Kleene state fluent hash key for each state fluent
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

    //calculate (bool) hash key and state fluent hash keys (if state
    //hashing is possible)
    void calcStateHashKey(State& state) const {
        if(stateHashingPoss) {
            state.hashKey = 0;
            for(int i = 0; i < stateSize; ++i) {
                if(MathUtils::doubleIsEqual(state[i],1.0)) {
                    assert(MathUtils::twoToThePowerOf(i) == CPFs[i]->hashKeyBase);
                    state.hashKey += CPFs[i]->hashKeyBase;
                }
            }
        } else {
            assert(state.hashKey == -1);
        }
    }

    //calculate hash key of states as probability distribution (if
    //state hashing is possible)
    void calcHashKeyOfProbabilityDistribution(State& state) {
        if(probStateHashingPoss) {
            state.hashKey = 0;
            for(int i = 0; i < firstProbabilisticVarIndex; ++i) {
                if(MathUtils::doubleIsEqual(state[i],1.0)) {
                    state.hashKey += CPFs[i]->hashKeyBase;
                }
            }

            for(int i = firstProbabilisticVarIndex; i < stateSize; ++i) {
                assert(CPFs[i]->probDomainMap.find(state[i]) != CPFs[i]->probDomainMap.end());
                state.hashKey += CPFs[i]->probDomainMap[state[i]];
            }
        } else {
            assert(state.hashKey == -1);
        }
    }

    bool const& stateHashingPossible() const {
        return stateHashingPoss;
    }

    bool const& probabilisticStateHashingPossible() const {
        return probStateHashingPoss;
    }

    ActionState const& actionState(int const& index) const {
        assert(index < actionStates.size());
        return actionStates[index];
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

    bool const& noopIsOptimalFinalAction() const {
        return noopIsOptimalFinalAct;
    }

    bool rewardIsNextStateIndependent() const {
        return rewardCPF->isNextStateIndependent();
    }

    bool rewardIsActionIndependent() const {
        return rewardCPF->isActionIndependent();
    }

    //if res[index] == index, the action should be expanded, otherwise
    //the action index leads to equivalent results as res[index] if
    //the reward is action independent, and to worse results if
    void setActionsToExpand(State const& state, std::vector<int>& res);
    bool isARewardLock(State const& current);

    void generateRandomNumber(double& res) const {
        res = ((double)(rand() % 1000001) / 1000000.0);
    }

    void disableCaching();

    void print(std::ostream& out) const;
    void printState(std::ostream& out, State const& s) const;
    void printAction(std::ostream& out, int const& index) const;

private:
    void initializeCPFs(std::vector<ConditionalProbabilityFunction*>& _CPFs);
    void initializeActions(std::vector<ActionFluent*>& _actionFluents);
    void calcPossiblyLegalActionStates(int actionsToSchedule, std::list<std::vector<int> >& result,
                                       std::vector<int> addTo = std::vector<int>());

    void initializeStateFluentHashKeys();
    void initializeStateHashKeys();
    void initializeHashKeysOfStatesAsProbabilityDistributions();
    void initializeOtherStuff();

    //internal function for action pruning and reward lock detection
    void checkForReasonableActions(State const& state, std::vector<int>& res);
    bool checkDeadLock(State const& state);
    bool checkGoal(State const& state);

    //BDD related functions
    void stateToBDD(State const& state, bdd& res);
    bool BDDIncludes(bdd BDD, State const& state);

    ProstPlanner* planner;

    std::vector<ActionFluent*> actionFluents;
    std::vector<ActionState> actionStates;
    int numberOfActions;

    bool isDeterministic;
    bool noopIsOptimalFinalAct;
    bool isPruningEquivalentToDet;

    State initialState;

    int firstProbabilisticVarIndex;
    int numberOfConcurrentActions;
    int horizon;
    double discountFactor;
    int stateSize;

    bool stateHashingPoss;
    //TODO: This is ugly as it only belongs to probabilistic task ->
    //Maybe we should separate probabilistic and deterministic
    //planning task by creating two classes (might also be more
    //efficient for state transition)
    bool probStateHashingPoss;

    mutable double randNum;

    //reward lock detection related stuff
    bdd cachedDeadLocks;
    bdd cachedGoals;
    bool useRewardLockDetection;

    //applicable and reasonable action stuff
    std::map<State, std::vector<int>, State::CompareIgnoringRemainingSteps> actionsToExpandCache;
    bool cacheActionsToExpand;
    bool useReasonableActionPruning;

    std::vector<ConditionalProbabilityFunction*> CPFs;
    ConditionalProbabilityFunction* rewardCPF;
    std::vector<StateActionConstraint*> SACs;

    //TODO: This is very very ugly, but cpfs, planning tasks and
    //states are very tightly coupled. Nevertheless, there must be a
    //way to get rid of this, even if it takes some work!
    friend class ConditionalProbabilityFunction;

    //the CPF indexToStateFluentHashKeyMap[i][j].first depends on
    //variable i, and is updated with
    //indexToStateFluentHashKeyMap[i][j].second
    std::vector<std::vector<std::pair<int,long> > > indexToStateFluentHashKeyMap;
    std::vector<std::vector<std::pair<int,long> > > indexToKleeneStateFluentHashKeyMap;

public:
    //Caches known state evaluations. Cache with care!
    std::map<State, double, State::CompareConsideringRemainingSteps> stateValueCache;
};

#endif
