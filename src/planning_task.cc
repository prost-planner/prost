#include "planning_task.h"

#include "logical_expressions.h"
#include "state_action_constraint.h"

#include <cassert>
#include <iostream>

using namespace std;

/*****************************************************************
                         Initialization
*****************************************************************/

void PlanningTask::initialize(vector<ActionFluent*>& _actionFluents, vector<ConditionalProbabilityFunction*>& _CPFs, 
                              vector<StateActionConstraint*>& _SACs, int _numberOfConcurrentActions,
                              int _horizon, double _discountFactor, map<string,int>& stateVariableIndices) {
    //Set member vars
    numberOfConcurrentActions = _numberOfConcurrentActions;
    horizon = _horizon;
    discountFactor = _discountFactor;

    // Initialize SACs, CPFs, initial state and actions
    initializeSACs(_SACs);
    initializeCPFs(_CPFs);
    initializeActions(_actionFluents);

    // Prepare hash key calculation
    initializeStateFluentHashKeys();
    initializeStateHashKeys();
    initializeHashKeysOfStatesAsProbabilityDistributions();

    // Calculate hash keys of initial state
    calcStateFluentHashKeys(initialState);
    calcStateHashKey(initialState);

    // Determine if this task is pruning equivalent to its
    // determinization
    determinePruningEquivalence();

    // Initialize reward and reward lock dependent variables
    initializeRewardDependentVariables();

    // Set mapping of variables to variable names for communication
    // with environment
    for(unsigned int i = 0; i < stateSize; ++i) {
        assert(stateVariableIndices.find(CPFs[i]->head->name) == stateVariableIndices.end());
        stateVariableIndices[CPFs[i]->head->name] = i;
    }
}

void PlanningTask::initializeSACs(vector<StateActionConstraint*>& _SACs) {
    // Initialize SACs, then divide them into dynamic SACs, static
    // SACs and state invariants.
    for(unsigned int index = 0; index < _SACs.size(); ++index) {
        _SACs[index]->initialize();

    	if(_SACs[index]->containsStateFluent()) {
             if(_SACs[index]->containsActionFluent()) {
                 // An SAC that contain both state and action fluents
                 // must be evaluated like a precondition for actions.
                 dynamicSACs.push_back(_SACs[index]);
            } else {
                // An SAC that only contains state fluents represents
                // a state invariant that must be true in every state.
                stateInvariants.push_back(_SACs[index]);
            }
    	} else {
            // An SAC that only contains action fluents is used to
            // statically forbid action combinations.
            staticSACs.push_back(_SACs[index]);
    	}
    }

    numberOfStateFluentHashKeys += dynamicSACs.size();
}

void PlanningTask::initializeCPFs(vector<ConditionalProbabilityFunction*>& _CPFs) {
    // Calculate basic properties
    for(unsigned int index = 0; index < _CPFs.size(); ++index) {
        _CPFs[index]->initialize();
    }

    // Order CPFs and get reward CPF
    vector<ConditionalProbabilityFunction*> probCPFs;
    for(unsigned int index = 0; index < _CPFs.size(); ++index) {
        if(_CPFs[index]->head == StateFluent::rewardInstance()) {
            rewardCPF = _CPFs[index];
        } else if(_CPFs[index]->isProbabilistic()) {
            probCPFs.push_back(_CPFs[index]);
        } else {
            CPFs.push_back(_CPFs[index]);
        }
    }
    assert(rewardCPF);

    // Set constants based on CPFs
    if(probCPFs.empty()) {
        deterministic = true;
    } else {
        deterministic = false;
    }

    // Set variables that depend on CPFs
    firstProbabilisticVarIndex = (int)CPFs.size();
    CPFs.insert(CPFs.end(), probCPFs.begin(), probCPFs.end());
    stateSize = (int)CPFs.size();
 
    // Set indices
    for(unsigned int i = 0; i < stateSize; ++i) {
        CPFs[i]->head->index = i;
    }

    numberOfStateFluentHashKeys += (CPFs.size() + 1);

    // Set initial state
    initialState = State(stateSize, horizon, numberOfStateFluentHashKeys);
    for(unsigned int i = 0; i < stateSize; ++i) {
        initialState[i] = CPFs[i]->head->initialValue;
    }
}

void PlanningTask::initializeActions(vector<ActionFluent*>& _actionFluents) {
    // Initialize action fluents
    actionFluents = _actionFluents;
    for(unsigned int i = 0; i < actionFluents.size(); ++i) {
        actionFluents[i]->index = i;
    }

    // Calculate all possible action combinations with up to
    // numberOfConcurrentActions concurrent actions
    list<vector<int> > actionCombinations;    
    calcPossiblyLegalActionStates(numberOfConcurrentActions, actionCombinations);

    double res = 0.0;
    State current(initialState);

    set<ActionState> legalActions;

    // Remove all illegal action combinations by checking the SACs
    // that are state independent
    for(list<vector<int> >::iterator it = actionCombinations.begin(); it != actionCombinations.end(); ++it) {
        vector<int>& tmp = *it;

        ActionState actionState((int)actionFluents.size());
        for(unsigned int i = 0; i < tmp.size(); ++i) {
            actionState[tmp[i]] = 1;
        }

        bool isLegal = true;
        for(unsigned int i = 0; i < staticSACs.size(); ++i) {
            staticSACs[i]->evaluate(res, current, actionState);
            if(MathUtils::doubleIsEqual(res,0.0)) {
                isLegal = false;
                break;
            }
        }

        if(isLegal) {
            legalActions.insert(actionState);
        }
    }

    // All remaining action states might be legal in some state.
    // initialize the actionStates vector and set their index
    for(set<ActionState>::iterator it = legalActions.begin(); it != legalActions.end(); ++it) {
        actionStates.push_back(*it);
    }

    for(unsigned int i = 0; i < actionStates.size(); ++i) {
        actionStates[i].index = i;
        actionStates[i].calculateProperties(actionFluents, dynamicSACs);
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

    int hashIndex = 0;

    for(; hashIndex < stateSize; ++hashIndex) {
        CPFs[hashIndex]->initializeHashKeys(hashIndex, actionStates, CPFs, indexToStateFluentHashKeyMap, indexToKleeneStateFluentHashKeyMap);
    }
    rewardCPF->initializeHashKeys(hashIndex, actionStates, CPFs, indexToStateFluentHashKeyMap, indexToKleeneStateFluentHashKeyMap);

    for(unsigned int i = 0; i < dynamicSACs.size(); ++i) {
        ++hashIndex;
        dynamicSACs[i]->initializeHashKeys(hashIndex, actionStates, CPFs, indexToStateFluentHashKeyMap, indexToKleeneStateFluentHashKeyMap);
    }
}

// Check if state hashing is possible, and assign hash key bases if so
void PlanningTask::initializeStateHashKeys() {
    stateHashingPossible = true;
    long nextHashKeyBase = 1;
    vector<long> hashKeyBases;
    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        hashKeyBases.push_back(nextHashKeyBase);
        if(!MathUtils::multiplyWithOverflowCheck(nextHashKeyBase, 2)) {
            stateHashingPossible = false;
            return;
        }
    }

    assert(hashKeyBases.size() == CPFs.size());

    for(unsigned int i = 0; i < hashKeyBases.size(); ++i) {
        CPFs[i]->hashKeyBase = hashKeyBases[i];
    }
}

// Check if state hashing is possible with states as probability
// distributuions, and assign prob hash key bases if so
void PlanningTask::initializeHashKeysOfStatesAsProbabilityDistributions() {
    // We first need to compute the possible values to determine the
    // domain size
    for(unsigned int index = 0; index < stateSize; ++index) {
        initializeDomainOfCPF(index);
    }

    // Assign hash key bases
    stateHashingWithStatesAsProbabilityDistributionPossible = true;
    long nextHashKeyBase = 1;
    vector<long> hashKeyBases;
    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        hashKeyBases.push_back(nextHashKeyBase);
        if(!MathUtils::multiplyWithOverflowCheck(nextHashKeyBase, CPFs[i]->probDomainSize)) {
            stateHashingWithStatesAsProbabilityDistributionPossible = false;
            return;
        }
    }

    assert(hashKeyBases.size() == CPFs.size());

    for(unsigned int index = 0; index < hashKeyBases.size(); ++index) {
        CPFs[index]->probHashKeyBase = hashKeyBases[index];
        for(map<double, long>::iterator it = CPFs[index]->probDomainMap.begin(); it != CPFs[index]->probDomainMap.end(); ++it) {
            it->second *= CPFs[index]->probHashKeyBase;
        }
    }
}

set<double> PlanningTask::calculateDomainOfCPF(ConditionalProbabilityFunction* cpf) {
    set<double> result;
    for(unsigned int actionIndex = 0; actionIndex < getNumberOfActions(); ++actionIndex) {
        // Calculate the values that can be achieved if the action
        // with actionIndex is applied and add it to the values of
        // all other actions
        set<double> actionDependentValues;
        cpf->formula->calculateDomain(actionStates[actionIndex], actionDependentValues);
        result.insert(actionDependentValues.begin(), actionDependentValues.end());
    }
    return result;
}

void PlanningTask::initializeDomainOfCPF(int const& index) {
    if(CPFs[index]->isProbabilistic()) {
        set<double> actionIndependentValues = calculateDomainOfCPF(CPFs[index]);

        // Enumerate all values with 0...domainSize. We will later
        // multiply these with the hash key base once it's determined.
        CPFs[index]->probDomainSize = 0;
        for(set<double>::iterator it = actionIndependentValues.begin(); it != actionIndependentValues.end(); ++it) {
            CPFs[index]->probDomainMap[*it] = CPFs[index]->probDomainSize;
            ++CPFs[index]->probDomainSize;
        }
    } else {
        // The domainSize is 2 with possible values 0 and 1.
        CPFs[index]->probDomainSize = 2;
        CPFs[index]->probDomainMap[0.0] = 0;
        CPFs[index]->probDomainMap[1.0] = 1;
    }
}

void PlanningTask::determinePruningEquivalence() {
    // A probabilistic PlanningTask is pruning equivalent to its
    // determinization if:

    // 1. there are no unreasonable actions (as applicability is
    // always equivalent because SACs must be deterministic) 

    // 2. each probabilistic variable has a domain size of at most 2,
    // and if one value is higher and one smaller or equal to 0.5. If
    // it is pruning equivalent, the determinization is used for
    // action pruning.

    // TODO: only works with the most-likely determinization! Make
    // PlanningTask an abstract superclass with derived classes
    // ProbabilisticTask and MostLikelyDeterminization.

    isPruningEquivalentToDet = true;
    if(hasUnreasonableActions) {
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
    }
}

void PlanningTask::initializeRewardDependentVariables() {
    set<double> rewardValues = calculateDomainOfCPF(rewardCPF);
    rewardCPF->minVal = *(rewardValues.begin());
    rewardCPF->maxVal = *(rewardValues.rbegin());

    noopIsOptimalFinalAction = false;
    useRewardLockDetection = true;

    // TODO: goalTestActionIndex should actually be a vector that
    // contains all actions indices that could potentially be the
    // action that is necessary to stay in a goal. Currently, we just
    // set it to noop or to -1, as there is no domain with a
    // "stayInGoal" action that is different from noop. Moreover, it
    // could also be that there are several such actions, e.g.
    // stayInGoal_1 and stayInGoal_2 which are used for different goal
    // states. What we do currently is sound but makes the
    // rewardLockDetection even more incomplete.
    if(rewardCPF->hasPositiveActionDependencies()) {
        goalTestActionIndex = -1;
        useRewardLockDetection = false;
    } else {
        goalTestActionIndex = 0;

        if(actionStates[0].scheduledActionFluents.empty() && actionStates[0].relevantSACs.empty()) {
            // If no action fluent occurs positively in the reward, if
            // noop is not forbidden due to static SACs, and if noop
            // has no SACs that might make it inapplicable, noop is
            // always at least as good as other actions in the final
            // state transition.
            noopIsOptimalFinalAction = true;
        }
    }

    // TODO: Make sure these numbers make sense!
    bdd_init(5000000,20000);
    bdd_setvarnum(stateSize);
}

/*****************************************************************
                         Determinization
*****************************************************************/

// TODO: Determinization is currently a very dirty process that is
// very error prone (and possibly contains several). We should really
// split planning task to different kinds of classes to make
// everything nice and clean.
PlanningTask* PlanningTask::determinizeMostLikely(UnprocessedPlanningTask* task) {
    // There is nothing to do in deterministic problems
    if(deterministic) {
        return this;
    }

    // Create the determinization as a copy of this
    PlanningTask* detPlanningTask = new PlanningTask(*this);

    // Clear the CPFs and determinize them
    detPlanningTask->CPFs.clear();
    NumericConstant* randomNumberReplacement = task->getConstant(0.5);

    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        ConditionalProbabilityFunction* detCPF = CPFs[i]->determinizeMostLikely(randomNumberReplacement, task);
        detPlanningTask->CPFs.push_back(detCPF);
    }

    // We currently assume deterministic reward CPFs. To change this,
    // we have to determinize the reward CPF here as well (I am not
    // sure about the consequences of this elsewhere, which is why I
    // leave it as is)
    assert(!rewardCPF->isProbabilistic());

    // There are no probabilistic variables
    detPlanningTask->firstProbabilisticVarIndex = stateSize;

    // As detPlanningTask is deterministic, it is obviously pruning
    // equivalent to itself.Nevertheless, this is never used.
    detPlanningTask->isPruningEquivalentToDet = true;

    // DEPRECATED COMMENT: While this should not be needed, prob state
    // hashing is possible in a deterministic planning task when state
    // hashing is possible in a probabilistic one (as all domains of
    // variables are {0,1})

    // UPDATED COMMENT: Actually, we could convert the planning task
    // such that the above statement is true, but then we have
    // different hash keys for the same CPF in the two tasks (because
    // the bases are different), which results in errors when we use
    // the cache. For now, we leave it this way, even though in some
    // domains we could use state hashing for states as probability
    // distribution if implemented correctly but cannot currently
    // (with the below commented). TODO

    //detPlanningTask->stateHashingWithStatesAsProbabilityDistributionPossible = detPlanningTask->stateHashingPossible;

    // The created planning task is now deterministic
    detPlanningTask->deterministic = true;

    return detPlanningTask;
}

/*****************************************************************
                             Learning
*****************************************************************/

bool PlanningTask::learn(vector<State> const& trainingSet) {
    if(deterministic) {
        cout << " DET: learning..." << endl;
    } else {
        cout << "PROB: learning..." << endl;
    }
    // Learn if reasonable action pruning and reward lock detection
    // might be needed in this planning task

    // This will be set if one of the methods was applied successfully
    bool rewardLockFound = false;
    bool unreasonableActionFound = false;

    for(unsigned int stateIndex = 0; stateIndex < trainingSet.size(); ++stateIndex) {
        assert(trainingSet[stateIndex].remSteps >= 0);

        // Check if this training state has unreasonable actions
        vector<int> applicableActions = getApplicableActions(trainingSet[stateIndex]);

        for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
            if((applicableActions[actionIndex] != actionIndex) && (applicableActions[actionIndex] != -1)) {
                unreasonableActionFound = true;
            }
        }

        // Check if this training state is a reward lock
        if(isARewardLock(trainingSet[stateIndex])) {
            rewardLockFound = true;
        }
    }

    // Set the variables that control action pruning and reward lock
    // detection
    hasUnreasonableActions = unreasonableActionFound;
    useRewardLockDetection = rewardLockFound;

    if(deterministic) {
        cout << " DET: ...finished" << endl;
    } else {
        cout << "PROB: ...finished" << endl;
    }
    return LearningComponent::learn(trainingSet);
}


/******************************************************************
               Applicable Actions and Action Pruning
******************************************************************/

vector<int> PlanningTask::getApplicableActions(State const& state) {
    vector<int> res(getNumberOfActions(), 0);

    if(hasUnreasonableActions) {
        map<State, vector<int> >::iterator it = applicableReasonableActionsCache.find(state);
        if(it != applicableReasonableActionsCache.end()) {
            assert(it->second.size() == res.size());
            for(unsigned int i = 0; i < res.size(); ++i) {
                res[i] = it->second[i];
            }
        } else {
            setApplicableReasonableActions(state, res);

            if(cacheApplicableActions) {
                applicableReasonableActionsCache[state] = res;
            }
        }
    } else {
        map<State, vector<int> >::iterator it = applicableActionsCache.find(state);
        if(it != applicableActionsCache.end()) {
            assert(it->second.size() == res.size());
            for(unsigned int i = 0; i < res.size(); ++i) {
                res[i] = it->second[i];
            }
        } else {
            setApplicableActions(state, res);

            if(cacheApplicableActions) {
                applicableActionsCache[state] = res;
            }
        }
    }
    return res;
}

inline void PlanningTask::setApplicableReasonableActions(State const& state, std::vector<int>& res) const {
    map<State, int, State::CompareIgnoringRemainingSteps> childStates;

    // TODO: Check if there are positive actions in the reward CPF, as
    // we are not allowed to prune an action that occurs positively in
    // the rewardCPF!

    for(unsigned int actionIndex = 0; actionIndex < getNumberOfActions(); ++actionIndex) {
        if(actionStates[actionIndex].isApplicable(state)) {
            State nxt(stateSize, state.remainingSteps()-1, numberOfStateFluentHashKeys);
            // This action is applicable
            calcSuccessorAsProbabilityDistribution(state, actionIndex, nxt);
            calcHashKeyOfProbabilityDistribution(nxt);
            if(childStates.find(nxt) == childStates.end()) {
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
}

inline void PlanningTask::setApplicableActions(State const& state, vector<int>& res) const {
    for(unsigned int actionIndex = 0; actionIndex < getNumberOfActions(); ++actionIndex) {
        if(!actionStates[actionIndex].isApplicable(state)) {
            res[actionIndex] = -1;
        } else {
            res[actionIndex] = actionIndex;
        }
    }
}

/******************************************************************
            Reward Lock Detection (including BDD Stuff)
******************************************************************/

// Currently, we only consider goals and dead ends (i.e., reward locks
// with min or max reward). This makes sense on the IPPC 2011 domains,
// yet we might want to change it in the future so keep an eye on it.
// Nevertheless, isARewardLock is sound as is (and incomplete
// independently from this decision).
bool PlanningTask::isARewardLock(State const& current) {
    if(!useRewardLockDetection) {
        return false;
    }

    assert(goalTestActionIndex >= 0);

    // Calculate the reference reward
    double reward = 0.0;
    calcReward(current, goalTestActionIndex, reward);

    if(isMinReward(reward)) {
        // Check if current is known to be a dead end
        if(BDDIncludes(cachedDeadEnds, current)) {
            return true;
        }

        // Convert to Kleene state (necessary for hash keys)
        State currentInKleene = toKleeneState(current);

        // Check reward lock on Kleene state
        return checkDeadEnd(currentInKleene);
    } else if(isMaxReward(reward)) {
        // Check if current is known to be a goal
        if(BDDIncludes(cachedGoals, current)) {
            return true;
        }

        // Convert to Kleene state (necessary for hash keys)
        State currentInKleene = toKleeneState(current);

        return checkGoal(currentInKleene);
    }

    return false;
}

bool PlanningTask::checkDeadEnd(State const& state) {
    // TODO: We do currently not care about action applicability.
    // Nevertheless, the results remain sound, as we only check too
    // many actions (it might be the case that we think some state is
    // not a dead end even though it is. This is because the action
    // that would make us leave the dead end is actually
    // inapplicable).

    // Calc noop successor
    State mergedSuccs(stateSize, -1, numberOfStateFluentHashKeys);
    double reward = 0.0;
    calcKleeneSuccessor(state, 0, mergedSuccs);
    calcKleeneReward(state, 0, reward);

    // If reward is not minimal this is not a dead end
    if(!isMinReward(reward)) {
        return false;
    }

    for(unsigned int actionIndex = 1; actionIndex < getNumberOfActions(); ++actionIndex) {
        // Calc Kleene successor of applying action actionIndex
        State succ(stateSize, -1, numberOfStateFluentHashKeys);
        calcKleeneSuccessor(state, actionIndex, succ);
        calcKleeneReward(state, actionIndex, reward);

        // If reward is not minimal this is not a dead end
        if(!isMinReward(reward)) {
            return false;
        }

        // Merge with previous successors
        mergeKleeneStates(succ, mergedSuccs);
    }

    // Calculate state fluent hash keys
    calcKleeneStateFluentHashKeys(mergedSuccs);

    // Check if the merged successor is equivalent to state, otherwise
    // continue to check if this is a dead end
    if(mergedSuccs.isEqualIgnoringRemainingStepsTo(state) || checkDeadEnd(mergedSuccs)) {
        bdd stateAsBDD;
        stateToBDD(state, stateAsBDD);
        cachedDeadEnds |= stateAsBDD;
        return true;
    }
    return false;
}

// We underapproximate the set of goals, as we only consider those
// where applying goalTestActionIndex makes us stay in the reward lock. 
bool PlanningTask::checkGoal(State const& state) {
    State succ(stateSize, -1, numberOfStateFluentHashKeys);
    double reward = 0.0;

    calcKleeneSuccessor(state, goalTestActionIndex, succ);
    calcKleeneReward(state, goalTestActionIndex, reward);

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

/******************************************************************
                               Caching
******************************************************************/

void PlanningTask::disableCaching() {
    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        CPFs[i]->disableCaching();
    }
    rewardCPF->disableCaching();
    cacheApplicableActions = false;
}

/******************************************************************
               Calculation of Final Reward and Action
******************************************************************/

void PlanningTask::calcOptimalFinalReward(State const& current, double& reward) {
    if(noopIsOptimalFinalAction) {
        return calcReward(current, 0, reward);
    }

    // Get applicable actions
    vector<int> applicableActions = getApplicableActions(current);

    if(rewardCPF->isActionIndependent()) {
        // If no action fluent occurs in the reward ,all rewards are
        // the same and we only need to find an applicable action
        for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
            if(applicableActions[actionIndex] == actionIndex) {
                return calcReward(current, actionIndex, reward);
            }
        }
        assert(false);
    }

    // Otherwise we compute which action yields the highest reward

    reward = -numeric_limits<double>::max();
    double tmpReward = 0.0;

    for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
        if(applicableActions[actionIndex] == actionIndex) {
            calcReward(current, actionIndex, tmpReward);

            if(MathUtils::doubleIsGreater(tmpReward, reward)) {
                reward = tmpReward;
            }
        }
    }
}

int PlanningTask::getOptimalFinalActionIndex(State const& current) {
    if(noopIsOptimalFinalAction) {
        return 0;
    }

    // Get applicable actions
    vector<int> applicableActions = getApplicableActions(current);

    if(rewardCPF->isActionIndependent()) {
        // If no action fluent occurs in the reward ,all rewards are
        // the same and we only need to find an applicable action
        for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
            if(applicableActions[actionIndex] == actionIndex) {
                return actionIndex;
            }
        }
        assert(false);
        return -1;
    }

    // Otherwise we compute which action yields the highest reward
    double reward = -numeric_limits<double>::max();
    double tmpReward = 0.0;
    int optimalFinalActionIndex = -1;

    for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
        if(applicableActions[actionIndex] == actionIndex) {
            calcReward(current, actionIndex, tmpReward);

            if(MathUtils::doubleIsGreater(tmpReward, reward)) {
                reward = tmpReward;
                optimalFinalActionIndex = actionIndex;
            }
        }
    }

    return optimalFinalActionIndex;
}

/******************************************************************
                            Printers
******************************************************************/

void PlanningTask::print(ostream& out) const {
    out << "This task is " << (deterministic? "deterministic." : "probabilistic.") << endl;

    out << "----------------Actions---------------" << endl << endl;
    out << "Action fluents: " << endl;
    for(unsigned int index = 0; index < actionFluents.size(); ++index) {
        actionFluents[index]->print(out);
        out << endl;
    }
    out << "---------------" << endl << endl;
    out << "Legal Action Combinations: " << endl;
    for(unsigned int index = 0; index < actionStates.size(); ++index) {
        printActionInDetail(out, index);
        out << "---------------" << endl;
    }
    out << endl;
    out << "-----------------CPFs-----------------" << endl << endl;
    for(unsigned int index = 0; index < stateSize; ++index) {
        printCPFInDetail(out, index);
        out << endl << "--------------" << endl;
    }

    out << endl << "Reward CPF:" << endl;
    printRewardCPFInDetail(out);
    out << endl << endl;

    out << "------State Fluent Hash Key Map-------" << endl << endl;

    for(unsigned int varIndex = 0; varIndex < indexToStateFluentHashKeyMap.size(); ++varIndex) {
        out << "a change of variable " << varIndex << " influences variables ";
        for(unsigned int influencedVarIndex = 0; influencedVarIndex < indexToStateFluentHashKeyMap[varIndex].size(); ++influencedVarIndex) {
            out << indexToStateFluentHashKeyMap[varIndex][influencedVarIndex].first << " (" 
                << indexToStateFluentHashKeyMap[varIndex][influencedVarIndex].second << ")  ";
        }
        out << endl;
    }

    out << endl;
    for(unsigned int varIndex = 0; varIndex < indexToStateFluentHashKeyMap.size(); ++varIndex) {
        out << "a change of variable " << varIndex << " influences variables in Kleene states ";
        for(unsigned int influencedVarIndex = 0; influencedVarIndex < indexToKleeneStateFluentHashKeyMap[varIndex].size(); ++influencedVarIndex) {
            out << indexToKleeneStateFluentHashKeyMap[varIndex][influencedVarIndex].first << " ("
                << indexToKleeneStateFluentHashKeyMap[varIndex][influencedVarIndex].second << ")  ";
        }
        out << endl;
    }

    out << endl;
    out << "-----------------SACs-----------------" << endl << endl;
    out << "Static SACs: " << endl;
    for(unsigned int index = 0; index < staticSACs.size(); ++index) {
        printStaticSACInDetail(out, index);
        out << endl << "--------------" << endl;
    }

    out << "Dynamic SACs: " << endl;
    for(unsigned int index = 0; index < dynamicSACs.size(); ++index) {
        printDynamicSACInDetail(out, index);
        out << endl << "--------------" << endl;
    }

    out << "State invariants: " << endl;
    for(unsigned int index = 0; index < stateInvariants.size(); ++index) {
        printStateInvariantInDetail(out, index);
        out << endl << "--------------" << endl;
    }

    out << "----------Initial State---------------" << endl << endl;
    printState(out, initialState);
    out << endl;

    out << "This task is " << (isPruningEquivalentToDet? "" : "not ") << "pruning equivalent to its most likely determinization" << endl;

    out << "Deterministic state hashing is " << (stateHashingPossible? "" : "not ") << "possible";
    if(deterministic) {
        out << "." << endl;
    } else {
        out << " and state hashing with states as probability distribution is " 
            << (stateHashingWithStatesAsProbabilityDistributionPossible? "" : "not ") << "possible." << endl;
    }

    if(useRewardLockDetection) {
        if(goalTestActionIndex >= 0) {
            out << "Reward lock detection is enabled for goals and dead ends." << endl;
        } else {
            out << "Reward lock detection is enabled for dead ends." << endl;
        }
    } else {
        out << "Reward lock detection is disabled." << endl;
    }

    if(hasUnreasonableActions) {
        out << "This task contains unreasonable actions." << endl;
    } else {
        out << "This task does not contain unreasonable actions." << endl;
    }

    if(noopIsOptimalFinalAction) {
        out << "NOOP is always optimal as final action." << endl;
    } else {
        out << "There is no optimal final action." << endl;
    }
    out << endl;
}

void PlanningTask::printState(ostream& out, State const& state) const {
    assert(state.state.size() == stateSize);
    for(unsigned int index = 0; index < stateSize; ++index) {
        out << CPFs[index]->head->name << ": " << state[index] << endl;
    }
    out << "Remaining Steps: " << state.remainingSteps() << endl;
    out << "StateHashKey: " << state.hashKey << endl;
}

void PlanningTask::printAction(ostream& out, int const& actionIndex) const {
    if(actionStates[actionIndex].scheduledActionFluents.empty()) {
        out << "noop() ";
    } else {
        for(unsigned int index = 0; index < actionStates[actionIndex].scheduledActionFluents.size(); ++index) {
            out << actionStates[actionIndex].scheduledActionFluents[index]->name << " ";
        }
    }
}

void PlanningTask::printActionInDetail(ostream& out, int const& index) const {
    printAction(out, index);
    out << ": " << endl;
    out << "Index : " << actionStates[index].index << endl;
    out << "Relevant SACs:" << endl;
    for(unsigned int i = 0; i < actionStates[index].relevantSACs.size(); ++i) {
        out << "  ";
        actionStates[index].relevantSACs[i]->formula->print(out);
        out << endl;
    }
    out << endl;
}

void PlanningTask::printCPFInDetail(std::ostream& out, int const& index) const {
    printEvaluatableInDetail(out, CPFs[index]);

    cout << endl << "  Map for probabilistic state hash keys: " << endl;
    for(map<double, long>::iterator it = CPFs[index]->probDomainMap.begin(); it != CPFs[index]->probDomainMap.end(); ++it) {
        out << "    " << it->first << " -> " << it->second << endl;
    }
}

void PlanningTask::printRewardCPFInDetail(std::ostream& out) const {
    printEvaluatableInDetail(out, rewardCPF);

    out << "Min Reward: " << rewardCPF->getMinVal() << endl;
    out << "Max Reward: " << rewardCPF->getMaxVal() << endl;
}

void PlanningTask::printDynamicSACInDetail(std::ostream& out, int const& index) const {
    printEvaluatableInDetail(out, dynamicSACs[index]);
}
 
void PlanningTask::printStaticSACInDetail(std::ostream& out, int const& index) const {
    printEvaluatableInDetail(out, staticSACs[index]);
}

void PlanningTask::printStateInvariantInDetail(std::ostream& out, int const& index) const {
    printEvaluatableInDetail(out, stateInvariants[index]);
}

void PlanningTask::printEvaluatableInDetail(ostream& out, Evaluatable* eval) const {
    out << eval->name << endl;
    out << "  HashIndex: " << eval->hashIndex << ",";

    if(!eval->isProbabilistic()) {
        out << " deterministic,";
    } else {
        out << " probabilistic,";
    }
    
    if(eval->isActionIndependent()) {
        out << " action independent,";
    } else {
        out << " action dependent,";
    }

    switch(eval->cachingType) {
    case Evaluatable::NONE:
        out << " no caching, ";
        break;
    case Evaluatable::MAP:
    case Evaluatable::DISABLED_MAP:
        out << " caching in maps, ";
        break;
    case Evaluatable::VECTOR:
        out << " caching in vectors of size " << eval->evaluationCacheVector.size() << ", ";
        break;
    }

    switch(eval->kleeneCachingType) {
    case Evaluatable::NONE:
        out << " no Kleene caching.";
        break;
    case Evaluatable::MAP:
    case Evaluatable::DISABLED_MAP:
        out << " Kleene caching in maps.";
        break;
    case Evaluatable::VECTOR:
        out << " Kleene caching in vectors of size " << eval->kleeneEvaluationCacheVector.size() << ".";
        break;
    }

    out << endl << endl;

    if(!eval->positiveActionDependencies.empty()) {
        out << "  Depends positively on action fluents:" << endl;

        for(set<ActionFluent*>::iterator it = eval->positiveActionDependencies.begin(); it != eval->positiveActionDependencies.end(); ++it) {
            out << "    ";
            (*it)->print(cout);
            out << endl;
        }
    } else {
        out << "  Has no positive dependencies on actions." << endl;
    }

    if(!eval->negativeActionDependencies.empty()) {
        out << "  Depends negatively on action fluents:" << endl;

        for(set<ActionFluent*>::iterator it = eval->negativeActionDependencies.begin(); it != eval->negativeActionDependencies.end(); ++it) {
            out << "    ";
            (*it)->print(cout);
            out << endl;
        }
    } else {
        out << "  Has no negative dependencies on actions." << endl;
    }

    if(!eval->dependentStateFluents.empty()) {
        out << "  Depends on state fluents:" << endl;
        for(set<StateFluent*>::iterator it = eval->dependentStateFluents.begin(); it != eval->dependentStateFluents.end(); ++it) {
            out << "    " << (*it)->name << endl;
        }
    } else {
        out << "  Has no dependencies on state fluents." << endl;
    }

    out << (eval->containsArithmeticFunction() ? "  Contains an arithmetic function." : "  Does not contain an arithmetic function.") << endl << endl;

    if(!eval->isActionIndependent()) {
        out <<  "  Action Hash Key Map: " << endl;
        for(unsigned int i = 0; i < eval->actionHashKeyMap.size(); ++i) {
            if(eval->actionHashKeyMap[i] != 0) {
                out << "    ";
                printAction(out, i);
                out << " : " << eval->actionHashKeyMap[i] << endl;
            }
        }
    }

    eval->formula->print(out);
    out << endl;
}
