#include "planning_task.h"

#include "prost_planner.h"
#include "logical_expressions.h"
#include "conditional_probability_functions.h"

#include <cassert>
#include <iostream>

using namespace std;

/*****************************************************************
                         Initialization
*****************************************************************/

void PlanningTask::initialize(vector<ActionFluent*>& _actionFluents, vector<ConditionalProbabilityFunction*>& _CPFs, 
                              vector<StateActionConstraint*>& _SACs, int _numberOfConcurrentActions,
                              int _horizon, double _discountFactor, map<string,int>& stateVariableIndices) {
    //set member vars
    SACs = _SACs;
    numberOfConcurrentActions = _numberOfConcurrentActions;
    horizon = _horizon;
    discountFactor = _discountFactor;

    //initialize CPFs and initial state
    initializeCPFs(_CPFs);

    //initialize actions
    initializeActions(_actionFluents);

    //prepare hash key calculation
    initializeStateFluentHashKeys();
    initializeStateHashKeys();
    initializeHashKeysOfStatesAsProbabilityDistributions();

    //calculate hash keys of initial state
    calcStateFluentHashKeys(initialState);
    calcStateHashKey(initialState);

    //do the rest
    initializeOtherStuff();

    //set mapping of variables to variable names
    for(unsigned int i = 0; i < stateSize; ++i) {
        assert(stateVariableIndices.find(CPFs[i]->head->name) == stateVariableIndices.end());
        stateVariableIndices[CPFs[i]->head->name] = i;
    }
}

void PlanningTask::initializeCPFs(vector<ConditionalProbabilityFunction*>& _CPFs) {
    //calculate basic properties
    for(unsigned int i = 0; i < _CPFs.size(); ++i) {
        _CPFs[i]->initialize();
    }

    //order CPFs and get reward CPF
    vector<ConditionalProbabilityFunction*> probCPFs;
    for(unsigned int i = 0; i < _CPFs.size(); ++i) {
        if(_CPFs[i]->head == StateFluent::rewardInstance()) {
            rewardCPF = _CPFs[i];
        } else if(_CPFs[i]->isProbabilistic()) {
            probCPFs.push_back(_CPFs[i]);
        } else {
            CPFs.push_back(_CPFs[i]);
        }
    }
    assert(rewardCPF);

    //set constants based on CPFs
    if(probCPFs.empty()) {
        isDeterministic = true;
    } else {
        isDeterministic = false;
    }

    //set variables that depend on CPFs
    firstProbabilisticVarIndex = (int)CPFs.size();
    CPFs.insert(CPFs.end(), probCPFs.begin(), probCPFs.end());
    stateSize = (int)CPFs.size();
 
    //set indices
    for(unsigned int i = 0; i < stateSize; ++i) {
        CPFs[i]->head->index = i;
        CPFs[i]->index = i;
    }
    rewardCPF->index = (int)CPFs.size();

    //set initial state
    initialState = State(stateSize,horizon);
    for(unsigned int i = 0; i < stateSize; ++i) {
        initialState[i] = CPFs[i]->head->initialValue;
    }
}

void PlanningTask::initializeActions(vector<ActionFluent*>& _actionFluents) {
    //initialize action fluents
    actionFluents = _actionFluents;
    for(unsigned int i = 0; i < actionFluents.size(); ++i) {
        actionFluents[i]->index = i;
    }

    list<vector<int> > actionCombinations;    
    calcPossiblyLegalActionStates(numberOfConcurrentActions, actionCombinations);

    double res = 0.0;
    State current(initialState);
    State next(stateSize);

    set<ActionState> legalActions;

    for(list<vector<int> >::iterator it = actionCombinations.begin(); it != actionCombinations.end(); ++it) {
        vector<int>& tmp = *it;

        ActionState actionState((int)actionFluents.size());
        for(unsigned int i = 0; i < tmp.size(); ++i) {
            actionState[tmp[i]] = 1;
        }

        bool isLegal = true;
        for(unsigned int i = 0; i < SACs.size(); ++i) {
            SACs[i]->sac->evaluate(res, current, next, actionState);
            if(MathUtils::doubleIsEqual(res,0.0)) {
                isLegal = false;
                break;
            }
        }

        if(isLegal) {
            legalActions.insert(actionState);
        }
    }

    for(set<ActionState>::iterator it = legalActions.begin(); it != legalActions.end(); ++it) {
        actionStates.push_back(*it);
    }

    for(unsigned int i = 0; i < actionStates.size(); ++i) {
        actionStates[i].index = i;
        actionStates[i].calculateProperties(actionFluents);
    }

    numberOfActions = (int)actionStates.size();
}

void PlanningTask::calcPossiblyLegalActionStates(int actionsToSchedule, list<vector<int> >& result, vector<int> addTo) {
    int nextVal = 0;
    result.push_back(addTo);

    if(!addTo.empty()) {
        nextVal = addTo[addTo.size()-1]+1;
    }

    for(unsigned int i = nextVal; i < actionFluents.size(); ++i) {
        vector<int> copy = addTo;
        copy.push_back(i);
        if(actionsToSchedule > 0) {
            calcPossiblyLegalActionStates(actionsToSchedule -1, result, copy);
        }
    }
}

void PlanningTask::initializeStateFluentHashKeys() {
    indexToStateFluentHashKeyMap = vector<vector<pair<int, long> > >(stateSize);
    indexToKleeneStateFluentHashKeyMap = vector<vector<pair<int, long> > >(stateSize);

    for(unsigned int i = 0; i < stateSize; ++i) {
        CPFs[i]->initializeStateFluentHashKeys();
        CPFs[i]->initializeKleeneStateFluentHashKeys();
    }

    rewardCPF->initializeStateFluentHashKeys();
    rewardCPF->initializeKleeneStateFluentHashKeys();
}

void PlanningTask::initializeStateHashKeys() {
    if(stateSize < (planner->getBitSize() - 1)) {
        stateHashingPoss = true;
        for(unsigned int i = 0; i < stateSize; ++i) {
            CPFs[i]->hashKeyBase = MathUtils::twoToThePowerOf(CPFs[i]->index);
        }
    } else {
        stateHashingPoss = false;
        for(unsigned int i = 0; i < stateSize; ++i) {
            CPFs[i]->hashKeyBase = 0;
        }
    }
}

void PlanningTask::initializeHashKeysOfStatesAsProbabilityDistributions() {
    //calculate domains
    for(unsigned int i = 0; i < stateSize; ++i) {
        CPFs[i]->calculateDomain(actionStates);
    }
    rewardCPF->calculateDomain(actionStates);

    //assign hash key bases
    int nextBase = 1;
    for(unsigned int i = 0; i <  stateSize; ++i) {
        CPFs[i]->probHashKeyBase = nextBase;
        nextBase *= CPFs[i]->probDomainSize;
    }

    //check if hash key bases grow (otherwise prob state hashing is not possible)
    probStateHashingPoss = true;
    for(unsigned int i = 0; i < stateSize-1; ++i) {
        if(CPFs[i]->probHashKeyBase > CPFs[i+1]->probHashKeyBase) {
            probStateHashingPoss = false;
        }
    }
    if(CPFs[stateSize-1]->probHashKeyBase > nextBase) {
        probStateHashingPoss = false;
    }

    //multiply values with the hash key base if prob state hashing is possible
    if(!probStateHashingPoss) {
        for(unsigned int i = 0; i <  stateSize; ++i) {
            CPFs[i]->probHashKeyBase = 0;
        }
    } else {
        for(unsigned int i = 0; i < stateSize; ++i) {
            for(map<double, int>::iterator it = CPFs[i]->probDomainMap.begin(); it != CPFs[i]->probDomainMap.end(); ++it) {
                it->second *= (int)CPFs[i]->probHashKeyBase;
            }
        }
    }
}

void PlanningTask::initializeOtherStuff() {
    isPruningEquivalentToDet = true;
    for(unsigned int i = firstProbabilisticVarIndex; i < stateSize; ++i) {
        if(CPFs[i]->probDomainSize > 2) {
            isPruningEquivalentToDet = false;
            break;
        } else if(CPFs[i]->probDomainSize == 2) {
            if(MathUtils::doubleIsGreater(CPFs[i]->probDomainMap.begin()->first,0.5) &&
               MathUtils::doubleIsGreater(CPFs[i]->probDomainMap.rbegin()->first,0.5)) {
                isPruningEquivalentToDet = false;
                break;
            }

            if(MathUtils::doubleIsSmallerOrEqual(CPFs[i]->probDomainMap.begin()->first,0.5) &&
               MathUtils::doubleIsSmallerOrEqual(CPFs[i]->probDomainMap.rbegin()->first,0.5)) {
                isPruningEquivalentToDet = false;
                break;
            }
        } //otherwise probDomainSize is 1 and the variable is pruning equivalent
    }

    noopIsOptimalFinalAct = false;
    if(rewardCPF->doesNotDependPositivelyOnActions() && rewardCPF->isNextStateIndependent()) {
        noopIsOptimalFinalAct = true;
    }

    //TODO: Make sure these numbers make sense!
    bdd_init(5000000,20000);
    bdd_setvarnum(stateSize);

    cachedDeadLocks = bddfalse;
    cachedGoals = bddfalse;
}

/*****************************************************************
                         Determinization
*****************************************************************/

PlanningTask* PlanningTask::determinizeMostLikely(UnprocessedPlanningTask* task) {
    //Create the determinization as a copy of this
    PlanningTask* detPlanningTask = new PlanningTask(*this);

    //The created planning task is deterministic
    detPlanningTask->isDeterministic = true;

    //clear the CPFs and determinize them
    detPlanningTask->CPFs.clear();
    //map<StateFluent*,NumericConstant*> replacements;
    NumericConstant* randomNumberReplacement = task->getConstant(0.5);

    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        ConditionalProbabilityFunction* detCPF = CPFs[i]->determinizeMostLikely(randomNumberReplacement, detPlanningTask, task);
        detPlanningTask->CPFs.push_back(detCPF);
    }

    //we currently assume deterministic reward CPFs. To change this,
    //we have to determinize the reward CPF here as well (I am not
    //sure about the consequences of this elsewhere, which is why I
    //leave it as is)
    assert(!rewardCPF->isProbabilistic());

    //there are no probabilistic variables
    detPlanningTask->firstProbabilisticVarIndex = stateSize;

    //as detPlanningTask is deterministic, it is obviously pruning
    //equivalent to itself
    detPlanningTask->isPruningEquivalentToDet = true;

    //while this should not be needed, prob state hashing is possible
    //in a deterministic planning task when state hashing is possible
    //in a probabilistic one (as all domains of variables are {0,1})
    detPlanningTask->probStateHashingPoss = detPlanningTask->stateHashingPoss;

    return detPlanningTask;
}

/*****************************************************************
                             Learning
*****************************************************************/


bool PlanningTask::learn(vector<State> const& trainingSet) {
    //learn usage of reasonable action pruning and reward lock detection
    vector<int> actionsToExpand(getNumberOfActions(),-1);
    bool rewardLockFound = false;
    bool unreasonableActionFound = false;

    useRewardLockDetection = true;
    useReasonableActionPruning = true;

    for(unsigned int i = 0; i < trainingSet.size(); ++i) {
        assert(trainingSet[i].remSteps >= 0);
        setActionsToExpand(trainingSet[i], actionsToExpand);
        if(!unreasonableActionFound) {
            for(unsigned int i = 0; i < actionsToExpand.size(); ++i) {
                if(actionsToExpand[i] != i) {
                    unreasonableActionFound = true;
                }
            }
        }

        //TODO: Our implementation only works if the reward is next state independent
        if(rewardCPF->isNextStateIndependent() && isARewardLock(trainingSet[i])) {
            rewardLockFound = true;
        }
    }

    if(unreasonableActionFound) {
        cout << (isDeterministic ? " DET: " : "PROB: ") << ": Reasonable action pruning enabled!" << endl;
    } else {
        useReasonableActionPruning = false;
        cout << (isDeterministic ? " DET: " : "PROB: ") << ": Reasonable action pruning disabled!" << endl;
    }

    if(rewardLockFound) {
        cout << (isDeterministic ? " DET: " : "PROB: ") << ": Reward lock detection enabled!" << endl;
    } else {
        useRewardLockDetection = false;
        cout << (isDeterministic ? " DET: " : "PROB: ") << ": Reward lock detection disabled!" << endl;
    }
    return LearningComponent::learn(trainingSet);
}


/******************************************************************
               Applicable Actions and Action Pruning
******************************************************************/

void PlanningTask::setActionsToExpand(State const& state, vector<int>& res) {
    assert(res.size() == getNumberOfActions());

    //TODO: Check dynamic state action constraints and mark
    //unapplicable operators

    if(!useReasonableActionPruning) {
        for(unsigned int i = 0; i < res.size(); ++i) {
            res[i] = i;
        }
        return;
    }
 
    map<State, vector<int> >::iterator it = actionsToExpandCache.find(state);
    if(it != actionsToExpandCache.end()) {
        assert(it->second.size() == res.size());
        for(unsigned int i = 0; i < res.size(); ++i) {
            res[i] = it->second[i];
        }
    } else {
        checkForReasonableActions(state, res);

        if(cacheActionsToExpand) {
            actionsToExpandCache[state] = res;
        }
    }
}

inline void PlanningTask::checkForReasonableActions(State const& state, vector<int>& res) {
    State nxt(stateSize);
    map<State, int, State::CompareIgnoringRemainingSteps> childStates;

    for(int actionIndex = 0; actionIndex < getNumberOfActions(); ++actionIndex) {
        calcSuccessorAsProbabilityDistribution(state, actionIndex, nxt);
        calcHashKeyOfProbabilityDistribution(nxt);
        if(childStates.find(nxt) != childStates.end()) {
            //TODO: Check if rewardCPF->doesNotDependPositivelyOnActions(), and if NOT
            //we can NOT prune if both variables appear positively in the rewardCPF!
            res[actionIndex] = childStates[nxt];
        } else {
            childStates[nxt] = actionIndex;
            res[actionIndex] = actionIndex;
        }
    }
}

/******************************************************************
            Reward Lock Detection (including BDD Stuff)
******************************************************************/

bool PlanningTask::isARewardLock(State const& current) {
    if(!useRewardLockDetection) {
        return false;
    }

    //calculate the reference reward (TODO: Careful, we assume here that NOOP is always applicable!)
    assert(rewardCPF->isNextStateIndependent());

    State succ(stateSize);
    double reward = 0.0;
    calcReward(current, 0, succ, reward);

    if(isMinReward(reward)) {
        //Check if current is known to be a dead lock
        if(BDDIncludes(cachedDeadLocks, current)) {
            return true;
        }

        //convert to Kleene state (necessary for hash keys)
        State currentInKleene = toKleeneState(current);

        //check reward lock on Kleene state
        return checkDeadLock(currentInKleene);
    } else if(isMaxReward(reward)) {
        //Check if current is known to be a goal
        if(BDDIncludes(cachedGoals, current)) {
            return true;
        }

        //convert to Kleene state (necessary for hash keys)
        State currentInKleene = toKleeneState(current);

        return checkGoal(currentInKleene);
    }

    return false;
}

bool PlanningTask::checkDeadLock(State const& state) {
    //calc noop successor and check if reward is identical
    State mergedSuccs(stateSize);
    calcKleeneSuccessor(state, 0, mergedSuccs);
    double reward = 0.0;
    calcKleeneReward(state, 0, mergedSuccs, reward);

    if(!isMinReward(reward)) {
        return false;
    }

    for(unsigned int actionIndex = 1; actionIndex < getNumberOfActions(); ++actionIndex) {
        //calc Kleene successor of applying action actionIndex, and check if reward is identical
        State succ(stateSize);
        calcKleeneSuccessor(state, actionIndex, succ);
        calcKleeneReward(state, actionIndex, succ, reward);

        if(!isMinReward(reward)) {
            return false;
        }

        //merge with noop successor
        mergeKleeneStates(succ, mergedSuccs);
    }

    //refresh state fluent hash keys
    calcKleeneStateFluentHashKeys(mergedSuccs);

    //Check if the successor is unchanged, otherwise continue to check if it is a reward lock
    if(mergedSuccs.isEqualIgnoringRemainingStepsTo(state) || checkDeadLock(mergedSuccs)) {
        bdd stateAsBDD;
        stateToBDD(state, stateAsBDD);
        cachedDeadLocks |= stateAsBDD;

        return true;
    }
    return false;
}


//We underapproximate the set of goals, as we only consider those
//where applying noop makes us stay in the reward lock
bool PlanningTask::checkGoal(State const& state) {
    State succ(stateSize);
    calcKleeneSuccessor(state, 0, succ);
    double reward = 0.0;
    calcKleeneReward(state, 0, succ, reward);

    if(!isMaxReward(reward)) {
        return false;
    }

    //add parent to successor
    mergeKleeneStates(state, succ);

    //update state fluent hash keys
    calcKleeneStateFluentHashKeys(succ);

    if(succ.isEqualIgnoringRemainingStepsTo(state) || checkGoal(succ)) {
        bdd stateAsBDD;
        stateToBDD(state, stateAsBDD);
        cachedGoals |= stateAsBDD;

        return true;
    }
    return false;
}

inline void PlanningTask::stateToBDD(State const& state, bdd& res) {
    res = bddtrue;
    for(unsigned int i = 0; i < stateSize; ++i) {
        if(MathUtils::doubleIsEqual(state[i],1.0)) {
            res &= bdd_ithvar(i);
        } else if(MathUtils::doubleIsEqual(state[i],0.0)) {
            res &= bdd_nithvar(i);
        }
    }
}

bool PlanningTask::BDDIncludes(bdd BDD, State const& state) {
    while(true) {
        if(BDD == bddtrue) {            
            return true;
        } else if(BDD == bddfalse) {
            return false;
        }

        if(MathUtils::doubleIsEqual(state[bdd_var(BDD)],1.0)) {
            BDD = bdd_high(BDD);
        } else {
            BDD = bdd_low(BDD);
        }
    }
}

void PlanningTask::disableCaching() {
    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        CPFs[i]->disableCaching();
    }
    rewardCPF->disableCaching();
    cacheActionsToExpand = false;
}

void PlanningTask::print(ostream& out) const {
    out << "This task is " << (isDeterministic? "deterministic." : "probabilistic.") << endl;
    out << "--------StateActionConstraints--------" << endl<< endl;
    for(unsigned int i = 0; i < SACs.size(); ++i) {
        SACs[i]->print(out);
        out << endl;
    }
    out << endl << endl;

    out << "----------------Actions---------------" << endl << endl;
    out << "Action fluents: " << endl;
    for(unsigned int i = 0; i < actionFluents.size(); ++i) {
        actionFluents[i]->print(out);
        out << endl;
    }
    out << "---------------" << endl << endl;
    out << "Legal Action Combinations: " << endl;
    for(unsigned int i = 0; i < actionStates.size(); ++i) {
        printAction(out, i);
        out << endl;
    }
    out << endl;
    out << "-----------------CPFs-----------------" << endl << endl;
    for(unsigned int i = 0; i < stateSize; ++i) {
        CPFs[i]->print(out);
        out << endl << "--------------" << endl;
    }
    out << endl;

    out << "Reward CPF:" << endl;
    rewardCPF->print(out);
    out << endl << endl;

    for(unsigned int i = 0; i < indexToStateFluentHashKeyMap.size(); ++i) {
        out << "a change of variable " << i << " influences variables ";
        for(unsigned int j = 0; j < indexToStateFluentHashKeyMap[i].size(); ++j) {
            out << indexToStateFluentHashKeyMap[i][j].first << " (" << indexToStateFluentHashKeyMap[i][j].second << ")  ";
        }
        out << endl;
    }

    out << endl;
    for(unsigned int i = 0; i < indexToStateFluentHashKeyMap.size(); ++i) {
        out << "a change of variable " << i << " influences variables in Kleene states ";
        for(unsigned int j = 0; j < indexToKleeneStateFluentHashKeyMap[i].size(); ++j) {
            out << indexToKleeneStateFluentHashKeyMap[i][j].first << " (" << indexToKleeneStateFluentHashKeyMap[i][j].second << ")  ";
        }
        out << endl;
    }

    out << "----------Initial State---------------" << endl << endl;
    printState(out, initialState);
    out << endl;

    out << "This task is " << (isPruningEquivalentToDet? "" : "not ") << "pruning equivalent to its most likely determinization" << endl;
    out << "NOOP as last action is " << (noopIsOptimalFinalAct? "" : "not ") << "always optimal." << endl;
    out << "Deterministic state hashing is " << (stateHashingPoss? "" : "not ") << "possible";
    if(isDeterministic) {
        out << "." << endl;
    } else {
        out << " and probabilistic state hashing is " << (probStateHashingPoss? "" : "not ") << "possible." << endl;
    }
    
}

void PlanningTask::printState(ostream& out, State const& s) const {
    assert(s.state.size() == stateSize);
    for(unsigned int i = 0; i < stateSize; ++i) {
        out << CPFs[i]->head->name << ": " << s[i] << endl;
    }
    out << "Remaining Steps: " << s.remainingSteps() << endl;
    out << "StateHashKey: " << s.hashKey << endl;
}

void PlanningTask::printAction(ostream& out, int const& index) const {
    if(actionStates[index].scheduledActionFluents.empty()) {
        out << "noop() ";
    } else {
        for(unsigned int i = 0; i < actionStates[index].scheduledActionFluents.size(); ++i) {
            out << actionStates[index].scheduledActionFluents[i]->name << " ";
        }
    }
}
