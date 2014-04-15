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

/******************************************************************
                    Static variable definitions
******************************************************************/

int State::stateSize = 0;
int State::numberOfStateFluentHashKeys = 0;
bool State::stateHashingPossible = true;
vector<vector<long> > State::stateHashKeys;
vector<vector<pair<int, long> > > State::indexToStateFluentHashKeyMap;

int KleeneState::stateSize = 0;
int KleeneState::numberOfStateFluentHashKeys = 0;
bool KleeneState::stateHashingPossible = true;
vector<long> KleeneState::hashKeyBases;
vector<vector<pair<int,long> > > KleeneState::indexToStateFluentHashKeyMap;

//bool PDState::stateHashingPossible = true;

string SearchEngine::taskName;
vector<State> SearchEngine::trainingSet;
vector<ActionFluent*> SearchEngine::actionFluents;
vector<ActionState> SearchEngine::actionStates;
vector<StateFluent*> SearchEngine::stateFluents;
vector<ConditionalProbabilityFunction*> SearchEngine::probCPFs;
vector<ConditionalProbabilityFunction*> SearchEngine::detCPFs;
RewardFunction* SearchEngine::rewardCPF = NULL;
vector<Evaluatable*> SearchEngine::actionPreconditions;
bool SearchEngine::taskIsDeterministic = true;
State SearchEngine::initialState;
int SearchEngine::horizon = numeric_limits<int>::max();
double SearchEngine::discountFactor = 1.0;
int SearchEngine::numberOfActions = -1;
int SearchEngine::firstProbabilisticVarIndex = -1;
SearchEngine::FinalRewardCalculationMethod SearchEngine::finalRewardCalculationMethod = NOOP;
vector<int> SearchEngine::candidatesForOptimalFinalAction;


bool SearchEngine::cacheApplicableActions = true;
bool SearchEngine::useRewardLockDetection = true;
int SearchEngine::goalTestActionIndex = -1;
bool SearchEngine::useBDDCaching = true;
bdd SearchEngine::cachedDeadEnds = bddfalse;
bdd SearchEngine::cachedGoals = bddfalse;

bool ProbabilisticSearchEngine::hasUnreasonableActions = true;
bool DeterministicSearchEngine::hasUnreasonableActions = true;

bool ProbabilisticSearchEngine::taskAnalyzed = false;
bool DeterministicSearchEngine::taskAnalyzed = false;

map<State, vector<int>, State::CompareIgnoringRemainingSteps> ProbabilisticSearchEngine::applicableActionsCache;
map<State, vector<int>, State::CompareIgnoringRemainingSteps> DeterministicSearchEngine::applicableActionsCache;

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
    if(ProbabilisticSearchEngine::taskAnalyzed) {
        return;
    }

    cout << "Prob-Task: learning..." << endl;
    // Try if reasonable action pruning and reward lock detection are useful

    // These are set if one of the methods is successful
    bool rewardLockFound = false;
    bool unreasonableActionFound = false;

    for(unsigned int stateIndex = 0; stateIndex < SearchEngine::trainingSet.size(); ++stateIndex) {
        // Check if this training state has unreasonable actions
        vector<int> applicableActions = getApplicableActions(SearchEngine::trainingSet[stateIndex]);

        for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
            if((applicableActions[actionIndex] != actionIndex) && (applicableActions[actionIndex] != -1)) {
                unreasonableActionFound = true;
            }
        }

        // Check if this training state is a reward lock
        if(isARewardLock(SearchEngine::trainingSet[stateIndex])) {
            rewardLockFound = true;
        }
    }

    // Set the variables that control action pruning and reward lock detection
    ProbabilisticSearchEngine::hasUnreasonableActions = unreasonableActionFound;
    ProbabilisticSearchEngine::useRewardLockDetection = rewardLockFound;

    if(ProbabilisticSearchEngine::useRewardLockDetection && ProbabilisticSearchEngine::useBDDCaching) {
        // TODO: These numbers are rather random. Since I know only little on
        // what they actually mean, it'd be nice to re-adjust these sometime.
        bdd_init(5000000,20000);

        int* domains = new int[State::stateSize];
        for(unsigned int index = 0; index < SearchEngine::probCPFs.size(); ++index) {
            domains[index] = SearchEngine::probCPFs[index]->getDomainSize();
        }
        fdd_extdomain(domains, State::stateSize);
    }

    ProbabilisticSearchEngine::taskAnalyzed = true;
    cout << "Prob-Task: ...finished" << endl;
}

void DeterministicSearchEngine::learn() {
    if(DeterministicSearchEngine::taskAnalyzed) {
        return;
    }

    cout << "Det-Task: learning..." << endl;
    // Try if reasonable action pruning is useful
    bool unreasonableActionFound = false;

    for(unsigned int stateIndex = 0; stateIndex < SearchEngine::trainingSet.size(); ++stateIndex) {
        // Check if this training state has unreasonable actions
        vector<int> applicableActions = getApplicableActions(SearchEngine::trainingSet[stateIndex]);

        for(unsigned int actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
            if((applicableActions[actionIndex] != actionIndex) && (applicableActions[actionIndex] != -1)) {
                unreasonableActionFound = true;
            }
        }
    }

    // Set the variables that control action pruning
    DeterministicSearchEngine::hasUnreasonableActions = unreasonableActionFound;

    DeterministicSearchEngine::taskAnalyzed = true;

    cout << "Det-Task: ...finished" << endl;
}

/******************************************************************
                       Main Search Functions
******************************************************************/

bool SearchEngine::estimateBestActions(State const& _rootState, std::vector<int>& bestActions) {
    vector<double> qValues(SearchEngine::numberOfActions);
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
    vector<double> qValues(SearchEngine::numberOfActions);
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
            Reward Lock Detection (including BDD Stuff)
******************************************************************/

// Currently, we only consider goals and dead ends (i.e., reward locks
// with min or max reward). This makes sense on the IPPC 2011 domains,
// yet we might want to change it in the future so keep an eye on it.
// Nevertheless, isARewardLock is sound as is (and incomplete
// independently from this decision).
bool ProbabilisticSearchEngine::isARewardLock(State const& current) const {
    if(!SearchEngine::useRewardLockDetection) {
        return false;
    }

    assert(SearchEngine::goalTestActionIndex >= 0);

    // Calculate the reference reward
    double reward = 0.0;
    calcReward(current, SearchEngine::goalTestActionIndex, reward);

    if(MathUtils::doubleIsEqual(SearchEngine::rewardCPF->getMinVal(), reward)) {
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
    } else if(MathUtils::doubleIsEqual(SearchEngine::rewardCPF->getMaxVal(), reward)) {
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

bool ProbabilisticSearchEngine::checkDeadEnd(KleeneState const& state) const {
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
    if((reward.size() != 1) || !MathUtils::doubleIsEqual(*reward.begin(), SearchEngine::rewardCPF->getMinVal())) {
        return false;
    }

    for(unsigned int actionIndex = 1; actionIndex < SearchEngine::numberOfActions; ++actionIndex) {
        reward.clear();
        // Apply action actionIndex
        KleeneState succ;
        calcKleeneSuccessor(state, actionIndex, succ);
        calcKleeneReward(state, actionIndex, reward);

        // If reward is not minimal this is not a dead end
        if((reward.size() != 1) || !MathUtils::doubleIsEqual(*reward.begin(), SearchEngine::rewardCPF->getMinVal())) {
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
bool ProbabilisticSearchEngine::checkGoal(KleeneState const& state) const {
    // Apply action goalTestActionIndex
    KleeneState succ;
    set<double> reward;
    calcKleeneSuccessor(state, SearchEngine::goalTestActionIndex, succ);
    calcKleeneReward(state, SearchEngine::goalTestActionIndex, reward);

    // If reward is not maximal with certainty this is not a goal
    if((reward.size() > 1) || !MathUtils::doubleIsEqual(SearchEngine::rewardCPF->getMaxVal(), *reward.begin())) {
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

inline bdd ProbabilisticSearchEngine::stateToBDD(KleeneState const& state) const {
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

inline bdd ProbabilisticSearchEngine::stateToBDD(State const& state) const {
    bdd res = bddtrue;
    for(unsigned int i = 0; i < State::stateSize; ++i) {
        res &= fdd_ithvar(i, state[i]);
    }
    return res;
}

inline bool ProbabilisticSearchEngine::BDDIncludes(bdd BDD, KleeneState const& state) const {
    return (BDD & stateToBDD(state)) != bddfalse;
}

/******************************************************************
               Calculation of Final Reward and Action
******************************************************************/

void SearchEngine::calcOptimalFinalRewardWithFirstApplicableAction(State const& current, double& reward) const {
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

void SearchEngine::calcOptimalFinalRewardAsBestOfCandidateSet(State const& current, double& reward) const {
    // Get applicable actions
    vector<int> applicableActions = getApplicableActions(current);

    reward = -numeric_limits<double>::max();
    double tmpReward = 0.0;

    for(unsigned int candidateIndex = 0; candidateIndex < SearchEngine::candidatesForOptimalFinalAction.size(); ++candidateIndex) {
        int& actionIndex = SearchEngine::candidatesForOptimalFinalAction[candidateIndex];
        if(applicableActions[actionIndex] == actionIndex) {
            calcReward(current, actionIndex, tmpReward);

            if(MathUtils::doubleIsGreater(tmpReward, reward)) {
                reward = tmpReward;
            }
        }
    }
}

int SearchEngine::getOptimalFinalActionIndex(State const& current) const {
    if(SearchEngine::finalRewardCalculationMethod == SearchEngine::NOOP) {
        return 0;
    }

    // Get applicable actions
    vector<int> applicableActions = getApplicableActions(current);

    if(SearchEngine::finalRewardCalculationMethod == SearchEngine::FIRST_APPLICABLE) {
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
    assert(SearchEngine::finalRewardCalculationMethod == SearchEngine::BEST_OF_CANDIDATE_SET);
    double reward = -numeric_limits<double>::max();
    double tmpReward = 0.0;
    int optimalFinalActionIndex = -1;

    for(unsigned int candidateIndex = 0; candidateIndex < SearchEngine::candidatesForOptimalFinalAction.size(); ++candidateIndex) {
        int& actionIndex = SearchEngine::candidatesForOptimalFinalAction[candidateIndex];
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

void SearchEngine::print(ostream& out) const {
    out << outStream.str() << endl;
    outStream.str("");
}

void SearchEngine::printStats(ostream& out, bool const& /*printRoundStats*/, string indent) const {
    out << indent << "Statistics of " << name << ":" << endl;
}


void SearchEngine::printTask(ostream& out) {
    out.unsetf(ios::floatfield);
    out.precision(numeric_limits<double>::digits10);

    out << "----------------Actions---------------" << endl << endl;
    out << "Action fluents: " << endl;
    for(unsigned int index = 0; index < actionFluents.size(); ++index) {
        actionFluents[index]->print(out);
        out << endl;
    }
    out << "---------------" << endl << endl;
    out << "Legal Action Combinations: " << endl;
    for(unsigned int index = 0; index < actionStates.size(); ++index) {
        actionStates[index].print(out);
        out << "---------------" << endl;
    }
    out << endl;
    out << "-----------------CPFs-----------------" << endl << endl;
    for(unsigned int index = 0; index < State::stateSize; ++index) {
        printCPFInDetail(out, index);
        out << endl << "--------------" << endl;
    }

    out << endl << "Reward CPF:" << endl;
    printRewardCPFInDetail(out);
    out << endl << endl;

    out << "------State Fluent Hash Key Map-------" << endl << endl;

    for(unsigned int varIndex = 0; varIndex < State::indexToStateFluentHashKeyMap.size(); ++varIndex) {
        out << "a change of variable " << varIndex << " influences variables ";
        for(unsigned int influencedVarIndex = 0; influencedVarIndex < State::indexToStateFluentHashKeyMap[varIndex].size(); ++influencedVarIndex) {
            out << State::indexToStateFluentHashKeyMap[varIndex][influencedVarIndex].first << " (";
            out << State::indexToStateFluentHashKeyMap[varIndex][influencedVarIndex].second << ") ";
        }
        out << endl;
    }
    out << endl;

    for(unsigned int varIndex = 0; varIndex < KleeneState::indexToStateFluentHashKeyMap.size(); ++varIndex) {
        out << "a change of variable " << varIndex << " influences variables in Kleene states ";
        for(unsigned int influencedVarIndex = 0; influencedVarIndex < KleeneState::indexToStateFluentHashKeyMap[varIndex].size(); ++influencedVarIndex) {
            out << KleeneState::indexToStateFluentHashKeyMap[varIndex][influencedVarIndex].first << " ("
                << KleeneState::indexToStateFluentHashKeyMap[varIndex][influencedVarIndex].second << ") ";
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
    initialState.print(out);
    out << endl;

    out << "Hashing of States is " << (State::stateHashingPossible? "" : "not ") << "possible." << endl;
    out << "Hashing of KleeneStates is " << (KleeneState::stateHashingPossible? "" : "not ") << "possible." << endl;

    if(SearchEngine::useRewardLockDetection) {
        if(SearchEngine::goalTestActionIndex >= 0) {
            out << "Reward lock detection is enabled for goals and dead ends." << endl;
        } else {
            out << "Reward lock detection is enabled for dead ends." << endl;
        }
    } else {
        out << "Reward lock detection is disabled." << endl;
    }

    if(ProbabilisticSearchEngine::hasUnreasonableActions && DeterministicSearchEngine::hasUnreasonableActions) {
        out << "This task contains unreasonable actions." << endl;
    } else if(ProbabilisticSearchEngine::hasUnreasonableActions) {
        assert(false);
    } else if(DeterministicSearchEngine::hasUnreasonableActions) {
        out << "This task contains unreasonable actions only in the determinization." << endl;
    } else {
        out << "This task does not contain unreasonable actions." << endl;
    }

    out << "The final reward is determined ";
    switch(finalRewardCalculationMethod) {
    case NOOP:
        out << "by applying NOOP." << endl;
        break;
    case FIRST_APPLICABLE:
        out << "by applying the first applicable action." << endl;
        break;
    case BEST_OF_CANDIDATE_SET:
        out << "as the maximum over the candidate set: " << endl;
        for(unsigned int i = 0; i < candidatesForOptimalFinalAction.size(); ++i) {
            out << "  ";
            actionStates[candidatesForOptimalFinalAction[i]].printCompact(out);
            out << endl;
        }
        break;
    }
    out << endl;
}

void SearchEngine::printCPFInDetail(ostream& out, int const& index) {
    printEvaluatableInDetail(out, probCPFs[index]);
  
    if(probCPFs[index]->isProbabilistic()) {
        out << "  Determinized formula: " << endl;
        out << "    ";
        detCPFs[index]->formula->print(out);
        out << endl;
    }

    out << endl;

    out << "  Domain: ";
    for(unsigned int i = 0; i < probCPFs[index]->head->values.size(); ++i) {
        out << probCPFs[index]->head->values[i] << " ";
    }
    out << endl;

    if(State::stateHashingPossible) {
        out << "  HashKeyBase: ";
        for(unsigned int i = 0; i < State::stateHashKeys[index].size(); ++i) {
            out << i << ": " << State::stateHashKeys[index][i];
            if(i != State::stateHashKeys[index].size() - 1) {
                out << ", ";
            } else {
                out << endl;
            }
        }
    }
    if(KleeneState::stateHashingPossible) {
        out << "  KleeneHashKeyBase: " << KleeneState::hashKeyBases[index] << endl;
    }
}

void SearchEngine::printRewardCPFInDetail(ostream& out) {
    printEvaluatableInDetail(out, rewardCPF);

    out << "Min Reward: " << rewardCPF->getMinVal() << endl;
    out << "Max Reward: " << rewardCPF->getMaxVal() << endl;

    out << endl;
}

void SearchEngine::printActionPreconditionInDetail(ostream& out, int const& index) {
    printEvaluatableInDetail(out, actionPreconditions[index]);

    out << endl;
}

void SearchEngine::printEvaluatableInDetail(ostream& out, Evaluatable* eval) {
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
                actionStates[i].printCompact(out);
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
}


