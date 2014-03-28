#include "planning_task.h"

#include "logical_expressions.h"

#include <cassert>
#include <iostream>

using namespace std;

/*****************************************************************
                         Initialization
*****************************************************************/

PlanningTask::PlanningTask(string _name,
                           vector<State> _trainingSet,
                           vector<ActionFluent*>& _actionFluents,
                           vector<ActionState>& _actionStates,
                           vector<StateFluent*>& _stateFluents,
                           vector<ConditionalProbabilityFunction*>& _CPFs, 
                           RewardFunction* _rewardCPF,
                           vector<Evaluatable*>& _actionPreconditions, 
                           State& _initialState,
                           int _horizon,
                           double _discountFactor,
                           bool _noopIsOptimalFinalAction,
                           bool _rewardFormulaAllowsRewardLockDetection,
                           vector<vector<long> > const& _stateHashKeys,
                           vector<long> const& _kleeneHashKeyBases,
                           vector<vector<pair<int,long> > > const& _indexToStateFluentHashKeyMap,
                           vector<vector<pair<int,long> > > const& _indexToKleeneStateFluentHashKeyMap) :
    name(_name),
    trainingSet(_trainingSet),
    actionFluents(_actionFluents),
    actionStates(_actionStates),
    stateFluents(_stateFluents),
    CPFs(_CPFs),
    rewardCPF(_rewardCPF),
    actionPreconditions(_actionPreconditions),
    initialState(_initialState),
    horizon(_horizon),
    discountFactor(_discountFactor),
    finalRewardCalculationMethod(BEST_OF_CANDIDATE_SET),
    useRewardLockDetection(_rewardFormulaAllowsRewardLockDetection),
    useBDDCaching(true), // TODO: Make this a parameter!
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
    numberOfStateFluentHashKeys = actionPreconditions.size() + CPFs.size() + 1;
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

    if(_noopIsOptimalFinalAction) {
        finalRewardCalculationMethod = NOOP;
    } else if(rewardCPF->isActionIndependent()) {
        finalRewardCalculationMethod = FIRST_APPLICABLE;
    }

    // Initialize stuff that is needed for reward lock detection
    // (goalTestActionIndex is not really implemented yet, and the FDDs are
    // initialized with values that are not necessarily good.
    if(useRewardLockDetection) {
        goalTestActionIndex = 0;
    } else {
        goalTestActionIndex = -1;
    }

    stateHashingPossible = !stateHashKeys.empty();
    kleeneStateHashingPossible = !kleeneHashKeyBases.empty();

    // Calculate hash keys of initial state
    calcStateFluentHashKeys(initialState);
    calcStateHashKey(initialState);

    // Calculate hash keys of states in training set
    for(unsigned int i = 0; i < trainingSet.size(); ++i) {
        calcStateFluentHashKeys(trainingSet[i]);
        calcStateHashKey(trainingSet[i]);
    }
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

    if(useRewardLockDetection && useBDDCaching) {
        // TODO: These numbers are rather random and chosen s.t. the bdd
        // operations do not output anything even on bigger problems.
        // Nevertheless, I know only little on what they actually mean, one
        // could readjust these if it were different.
        bdd_init(5000000,20000);

        int* domains = new int[stateSize];
        for(unsigned int index = 0; index < CPFs.size(); ++index) {
            domains[index] = CPFs[index]->getDomainSize();
        }
        fdd_extdomain(domains, stateSize);
    }

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
    for(unsigned int sacIndex = 0; sacIndex < action.actionPreconditions.size(); ++sacIndex) {
        action.actionPreconditions[sacIndex]->evaluate(res, current, action);
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
        if(useBDDCaching && BDDIncludes(cachedDeadEnds, current)) {
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
        if(useBDDCaching && BDDIncludes(cachedGoals, current)) {
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
        if(useBDDCaching) {
            cachedDeadEnds |= stateToBDD(state);
        }
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

    // If reward is not maximal with certainty this is not a goal
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
        if(useBDDCaching) {
            cachedGoals |= stateToBDD(state);
        }
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
    switch(finalRewardCalculationMethod) {
    case NOOP:
        calcReward(current, 0, reward);
        break;
    case FIRST_APPLICABLE:
        calcOptimalFinalRewardWithFirstApplicableAction(current, reward, useDeterminization);
        break;
    case BEST_OF_CANDIDATE_SET:
        calcOptimalFinalRewardAsBestOfCandidateSet(current, reward, useDeterminization);
        break;
    }
}

inline void PlanningTask::calcOptimalFinalRewardWithFirstApplicableAction(State const& current, double& reward, bool const& useDeterminization) {
    // Get applicable actions
    vector<int> applicableActions = getApplicableActions(current, useDeterminization);

    // If no action fluent occurs in the reward, the reward is the same for all
    // applicable actions, so we only need to find an applicable action
    for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
        if(applicableActions[actionIndex] == actionIndex) {
            return calcReward(current, actionIndex, reward);
        }
    }
    assert(false);
}

inline void PlanningTask::calcOptimalFinalRewardAsBestOfCandidateSet(State const& current, double& reward, bool const& useDeterminization) {
    // Get applicable actions
    vector<int> applicableActions = getApplicableActions(current, useDeterminization);

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
    if(finalRewardCalculationMethod == NOOP) {
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
            out << indexToStateFluentHashKeyMap[varIndex][influencedVarIndex].first << " (";
            out << indexToStateFluentHashKeyMap[varIndex][influencedVarIndex].second << " )";
            //for(unsigned int val = 0; val < indexToStateFluentHashKeyMap[varIndex][influencedVarIndex].second.size(); ++val) {
            //    out << indexToStateFluentHashKeyMap[varIndex][influencedVarIndex].second[val];
            //    if(val == indexToStateFluentHashKeyMap[varIndex][influencedVarIndex].second.size()-1) {
            //        out << ")";
            //    } else {
            //        out << " ";
            //    }
            //}
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

    out << "---------Action Preconditions---------" << endl << endl;
    for(unsigned int index = 0; index < actionPreconditions.size(); ++index) {
        printActionPreconditionInDetail(out, index);
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
        out << "This task does not contain unreasonable actions in the original or the determinization." << endl;
    }

    out << "The final is reward is determined ";
    switch(finalRewardCalculationMethod) {
    case NOOP:
        out << "by applying NOOP." << endl;
        break;
    case FIRST_APPLICABLE:
        out << "by applying the first applicable action." << endl;
        break;
    case BEST_OF_CANDIDATE_SET:
        out << "as the maximum over all applicable actions." << endl;
        break;
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
            out << actionStates[actionIndex].scheduledActionFluents[index]->name << " ";
        }
    }
}

void PlanningTask::printActionInDetail(ostream& out, int const& index) const {
    printAction(out, index);
    out << ": " << endl;
    out << "Index : " << actionStates[index].index << endl;
    out << "Relevant preconditions:" << endl;
    for(unsigned int i = 0; i < actionStates[index].actionPreconditions.size(); ++i) {
        out << "  ";
        actionStates[index].actionPreconditions[i]->formula->print(out);
        out << endl;
    }
    out << endl;
}

void PlanningTask::printCPFInDetail(ostream& out, int const& index) const {
    printEvaluatableInDetail(out, CPFs[index]);

    out << "  Domain: ";
    for(unsigned int i = 0; i < CPFs[index]->head->values.size(); ++i) {
        out << CPFs[index]->head->values[i] << " ";
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
    //out << "  KleeneDomainSize: " << CPFs[index]->kleeneDomainSize << endl;

    // cout << endl << "  Map for probabilistic state hash keys: " << endl;
    // for(map<double, long>::iterator it = CPFs[index]->probDomainMap.begin(); it != CPFs[index]->probDomainMap.end(); ++it) {
    //     out << "    " << it->first << " -> " << it->second << endl;
    // }
}

void PlanningTask::printRewardCPFInDetail(ostream& out) const {
    printEvaluatableInDetail(out, rewardCPF);

    out << "Min Reward: " << rewardCPF->getMinVal() << endl;
    out << "Max Reward: " << rewardCPF->getMaxVal() << endl;
    out << "Action Independent?: " << rewardCPF->isActionIndependent() << endl;
}

void PlanningTask::printActionPreconditionInDetail(ostream& out, int const& index) const {
    printEvaluatableInDetail(out, actionPreconditions[index]);
}

void PlanningTask::printEvaluatableInDetail(ostream& out, Evaluatable* eval) const {
    out << eval->name << endl;
    out << "  HashIndex: " << eval->hashIndex << ",";

    if(!eval->isProbabilistic()) {
        out << " deterministic,";
    } else {
        out << " probabilistic,";
    }

    switch(eval->cachingType) {
    case Evaluatable::NONE:
        out << " no caching,";
        break;
    case Evaluatable::MAP:
    case Evaluatable::DISABLED_MAP:
        out << " caching in maps,";
        break;
    case Evaluatable::VECTOR:
        out << " caching in vectors of size " << (eval->isProbabilistic() ? eval->pdEvaluationCacheVector.size() : eval->evaluationCacheVector.size()) << ",";
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

    if(!eval->actionHashKeyMap.empty()) {
        out <<  "  Action Hash Key Map: " << endl;
        for(unsigned int i = 0; i < eval->actionHashKeyMap.size(); ++i) {
            if(eval->actionHashKeyMap[i] != 0) {
                out << "    ";
                printAction(out, i);
                out << " : " << eval->actionHashKeyMap[i] << endl;
            }
        }
    } else {
        out << "  Has no positive dependencies on actions." << endl;
    }

    out << "  Formula: " << endl;
    out << "    ";
    eval->formula->print(out);
    out << endl;

    if(eval->isProbabilistic()) {
        out << "  Determinized formula: " << endl;
        out << "    ";
        eval->determinizedFormula->print(out);
        out << endl;
    }

    out << endl;
}
