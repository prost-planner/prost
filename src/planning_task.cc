#include "planning_task.h"

#include "logical_expressions.h"

#include <cassert>
#include <iostream>

using namespace std;

/*****************************************************************
                         Initialization
*****************************************************************/

PlanningTask::PlanningTask(string _name,
                           vector<ActionFluent*>& _actionFluents,
                           vector<ActionState>& _actionStates,
                           vector<ConditionalProbabilityFunction*>& _CPFs, 
                           RewardFunction* _rewardCPF,
                           vector<Evaluatable*>& _dynamicSACs, 
                           vector<Evaluatable*>& _staticSACs,
                           vector<Evaluatable*>& _stateInvariants,
                           State& _initialState,
                           int _numberOfConcurrentActions,
                           int _horizon,
                           double _discountFactor,
                           bool _noopIsOptimalFinalAction,
                           bool _rewardFormulaAllowsRewardLockDetection,
                           vector<vector<long> > const& _stateHashKeys,
                           vector<long> const& _kleeneHashKeyBases,
                           vector<vector<pair<int,long> > > const& _indexToStateFluentHashKeyMap,
                           vector<vector<pair<int,long> > > const& _indexToKleeneStateFluentHashKeyMap) :
    name(_name),
    actionFluents(_actionFluents),
    actionStates(_actionStates),
    CPFs(_CPFs),
    rewardCPF(_rewardCPF),
    dynamicSACs(_dynamicSACs),
    staticSACs(_staticSACs),
    stateInvariants(_stateInvariants),
    initialState(_initialState),
    numberOfConcurrentActions(_numberOfConcurrentActions),
    horizon(_horizon),
    discountFactor(_discountFactor),
    noopIsOptimalFinalAction(_noopIsOptimalFinalAction),
    useRewardLockDetection(_rewardFormulaAllowsRewardLockDetection),
    cachedDeadEnds(bddfalse),
    cachedGoals(bddfalse),
    cacheApplicableActions(true),
    hasUnreasonableActions(true),
    hasUnreasonableActionsInDeterminization(true),
    stateHashKeys(_stateHashKeys),
    kleeneHashKeyBases(_kleeneHashKeyBases),
    indexToStateFluentHashKeyMap(_indexToStateFluentHashKeyMap),
    indexToKleeneStateFluentHashKeyMap(_indexToKleeneStateFluentHashKeyMap) {
    // Set member vars that depend on SACs and CPFs
    numberOfStateFluentHashKeys = dynamicSACs.size() + CPFs.size() + 1;
    stateSize = (int)CPFs.size();
    firstProbabilisticVarIndex = (int)CPFs.size();
    deterministic = true;
    for(unsigned int i = 0; i < stateSize; ++i) {
        if(CPFs[i]->isProbabilistic()) {
            firstProbabilisticVarIndex = i;
            deterministic = false;
            break;
        }
    }
    numberOfActions = (int)actionStates.size();

    // Initialize stuff that is needed for reward lock detection
    // (goalTestActionIndex is not really implemented yet, and the FDDs are
    // initialized with values that are not necessarily good.
    if(useRewardLockDetection) {
        goalTestActionIndex = 0;

        bdd_init(5000000,20000);

        int* domains = new int[stateSize];
        for(unsigned int index = 0; index < CPFs.size(); ++index) {
           domains[index] = CPFs[index]->getDomainSize();
        }
        fdd_extdomain(domains, stateSize);
    } else {
        goalTestActionIndex = -1;
    }

    stateHashingPossible = !stateHashKeys.empty();
    kleeneStateHashingPossible = !kleeneHashKeyBases.empty();

    // Calculate hash keys of initial state
    calcStateFluentHashKeys(initialState);
    calcStateHashKey(initialState);
}

//void PlanningTask::determinePruningEquivalence() {
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

    //REPAIR
    //isPruningEquivalentToDet = false;
    //return;

    // isPruningEquivalentToDet = true;
    // if(hasUnreasonableActions) {
    //     for(unsigned int i = firstProbabilisticVarIndex; i < stateSize; ++i) {
    //         if(CPFs[i]->probDomainMap.size() > 2) {
    //             isPruningEquivalentToDet = false;
    //             break;
    //         } else if(CPFs[i]->probDomainMap.size() == 2) {
    //             if(MathUtils::doubleIsGreater(CPFs[i]->probDomainMap.begin()->first,0.5) &&
    //                MathUtils::doubleIsGreater(CPFs[i]->probDomainMap.rbegin()->first,0.5)) {
    //                 isPruningEquivalentToDet = false;
    //                 break;
    //             }

    //             if(MathUtils::doubleIsSmallerOrEqual(CPFs[i]->probDomainMap.begin()->first,0.5) &&
    //                MathUtils::doubleIsSmallerOrEqual(CPFs[i]->probDomainMap.rbegin()->first,0.5)) {
    //                 isPruningEquivalentToDet = false;
    //                 break;
    //             }
    //         } //otherwise probDomainMap.size() == 1 and the variable
    //           //is pruning equivalent
    //     }
    //}
    //}

/*****************************************************************
                             Learning
*****************************************************************/

void PlanningTask::learn(vector<State> const& trainingSet) {
    cout << "Task: learning..." << endl;
    // Learn if reasonable action pruning and reward lock detection
    // might be needed in this planning task

    // These are set if one of the methods is successful
    bool rewardLockFound = false;
    bool unreasonableActionFound = false;
    bool unreasonableActionFoundInDeterminization = false;

    for(unsigned int stateIndex = 0; stateIndex < trainingSet.size(); ++stateIndex) {
        assert(trainingSet[stateIndex].remSteps >= 0);

        // Check if this training state has unreasonable actions
        vector<int> applicableActions = getApplicableActions(trainingSet[stateIndex], false);

        for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
            if((applicableActions[actionIndex] != actionIndex) && (applicableActions[actionIndex] != -1)) {
                unreasonableActionFound = true;
            }
        }

        // Check if this training state has unreasonable actions in the determinization
        vector<int> applicableActionsInDeterminization = getApplicableActions(trainingSet[stateIndex], true);

        for(unsigned int actionIndex = 0; actionIndex < applicableActionsInDeterminization.size(); ++actionIndex) {
            if((applicableActionsInDeterminization[actionIndex] != actionIndex) && (applicableActionsInDeterminization[actionIndex] != -1)) {
                unreasonableActionFoundInDeterminization = true;
            }
        }

        // Check if this training state is a reward lock
        if(isARewardLock(trainingSet[stateIndex])) {
            rewardLockFound = true;
        }
    }

    // Set the variables that control action pruning and reward lock detection
    hasUnreasonableActions = unreasonableActionFound;
    hasUnreasonableActionsInDeterminization = unreasonableActionFoundInDeterminization;
    useRewardLockDetection = rewardLockFound;

    cout << "Task: ...finished" << endl;
}


/******************************************************************
               Applicable Actions and Action Pruning
******************************************************************/

inline vector<int> PlanningTask::setApplicableReasonableActionsInDeterminization(State const& state) {
    // TODO: Check if there are positive actions in the reward CPF, as we are
    // not allowed to prune an action that occurs positively in the rewardCPF!

    vector<int> res(getNumberOfActions(), 0);

    map<State, vector<int> >::iterator it = applicableReasonableActionsCacheInDeterminization.find(state);
    if(it != applicableReasonableActionsCacheInDeterminization.end()) {
        assert(it->second.size() == res.size());
        for(unsigned int i = 0; i < res.size(); ++i) {
            res[i] = it->second[i];
        }
    } else {
        map<State, int, State::CompareIgnoringRemainingSteps> childStates;
    
        for(unsigned int actionIndex = 0; actionIndex < getNumberOfActions(); ++actionIndex) {
            if(actionIsApplicable(actionStates[actionIndex], state)) {
                // This action is applicable
                State nxt(stateSize, -1, numberOfStateFluentHashKeys);
                calcSuccessorStateInDeterminization(state, actionIndex, nxt);
                calcStateHashKey(nxt);

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

        if(cacheApplicableActions) {
            applicableReasonableActionsCacheInDeterminization[state] = res;
        }
    }
    return res;
}

inline vector<int> PlanningTask::setApplicableReasonableActions(State const& state) {
    // TODO: Check if there are positive actions in the reward CPF, as we are
    // not allowed to prune an action that occurs positively in the rewardCPF!

    vector<int> res(getNumberOfActions(), 0);

    map<State, vector<int> >::iterator it = applicableReasonableActionsCache.find(state);
    if(it != applicableReasonableActionsCache.end()) {
        assert(it->second.size() == res.size());
        for(unsigned int i = 0; i < res.size(); ++i) {
            res[i] = it->second[i];
        }
    } else {
        map<PDState, int, PDState::CompareIgnoringRemainingSteps> childStates;
    
        for(unsigned int actionIndex = 0; actionIndex < getNumberOfActions(); ++actionIndex) {
            if(actionIsApplicable(actionStates[actionIndex], state)) {
                // This action is applicable
                PDState nxt(stateSize, state.remainingSteps()-1);
                calcSuccessorState(state, actionIndex, nxt);
                //REPAIR calcHashKeyOfProbabilityDistribution(nxt);

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
        if(cacheApplicableActions) {
            applicableReasonableActionsCache[state] = res;
        }
    }
    return res;
}

inline vector<int> PlanningTask::setApplicableActions(State const& state) {
    vector<int> res(getNumberOfActions(), 0);
    map<State, vector<int> >::iterator it = applicableActionsCache.find(state);
    if(it != applicableActionsCache.end()) {
        assert(it->second.size() == res.size());
        for(unsigned int i = 0; i < res.size(); ++i) {
            res[i] = it->second[i];
        }
    } else {
        for(unsigned int actionIndex = 0; actionIndex < getNumberOfActions(); ++actionIndex) {
            if(actionIsApplicable(actionStates[actionIndex], state)) {
                res[actionIndex] = actionIndex;
            } else {
                res[actionIndex] = -1;
            }
        }

        if(cacheApplicableActions) {
            applicableActionsCache[state] = res;
        }
    }

    return res;
}

inline bool PlanningTask::actionIsApplicable(ActionState const& action, State const& current) const {
    double res = 0.0;
    for(unsigned int sacIndex = 0; sacIndex < action.relevantSACs.size(); ++sacIndex) {
        action.relevantSACs[sacIndex]->evaluate(res, current, action);
        if(MathUtils::doubleIsEqual(res, 0.0)) {
            return false;
        }
    }
    return true;
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

        // Convert to Kleene state
        KleeneState currentInKleene(current);
        calcKleeneStateHashKey(currentInKleene);
        calcKleeneStateFluentHashKeys(currentInKleene);

        // Check reward lock on Kleene state
        return checkDeadEnd(currentInKleene);
    } else if(isMaxReward(reward)) {
        // Check if current is known to be a goal
        if(BDDIncludes(cachedGoals, current)) {
            return true;
        }

        // Convert to Kleene state
        KleeneState currentInKleene(current);
        calcKleeneStateHashKey(currentInKleene);
        calcKleeneStateFluentHashKeys(currentInKleene);

        // cout << "Checking state: " << endl;
        // printKleeneState(cout, currentInKleene);
        
        return checkGoal(currentInKleene);
    }
    return false;
}

bool PlanningTask::checkDeadEnd(KleeneState const& state) {
    // TODO: We do currently not care about action applicability.
    // Nevertheless, the results remain sound, as we only check too
    // many actions (it might be the case that we think some state is
    // not a dead end even though it is. This is because the action
    // that would make us leave the dead end is actually
    // inapplicable).

    // Apply noop
    KleeneState mergedSuccs(stateSize, numberOfStateFluentHashKeys);
    set<double> reward;
    calcKleeneSuccessor(state, 0, mergedSuccs);
    calcKleeneReward(state, 0, reward);

    // If reward is not minimal with cetainty this is not a dead end
    if((reward.size() != 1) || !isMinReward(*reward.begin())) {
        return false;
    }

    for(unsigned int actionIndex = 1; actionIndex < getNumberOfActions(); ++actionIndex) {
        reward.clear();
        // Apply action actionIndex
        KleeneState succ(stateSize, numberOfStateFluentHashKeys);
        calcKleeneSuccessor(state, actionIndex, succ);
        calcKleeneReward(state, actionIndex, reward);

        // If reward is not minimal this is not a dead end
        if((reward.size() != 1) || !isMinReward(*reward.begin())) {
            return false;
        }

        // Merge with previously computed successors
        mergedSuccs |= succ;
        //mergeKleeneStates(succ, mergedSuccs);
    }

    // Calculate hash keys
    calcKleeneStateHashKey(mergedSuccs);
    calcKleeneStateFluentHashKeys(mergedSuccs);

    // Check if nothing changed, otherwise continue dead end check
    if((mergedSuccs == state) || checkDeadEnd(mergedSuccs)) {
        cachedDeadEnds |= stateToBDD(state);
        return true;
    }
    return false;
}

// We underapproximate the set of goals, as we only consider those where
// applying goalTestActionIndex makes us stay in the reward lock.
bool PlanningTask::checkGoal(KleeneState const& state) {
    // Apply action goalTestActionIndex
    KleeneState succ(stateSize, numberOfStateFluentHashKeys);
    set<double> reward;
    calcKleeneSuccessor(state, goalTestActionIndex, succ);
    calcKleeneReward(state, goalTestActionIndex, reward);

    // If reward is not maximal with cetainty this is not a goal
    if((reward.size() > 1) || !isMaxReward(*reward.begin())) {
        return false;
    }

    // Add parent to successor
    succ |= state;
    //mergeKleeneStates(state, succ);

    // Calculate hash keys
    calcKleeneStateHashKey(succ);
    calcKleeneStateFluentHashKeys(succ);

    // Check if nothing changed, otherwise continue goal check
    if((succ == state) || checkGoal(succ)) {
        cachedGoals |= stateToBDD(state);
        return true;
    }
    return false;
}

inline bdd PlanningTask::stateToBDD(KleeneState const& state) const {
    bdd res = bddtrue;
    for(unsigned int i = 0; i < stateSize; ++i) {
        bdd tmp = bddfalse;
        for(set<double>::iterator it = state[i].begin(); it != state[i].end(); ++it) {
            tmp |= fdd_ithvar(i, *it);
        }
        res &= tmp;
    }
    return res;
}

inline bdd PlanningTask::stateToBDD(State const& state) const {
    bdd res = bddtrue;
    for(unsigned int i = 0; i < stateSize; ++i) {
        res &= fdd_ithvar(i, state[i]);
    }
    return res;
}

bool PlanningTask::BDDIncludes(bdd BDD, KleeneState const& state) const {
    return (BDD & stateToBDD(state)) != bddfalse;
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

void PlanningTask::calcOptimalFinalReward(State const& current, double& reward, bool const& useDeterminization) {
    if(noopIsOptimalFinalAction) {
        return calcReward(current, 0, reward);
    }

    // Get applicable actions
    vector<int> applicableActions = getApplicableActions(current, useDeterminization);

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

int PlanningTask::getOptimalFinalActionIndex(State const& current, bool const& useDeterminization) {
    if(noopIsOptimalFinalAction) {
        return 0;
    }

    // Get applicable actions
    vector<int> applicableActions = getApplicableActions(current, useDeterminization);

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

    //out << "This task is " << (isPruningEquivalentToDet? "" : "not ") << "pruning equivalent to its most likely determinization" << endl;

    out << "Hashing of States is " << (stateHashingPossible? "" : "not ") << "possible." << endl;
    out << "Hashing of KleeneStates is " << (kleeneStateHashingPossible? "" : "not ") << "possible." << endl;
    // if(!deterministic) {
    //     out << "Hashing of PDStates is " << (pdStateHashingPossible? "" : "not ") << "possible." << endl;
    // }

    if(useRewardLockDetection) {
        if(goalTestActionIndex >= 0) {
            out << "Reward lock detection is enabled for goals and dead ends." << endl;
        } else {
            out << "Reward lock detection is enabled for dead ends." << endl;
        }
    } else {
        out << "Reward lock detection is disabled." << endl;
    }

    if(hasUnreasonableActions && hasUnreasonableActionsInDeterminization) {
        out << "This task contains unreasonable actions in both the original and the determinization." << endl;
    } else if(hasUnreasonableActions) {
        assert(false);
    } else if(hasUnreasonableActionsInDeterminization) {
        out << "This task contains unreasonable actions only in the determinization." << endl;
    } else {
        out << "This task does not contain unreasonable actions in the original task or the determinization." << endl;
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
        out << CPFs[index]->name << ": ";
        //if(CPFs[index]->head->parent->valueType->type == Type::OBJECT) {
        //    out << CPFs[index]->head->parent->valueType->domain[state[index]]->name << endl;
        //} else {
        out << state[index] << endl;
        //}
    }
    out << "Remaining Steps: " << state.remainingSteps() << endl;
    out << "StateHashKey: " << state.hashKey << endl;
}

void PlanningTask::printKleeneState(ostream& out, KleeneState const& state) const {
    assert(state.state.size() == stateSize);
    for(unsigned int index = 0; index < stateSize; ++index) {
        out << CPFs[index]->name << ": { ";
        for(set<double>::iterator it = state[index].begin(); it != state[index].end(); ++it) {
            cout << *it << " ";
        }
        cout << "}" << endl;
    }
}

void PlanningTask::printPDState(ostream& out, PDState const& state) const {
    assert(state.state.size() == stateSize);
    for(unsigned int index = 0; index < stateSize; ++index) {
        out << CPFs[index]->name << ": ";
        state[index].print(out);
    }
    out << "Remaining Steps: " << state.remainingSteps() << endl;
}

void PlanningTask::printAction(ostream& out, int const& actionIndex) const {
    if(actionStates[actionIndex].scheduledActionFluents.empty()) {
        out << "noop() ";
    } else {
        for(unsigned int index = 0; index < actionStates[actionIndex].scheduledActionFluents.size(); ++index) {
            out << actionStates[actionIndex].scheduledActionFluents[index]->fullName << " ";
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

void PlanningTask::printCPFInDetail(ostream& out, int const& index) const {
    printEvaluatableInDetail(out, CPFs[index]);

    out << "  Domain: ";
    for(set<double>::iterator it = CPFs[index]->domain.begin(); it != CPFs[index]->domain.end(); ++it) {
        out << *it << " ";
    }
    out << endl;

    if(stateHashingPossible) {
        out << "  HashKeyBase: ";
        for(unsigned int i = 0; i < stateHashKeys[index].size(); ++i) {
            out << i << ": " << stateHashKeys[index][i];
            if(i != stateHashKeys[index].size() - 1) {
                out << ", ";
            } else {
                out << endl;
            }
        }
    }
    if(kleeneStateHashingPossible) {
        out << "  KleeneHashKeyBase: " << kleeneHashKeyBases[index] << endl;
    }
    out << "  KleeneDomainSize: " << CPFs[index]->kleeneDomainSize << endl;

    // cout << endl << "  Map for probabilistic state hash keys: " << endl;
    // for(map<double, long>::iterator it = CPFs[index]->probDomainMap.begin(); it != CPFs[index]->probDomainMap.end(); ++it) {
    //     out << "    " << it->first << " -> " << it->second << endl;
    // }
}

void PlanningTask::printRewardCPFInDetail(ostream& out) const {
    printEvaluatableInDetail(out, rewardCPF);

    out << "Min Reward: " << rewardCPF->getMinVal() << endl;
    out << "Max Reward: " << rewardCPF->getMaxVal() << endl;
}

void PlanningTask::printDynamicSACInDetail(ostream& out, int const& index) const {
    printEvaluatableInDetail(out, dynamicSACs[index]);
}
 
void PlanningTask::printStaticSACInDetail(ostream& out, int const& index) const {
    printEvaluatableInDetail(out, staticSACs[index]);
}

void PlanningTask::printStateInvariantInDetail(ostream& out, int const& index) const {
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
        out << " caching in vectors of size " << (eval->isProbabilistic() ? eval->pdEvaluationCacheVector.size() : eval->evaluationCacheVector.size()) << ", ";
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
            out << "    " << (*it)->fullName << endl;
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

    out << "  Formula: ";
    eval->formula->print(out);
    out << endl;
    if(eval->isProbabilistic()) {
        out << "  Determinization: ";
        eval->determinizedFormula->print(out);
        out << endl;
    }
    out << endl;
}
