#include "search_engine.h"

#include "prost_planner.h"

#include "mc_uct_search.h"
#include "max_mc_uct_search.h"
#include "dp_uct_search.h"

#include "iterative_deepening_search.h"
#include "depth_first_search.h"
#include "uniform_evaluation_search.h"

#include "utils/math_utils.h"
#include "utils/string_utils.h"
#include "utils/system_utils.h"

using namespace std;

bool SearchEngine::cacheApplicableActions = true;
bool SearchEngine::useRewardLockDetection = true;
int SearchEngine::goalTestActionIndex = -1;
bool SearchEngine::useBDDCaching = true;
bdd SearchEngine::cachedDeadEnds = bddfalse;
bdd SearchEngine::cachedGoals = bddfalse;

bool ProbabilisticSearchEngine::hasUnreasonableActions = true;
bool DeterministicSearchEngine::hasUnreasonableActions = true;

map<State, vector<int>, State::CompareIgnoringRemainingSteps> SearchEngine::applicableActionsCache;
map<State, vector<int>, State::CompareIgnoringRemainingSteps> SearchEngine::applicableReasonableActionsCache;
map<State, vector<int>, State::CompareIgnoringRemainingSteps> SearchEngine::applicableReasonableActionsCacheInDeterminization;

map<State, double, State::CompareConsideringRemainingSteps> ProbabilisticSearchEngine::stateValueCache;
map<State, double, State::CompareConsideringRemainingSteps> DeterministicSearchEngine::stateValueCache;

/******************************************************************
                     Search Engine Creation
******************************************************************/

SearchEngine* SearchEngine::fromString(string& desc) {
    StringUtils::trim(desc);
    assert(desc[0] == '[' && desc[desc.size()-1] == ']');
    StringUtils::removeFirstAndLastCharacter(desc);
    StringUtils::trim(desc);

    // Check if a shortcut description has been used. TODO: Implement
    // this in a clean and extendible way!
    if(desc.find("IPPC2011") == 0) {
        desc = desc.substr(8,desc.size());
        desc = "MC-UCT -sd 15 -i [IDS -sd 15]" + desc;
    } else if(desc.find("UCTStar") == 0) {
        desc = desc.substr(7,desc.size());
        desc = "DP-UCT -ndn 1 -iv 1" + desc;
    } else if(desc.find("MaxMC-UCTStar") == 0) {
        desc = desc.substr(13,desc.size());
        desc = "MaxMC-UCT -ndn 1" + desc;
    }

    SearchEngine* result = NULL;

    if(desc.find("MC-UCT") == 0) {
        desc = desc.substr(6,desc.size());
        result = new MCUCTSearch();
    } else if(desc.find("MaxMC-UCT") == 0) {
        desc = desc.substr(9,desc.size());
        result = new MaxMCUCTSearch();
    } else if(desc.find("DP-UCT") == 0) { 
        desc = desc.substr(6,desc.size());
        result = new DPUCTSearch();
    } else if(desc.find("IDS") == 0) {
        desc = desc.substr(3,desc.size());
        result = new IterativeDeepeningSearch();
    } else if(desc.find("DFS") == 0) {
        desc = desc.substr(3,desc.size());
        result = new DepthFirstSearch();
    } else if(desc.find("Uniform") == 0) {
        desc = desc.substr(7,desc.size());
        result = new UniformEvaluationSearch();
    } else {
        SystemUtils::abort("Unknown Search Engine: " + desc);
    }

    StringUtils::trim(desc);

    while(!desc.empty()) {
        string param;
        string value;
        StringUtils::nextParamValuePair(desc,param,value);

        if(!result->setValueFromString(param, value)) {
            SystemUtils::abort("Unused parameter value pair: " + param + " / " + value);
        }
    }
    return result;
}

bool SearchEngine::setValueFromString(string& param, string& value) {
    if(param == "-uc") {
        setCachingEnabled(atoi(value.c_str()));
        return true;
    } else if(param == "-sd") {
        setMaxSearchDepth(atoi(value.c_str()));
        return true;
    }

    return false;
}

/******************************************************************
                       Main Search Functions
******************************************************************/

void ProbabilisticSearchEngine::learn() {
    assert(!generalLearningFinished);

    cout << "PROB: learning..." << endl;
    // Try if reasonable action pruning and reward lock detection are useful

    // These are set if one of the methods is successful
    bool rewardLockFound = false;
    bool unreasonableActionFound = false;

    for(unsigned int stateIndex = 0; stateIndex < PlanningTask::trainingSet.size(); ++stateIndex) {
        // Check if this training state has unreasonable actions
        vector<int> applicableActions = getApplicableActions(PlanningTask::trainingSet[stateIndex]);

        for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
            if((applicableActions[actionIndex] != actionIndex) && (applicableActions[actionIndex] != -1)) {
                unreasonableActionFound = true;
            }
        }

        // Check if this training state is a reward lock
        if(isARewardLock(PlanningTask::trainingSet[stateIndex])) {
            rewardLockFound = true;
        }
    }

    // Set the variables that control action pruning and reward lock detection
    ProbabilisticSearchEngine::hasUnreasonableActions = unreasonableActionFound;
    ProbabilisticSearchEngine::useRewardLockDetection = rewardLockFound;

    if(ProbabilisticSearchEngine::useRewardLockDetection && ProbabilisticSearchEngine::useBDDCaching) {
        // TODO: These numbers are rather random and chosen s.t. the bdd
        // operations do not output anything even on bigger problems.
        // Nevertheless, I know only little on what they actually mean, one
        // could readjust these if it were different.
        bdd_init(5000000,20000);

        int* domains = new int[State::stateSize];
        for(unsigned int index = 0; index < PlanningTask::CPFs.size(); ++index) {
            domains[index] = PlanningTask::CPFs[index]->getDomainSize();
        }
        fdd_extdomain(domains, State::stateSize);
    }

    generalLearningFinished = true;

    cout << "PROB: ...finished" << endl;
}



void DeterministicSearchEngine::learn() {
    assert(!generalLearningFinished);

    cout << "DET: learning..." << endl;
    // Try if reasonable action pruning is useful
    bool unreasonableActionFound = false;

    for(unsigned int stateIndex = 0; stateIndex < PlanningTask::trainingSet.size(); ++stateIndex) {
        // Check if this training state has unreasonable actions
        vector<int> applicableActions = getApplicableActions(PlanningTask::trainingSet[stateIndex]);

        for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
            if((applicableActions[actionIndex] != actionIndex) && (applicableActions[actionIndex] != -1)) {
                unreasonableActionFound = true;
            }
        }
    }

    // Set the variables that control action pruning
    DeterministicSearchEngine::hasUnreasonableActions = unreasonableActionFound;

    generalLearningFinished = true;

    cout << "DET: ...finished" << endl;
}

/******************************************************************
                       Main Search Functions
******************************************************************/

bool SearchEngine::estimateBestActions(State const& _rootState, std::vector<int>& bestActions) {
    vector<double> qValues(PlanningTask::numberOfActions);
    vector<int> actionsToExpand = getApplicableActions(_rootState);

    if(!estimateQValues(_rootState, actionsToExpand, qValues)) {
        return false;
    }

    double stateValue = -numeric_limits<double>::max();    
    for(unsigned int actionIndex = 0; actionIndex < qValues.size(); ++actionIndex) {
        if(actionsToExpand[actionIndex] == actionIndex) {
            if(MathUtils::doubleIsGreater(qValues[actionIndex], stateValue)) {
                stateValue = qValues[actionIndex];
                bestActions.clear();
                bestActions.push_back(actionIndex);
            } else if(MathUtils::doubleIsEqual(qValues[actionIndex], stateValue)) {
                bestActions.push_back(actionIndex);
            }
        }
    }
    return true;
}

bool SearchEngine::estimateStateValue(State const& _rootState, double& stateValue) {
    vector<double> qValues(PlanningTask::numberOfActions);
    vector<int> actionsToExpand = getApplicableActions(_rootState);

    if(!estimateQValues(_rootState, actionsToExpand, qValues)) {
        return false;
    }

    stateValue = -numeric_limits<double>::max();
    for(unsigned int actionIndex = 0; actionIndex < qValues.size(); ++actionIndex) {
        if((actionsToExpand[actionIndex] == actionIndex) && MathUtils::doubleIsGreater(qValues[actionIndex], stateValue)) {
            stateValue = qValues[actionIndex];
        }
    }
    return true;
}


/******************************************************************
               Applicable Actions and Action Pruning
******************************************************************/

inline vector<int> DeterministicSearchEngine::setApplicableReasonableActions(State const& state) {
    // TODO: Check if there are positive actions in the reward CPF, as we are
    // not allowed to prune an action that occurs positively in the rewardCPF!

    vector<int> res(PlanningTask::numberOfActions, 0);

    map<State, vector<int> >::iterator it = SearchEngine::applicableReasonableActionsCacheInDeterminization.find(state);
    if(it != SearchEngine::applicableReasonableActionsCacheInDeterminization.end()) {
        assert(it->second.size() == res.size());
        for(unsigned int i = 0; i < res.size(); ++i) {
            res[i] = it->second[i];
        }
    } else {
        map<State, int, State::CompareIgnoringRemainingSteps> childStates;
    
        for(unsigned int actionIndex = 0; actionIndex < PlanningTask::numberOfActions; ++actionIndex) {
            if(actionIsApplicable(PlanningTask::actionStates[actionIndex], state)) {
                // This action is applicable
                State nxt;
                calcSuccessorStateInDeterminization(state, actionIndex, nxt);
                State::calcStateHashKey(nxt);

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

        if(SearchEngine::cacheApplicableActions) {
            SearchEngine::applicableReasonableActionsCacheInDeterminization[state] = res;
        }
    }
    return res;
}

inline vector<int> ProbabilisticSearchEngine::setApplicableReasonableActions(State const& state) {
    // TODO: Check if there are positive actions in the reward CPF, as we are
    // not allowed to prune an action that occurs positively in the rewardCPF!

    vector<int> res(PlanningTask::numberOfActions, 0);

    map<State, vector<int> >::iterator it = SearchEngine::applicableReasonableActionsCache.find(state);
    if(it != SearchEngine::applicableReasonableActionsCache.end()) {
        assert(it->second.size() == res.size());
        for(unsigned int i = 0; i < res.size(); ++i) {
            res[i] = it->second[i];
        }
    } else {
        map<PDState, int, PDState::CompareIgnoringRemainingSteps> childStates;
    
        for(unsigned int actionIndex = 0; actionIndex < PlanningTask::numberOfActions; ++actionIndex) {
            if(actionIsApplicable(PlanningTask::actionStates[actionIndex], state)) {
                // This action is applicable
                PDState nxt(State::stateSize, state.remainingSteps()-1);
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
        if(SearchEngine::cacheApplicableActions) {
            SearchEngine::applicableReasonableActionsCache[state] = res;
        }
    }
    return res;
}

inline vector<int> SearchEngine::setApplicableActions(State const& state) {
    vector<int> res(PlanningTask::numberOfActions, 0);
    map<State, vector<int> >::iterator it = SearchEngine::applicableActionsCache.find(state);
    if(it != SearchEngine::applicableActionsCache.end()) {
        assert(it->second.size() == res.size());
        for(unsigned int i = 0; i < res.size(); ++i) {
            res[i] = it->second[i];
        }
    } else {
        for(unsigned int actionIndex = 0; actionIndex < PlanningTask::numberOfActions; ++actionIndex) {
            if(SearchEngine::actionIsApplicable(PlanningTask::actionStates[actionIndex], state)) {
                res[actionIndex] = actionIndex;
            } else {
                res[actionIndex] = -1;
            }
        }

        if(SearchEngine::cacheApplicableActions) {
            SearchEngine::applicableActionsCache[state] = res;
        }
    }

    return res;
}

inline bool SearchEngine::actionIsApplicable(ActionState const& action, State const& current) {
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
bool SearchEngine::isARewardLock(State const& current) {
    if(!SearchEngine::useRewardLockDetection) {
        return false;
    }

    assert(SearchEngine::goalTestActionIndex >= 0);

    // Calculate the reference reward
    double reward = 0.0;
    calcReward(current, SearchEngine::goalTestActionIndex, reward);

    if(MathUtils::doubleIsEqual(PlanningTask::rewardCPF->getMinVal(), reward)) {
        // Check if current is known to be a dead end
        if(SearchEngine::useBDDCaching && BDDIncludes(SearchEngine::cachedDeadEnds, current)) {
            return true;
        }

        // Convert to Kleene state
        KleeneState currentInKleene(current);
        KleeneState::calcStateHashKey(currentInKleene);
        KleeneState::calcStateFluentHashKeys(currentInKleene);

        // Check reward lock on Kleene state
        return checkDeadEnd(currentInKleene);
    } else if(MathUtils::doubleIsEqual(PlanningTask::rewardCPF->getMaxVal(), reward)) {
        // Check if current is known to be a goal
        if(SearchEngine::useBDDCaching && BDDIncludes(SearchEngine::cachedGoals, current)) {
            return true;
        }

        // Convert to Kleene state
        KleeneState currentInKleene(current);
        KleeneState::calcStateHashKey(currentInKleene);
        KleeneState::calcStateFluentHashKeys(currentInKleene);

        // cout << "Checking state: " << endl;
        // printKleeneState(cout, currentInKleene);
        
        return checkGoal(currentInKleene);
    }
    return false;
}

bool SearchEngine::checkDeadEnd(KleeneState const& state) {
    // TODO: We do currently not care about action applicability.
    // Nevertheless, the results remain sound, as we only check too
    // many actions (it might be the case that we think some state is
    // not a dead end even though it is. This is because the action
    // that would make us leave the dead end is actually
    // inapplicable).

    // Apply noop
    KleeneState mergedSuccs;
    set<double> reward;
    calcKleeneSuccessor(state, 0, mergedSuccs);
    calcKleeneReward(state, 0, reward);

    // If reward is not minimal with certainty this is not a dead end
    if((reward.size() != 1) || !MathUtils::doubleIsEqual(*reward.begin(), PlanningTask::rewardCPF->getMinVal())) {
        return false;
    }

    for(unsigned int actionIndex = 1; actionIndex < PlanningTask::numberOfActions; ++actionIndex) {
        reward.clear();
        // Apply action actionIndex
        KleeneState succ;
        calcKleeneSuccessor(state, actionIndex, succ);
        calcKleeneReward(state, actionIndex, reward);

        // If reward is not minimal this is not a dead end
        if((reward.size() != 1) || !MathUtils::doubleIsEqual(*reward.begin(), PlanningTask::rewardCPF->getMinVal())) {
            return false;
        }

        // Merge with previously computed successors
        mergedSuccs |= succ;
        //mergeKleeneStates(succ, mergedSuccs);
    }

    // Calculate hash keys
    KleeneState::calcStateHashKey(mergedSuccs);
    KleeneState::calcStateFluentHashKeys(mergedSuccs);

    // Check if nothing changed, otherwise continue dead end check
    if((mergedSuccs == state) || checkDeadEnd(mergedSuccs)) {
        if(SearchEngine::useBDDCaching) {
            SearchEngine::cachedDeadEnds |= stateToBDD(state);
        }
        return true;
    }
    return false;
}

// We underapproximate the set of goals, as we only consider those where
// applying goalTestActionIndex makes us stay in the reward lock.
bool SearchEngine::checkGoal(KleeneState const& state) {
    // Apply action goalTestActionIndex
    KleeneState succ;
    set<double> reward;
    calcKleeneSuccessor(state, SearchEngine::goalTestActionIndex, succ);
    calcKleeneReward(state, SearchEngine::goalTestActionIndex, reward);

    // If reward is not maximal with certainty this is not a goal
    if((reward.size() > 1) || !MathUtils::doubleIsEqual(PlanningTask::rewardCPF->getMaxVal(), *reward.begin())) {
        return false;
    }

    // Add parent to successor
    succ |= state;
    //mergeKleeneStates(state, succ);

    // Calculate hash keys
    KleeneState::calcStateHashKey(succ);
    KleeneState::calcStateFluentHashKeys(succ);

    // Check if nothing changed, otherwise continue goal check
    if((succ == state) || checkGoal(succ)) {
        if(SearchEngine::useBDDCaching) {
            SearchEngine::cachedGoals |= stateToBDD(state);
        }
        return true;
    }
    return false;
}

inline bdd SearchEngine::stateToBDD(KleeneState const& state){
    bdd res = bddtrue;
    for(unsigned int i = 0; i < KleeneState::stateSize; ++i) {
        bdd tmp = bddfalse;
        for(set<double>::iterator it = state[i].begin(); it != state[i].end(); ++it) {
            tmp |= fdd_ithvar(i, *it);
        }
        res &= tmp;
    }
    return res;
}

inline bdd SearchEngine::stateToBDD(State const& state) {
    bdd res = bddtrue;
    for(unsigned int i = 0; i < State::stateSize; ++i) {
        res &= fdd_ithvar(i, state[i]);
    }
    return res;
}

bool SearchEngine::BDDIncludes(bdd BDD, KleeneState const& state) {
    return (BDD & stateToBDD(state)) != bddfalse;
}

/******************************************************************
               Calculation of Final Reward and Action
******************************************************************/

void SearchEngine::calcOptimalFinalReward(State const& current, double& reward) {
    switch(PlanningTask::finalRewardCalculationMethod) {
    case PlanningTask::NOOP:
        calcReward(current, 0, reward);
        break;
    case PlanningTask::FIRST_APPLICABLE:
        calcOptimalFinalRewardWithFirstApplicableAction(current, reward);
        break;
    case PlanningTask::BEST_OF_CANDIDATE_SET:
        calcOptimalFinalRewardAsBestOfCandidateSet(current, reward);
        break;
    }
}

inline void SearchEngine::calcOptimalFinalRewardWithFirstApplicableAction(State const& current, double& reward) {
    // Get applicable actions
    vector<int> applicableActions = getApplicableActions(current);

    // If no action fluent occurs in the reward, the reward is the same for all
    // applicable actions, so we only need to find an applicable action
    for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
        if(applicableActions[actionIndex] == actionIndex) {
            return calcReward(current, actionIndex, reward);
        }
    }
    assert(false);
}

inline void SearchEngine::calcOptimalFinalRewardAsBestOfCandidateSet(State const& current, double& reward) {
    // Get applicable actions
    vector<int> applicableActions = getApplicableActions(current);

    reward = -numeric_limits<double>::max();
    double tmpReward = 0.0;

    for(unsigned int candidateIndex = 0; candidateIndex < PlanningTask::candidatesForOptimalFinalAction.size(); ++candidateIndex) {
        int& actionIndex = PlanningTask::candidatesForOptimalFinalAction[candidateIndex];
        if(applicableActions[actionIndex] == actionIndex) {
            calcReward(current, actionIndex, tmpReward);

            if(MathUtils::doubleIsGreater(tmpReward, reward)) {
                reward = tmpReward;
            }
        }
    }
}

int SearchEngine::getOptimalFinalActionIndex(State const& current) {
    if(PlanningTask::finalRewardCalculationMethod == PlanningTask::NOOP) {
        return 0;
    }

    // Get applicable actions
    vector<int> applicableActions = getApplicableActions(current);

    if(PlanningTask::finalRewardCalculationMethod == PlanningTask::FIRST_APPLICABLE) {
        // If no action fluent occurs in the reward, all rewards are the
        // same and we only need to find an applicable action
        for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
            if(applicableActions[actionIndex] == actionIndex) {
                return actionIndex;
            }
        }
        assert(false);
        return -1;
    }

    // Otherwise we compute which action in the candidate set yields the highest
    // reward
    assert(PlanningTask::finalRewardCalculationMethod == PlanningTask::BEST_OF_CANDIDATE_SET);
    double reward = -numeric_limits<double>::max();
    double tmpReward = 0.0;
    int optimalFinalActionIndex = -1;

    for(unsigned int candidateIndex = 0; candidateIndex < PlanningTask::candidatesForOptimalFinalAction.size(); ++candidateIndex) {
        int& actionIndex = PlanningTask::candidatesForOptimalFinalAction[candidateIndex];
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
                   Statistics and Printers
******************************************************************/

void SearchEngine::print(ostream& out) {
    out << outStream.str() << endl;
    outStream.str("");
}

void SearchEngine::printStats(ostream& out, bool const& /*printRoundStats*/, string indent) {
    out << indent << "Statistics of " << name << ":" << endl;
}

