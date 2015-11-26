#include "search_engine.h"

#include "prost_planner.h"

#include "thts.h"
#include "iterative_deepening_search.h"
#include "depth_first_search.h"
#include "minimal_lookahead_search.h"
#include "uniform_evaluation_search.h"
#include "random_walk.h"

#include "utils/math_utils.h"
#include "utils/string_utils.h"
#include "utils/system_utils.h"

using namespace std;

/******************************************************************
                    Static variable definitions
******************************************************************/

int State::numberOfDeterministicStateFluents = 0;
int State::numberOfProbabilisticStateFluents = 0;

int State::numberOfStateFluentHashKeys = 0;
bool State::stateHashingPossible = true;

vector<vector<long> > State::stateHashKeysOfDeterministicStateFluents;
vector<vector<long> > State::stateHashKeysOfProbabilisticStateFluents;

vector<vector<pair<int, long> > > State::stateFluentHashKeysOfDeterministicStateFluents;
vector<vector<pair<int, long> > > State::stateFluentHashKeysOfProbabilisticStateFluents;

int KleeneState::stateSize = 0;
int KleeneState::numberOfStateFluentHashKeys = 0;
bool KleeneState::stateHashingPossible = true;
vector<long> KleeneState::hashKeyBases;
vector<vector<pair<int,long> > > KleeneState::indexToStateFluentHashKeyMap;

string SearchEngine::taskName;
vector<State> SearchEngine::trainingSet;

vector<ActionState> SearchEngine::actionStates;

vector<ActionFluent*> SearchEngine::actionFluents;
vector<StateFluent*> SearchEngine::stateFluents;

vector<Evaluatable*> SearchEngine::allCPFs;
vector<DeterministicCPF*> SearchEngine::deterministicCPFs;
vector<ProbabilisticCPF*> SearchEngine::probabilisticCPFs;

vector<DeterministicCPF*> SearchEngine::determinizedCPFs;

RewardFunction* SearchEngine::rewardCPF = nullptr;
vector<DeterministicEvaluatable*> SearchEngine::actionPreconditions;

bool SearchEngine::taskIsDeterministic = true;
State SearchEngine::initialState;
int SearchEngine::horizon = numeric_limits<int>::max();
double SearchEngine::discountFactor = 1.0;
int SearchEngine::numberOfActions = -1;
SearchEngine::FinalRewardCalculationMethod SearchEngine::finalRewardCalculationMethod = NOOP;
vector<int> SearchEngine::candidatesForOptimalFinalAction;

bool SearchEngine::cacheApplicableActions = true;
bool SearchEngine::rewardLockDetected = true;
int SearchEngine::goalTestActionIndex = -1;
bdd SearchEngine::cachedDeadEnds = bddfalse;
bdd SearchEngine::cachedGoals = bddfalse;

bool ProbabilisticSearchEngine::hasUnreasonableActions = true;
bool DeterministicSearchEngine::hasUnreasonableActions = true;

SearchEngine::ActionHashMap ProbabilisticSearchEngine::applicableActionsCache(520241);
SearchEngine::ActionHashMap DeterministicSearchEngine::applicableActionsCache(520241);

SearchEngine::StateValueHashMap ProbabilisticSearchEngine::stateValueCache(62233);
SearchEngine::StateValueHashMap DeterministicSearchEngine::stateValueCache(520241);

/******************************************************************
                     Search Engine Creation
******************************************************************/

SearchEngine* SearchEngine::fromString(string& desc) {
    StringUtils::trim(desc);
    assert(desc[0] == '[' && desc[desc.size() - 1] == ']');
    StringUtils::removeFirstAndLastCharacter(desc);
    StringUtils::trim(desc);

    // Check if a shortcut description has been used
    if (desc.find("IPPC2011") == 0) {
        // This is the configuration that was used at IPPC 2011 (all bugfixes
        // and code improvements that have been implemented since then are
        // contained, though)

        desc = desc.substr(8, desc.size());
        desc = "THTS -act [UCB1] -out [MC] -backup [MC] -ndn H -iv 5 -hw 1.0 -sd 15 -i [IDS -sd 15]" + desc;
    } else if (desc.find("IPPC2014") == 0) {
        // This is the configuration that was used at IPPC 2014 (without
        // consideration of the MDP-ESP problem that is described in our AAAI
        // 2015 paper, so it can be used for planner comparison)

        desc = desc.substr(8, desc.size());
        desc = "THTS -act [UCB1] -out [UMC] -backup [PB] -i [IDS] -hw 0.5" + desc;
    } else if (desc.find("MC-UCT") == 0) {
        // This is an implementation of the UCT algorithm as described by Kocsis
        // & Szepesvari (2006) within the THTS framework. It differs in that is
        // uses a heuristic (instead of a random walk initialization) and that
        // the tip node that is encountered in a trial is expanded entirely
        // (instead of adding only a single child to the tree)
 
        desc = desc.substr(6, desc.size());
        desc = "THTS -act [UCB1] -out [MC] -backup [MC]" + desc;
    } else if (desc.find("UCTStar") == 0) {
        // This is the UCT* algorithm as described in our ICAPS 2013 paper
 
        desc = desc.substr(7, desc.size());
        desc = "THTS -act [UCB1] -out [UMC] -backup [PB]" + desc;
    } else if (desc.find("DP-UCT") == 0) {
        // This is the DP-UCT algorithm as described in our ICAPS 2013 paper

        desc = desc.substr(6, desc.size());
        desc = "THTS -act [UCB1] -out [UMC] -backup [PB] -ndn H" + desc;
    } else if (desc.find("MaxUCT") == 0) {
        // This is the MaxUCT algorithm as described in our ICAPS 2013 paper

        desc = desc.substr(6, desc.size());
        desc = "THTS -act [UCB1] -out [MC] -backup [MaxMC] -ndn H" + desc;
    } else if (desc.find("BFS") == 0) {
        // This is a THTS version of breadth first search

        desc = desc.substr(3, desc.size());
        desc = "THTS -act [BFS] -out [UMC] -backup [PB]" + desc;
    }

    SearchEngine* result = nullptr;

    if (desc.find("THTS") == 0) {
        desc = desc.substr(4, desc.size());
        result = new THTS("THTS");
    } else if (desc.find("IDS") == 0) {
        desc = desc.substr(3, desc.size());
        result = new IDS();
    } else if (desc.find("DFS") == 0) {
        desc = desc.substr(3, desc.size());
        result = new DepthFirstSearch();
    } else if (desc.find("MLS") == 0) {
        desc = desc.substr(3, desc.size());
        result = new MinimalLookaheadSearch();
    } else if (desc.find("Uniform") == 0) {
        desc = desc.substr(7, desc.size());
        result = new UniformEvaluationSearch();
    } else if (desc.find("RandomWalk") == 0) {
        desc = desc.substr(10, desc.size());
        result = new RandomWalk();
    } else {
        SystemUtils::abort("Unknown Search Engine: " + desc);
    }

    StringUtils::trim(desc);

    while (!desc.empty()) {
        string param;
        string value;
        StringUtils::nextParamValuePair(desc, param, value);

        if (!result->setValueFromString(param, value)) {
            SystemUtils::abort(
                    "Unused parameter value pair: " + param + " / " + value);
        }
    }
    return result;
}

bool SearchEngine::setValueFromString(string& param, string& value) {
    if (param == "-uc") {
        setCachingEnabled(atoi(value.c_str()));
        return true;
    } else if (param == "-sd") {
        setMaxSearchDepth(atoi(value.c_str()));
        return true;
    } else if (param == "-t") {
        setTimeout(atof(value.c_str()));
        return true;
    } else if( param == "-rld") {
        setUseRewardLockDetection(atoi(value.c_str()));
        return true;
    } else if( param == "-crl") {
        setCacheRewardLocks(atoi(value.c_str()));
        return true;
    }

    return false;
}

/******************************************************************
                       Main Search Functions
******************************************************************/

bool SearchEngine::estimateBestActions(State const& _rootState,
                                       std::vector<int>& bestActions) {
    vector<double> qValues(numberOfActions);
    vector<int> actionsToExpand = getApplicableActions(_rootState);

    if (!estimateQValues(_rootState, actionsToExpand, qValues)) {
        return false;
    }

    double stateValue = -numeric_limits<double>::max();
    for (size_t actionIndex = 0; actionIndex < qValues.size(); ++actionIndex) {
        if (actionsToExpand[actionIndex] == actionIndex) {
            if (MathUtils::doubleIsGreater(qValues[actionIndex], stateValue)) {
                stateValue = qValues[actionIndex];
                bestActions.clear();
                bestActions.push_back(actionIndex);
            } else if (MathUtils::doubleIsEqual(qValues[actionIndex],
                               stateValue)) {
                bestActions.push_back(actionIndex);
            }
        }
    }
    return true;
}

bool SearchEngine::estimateStateValue(State const& _rootState,
                                      double& stateValue) {
    vector<double> qValues(numberOfActions);
    vector<int> actionsToExpand = getApplicableActions(_rootState);

    if (!estimateQValues(_rootState, actionsToExpand, qValues)) {
        return false;
    }

    stateValue = -numeric_limits<double>::max();
    for (size_t actionIndex = 0; actionIndex < qValues.size(); ++actionIndex) {
        if ((actionsToExpand[actionIndex] == actionIndex) &&
            MathUtils::doubleIsGreater(qValues[actionIndex], stateValue)) {
            stateValue = qValues[actionIndex];
        }
    }
    return true;
}

/******************************************************************
            Reward Lock Detection (including BDD Stuff)
******************************************************************/

// Currently, we only consider goals and dead ends (i.e., reward locks with min
// or max reward). This makes sense on the IPPC 2011 domains, yet we might want
// to change it in the future so keep an eye on it. Nevertheless, isARewardLock
// is sound as is (and incomplete independently from this decision).
bool ProbabilisticSearchEngine::isARewardLock(State const& current) const {
    if (!useRewardLockDetection) {
        return false;
    }

    assert(goalTestActionIndex >= 0);

    // Calculate the reference reward
    double reward = 0.0;
    calcReward(current, goalTestActionIndex, reward);

    if (MathUtils::doubleIsEqual(rewardCPF->getMinVal(), reward)) {
        // Check if current is known to be a dead end
        if (cacheRewardLocks && BDDIncludes(cachedDeadEnds, current)) {
            return true;
        }

        // Convert to Kleene state
        KleeneState currentInKleene(current);
        KleeneState::calcStateHashKey(currentInKleene);
        KleeneState::calcStateFluentHashKeys(currentInKleene);

        // Check reward lock on Kleene state
        return checkDeadEnd(currentInKleene);
    } else if (MathUtils::doubleIsEqual(rewardCPF->getMaxVal(), reward)) {
        // Check if current is known to be a goal
        if (cacheRewardLocks && BDDIncludes(cachedGoals, current)) {
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
    // TODO: We do currently not care about action applicability. Nevertheless,
    // the results remain sound, as we only check too many actions (it might be
    // the case that we think some state is not a dead end even though it is.
    // This is because the action that would make us leave the dead end is
    // actually inapplicable).

    // Apply noop
    KleeneState mergedSuccs;
    set<double> reward;
    calcKleeneSuccessor(state, 0, mergedSuccs);
    calcKleeneReward(state, 0, reward);

    // If reward is not minimal with certainty this is not a dead end
    if ((reward.size() != 1) ||
        !MathUtils::doubleIsEqual(*reward.begin(), rewardCPF->getMinVal())) {
        return false;
    }

    for (size_t actionIndex = 1; actionIndex < numberOfActions; ++actionIndex) {
        reward.clear();
        // Apply action actionIndex
        KleeneState succ;
        calcKleeneSuccessor(state, actionIndex, succ);
        calcKleeneReward(state, actionIndex, reward);

        // If reward is not minimal this is not a dead end
        if ((reward.size() != 1) ||
            !MathUtils::doubleIsEqual(*reward.begin(), rewardCPF->getMinVal())) {
            return false;
        }

        // Merge with previously computed successors
        mergedSuccs |= succ;
    }

    // Calculate hash keys
    KleeneState::calcStateHashKey(mergedSuccs);
    KleeneState::calcStateFluentHashKeys(mergedSuccs);

    // Check if nothing changed, otherwise continue dead end check
    if ((mergedSuccs == state) || checkDeadEnd(mergedSuccs)) {
        if (cacheRewardLocks) {
            cachedDeadEnds |= stateToBDD(state);
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
    calcKleeneSuccessor(state, goalTestActionIndex, succ);
    calcKleeneReward(state, goalTestActionIndex, reward);

    // If reward is not maximal with certainty this is not a goal
    if ((reward.size() > 1) ||
        !MathUtils::doubleIsEqual(rewardCPF->getMaxVal(), *reward.begin())) {
        return false;
    }

    // Add parent to successor
    succ |= state;

    // Calculate hash keys
    KleeneState::calcStateHashKey(succ);
    KleeneState::calcStateFluentHashKeys(succ);

    // Check if nothing changed, otherwise continue goal check
    if ((succ == state) || checkGoal(succ)) {
        if (cacheRewardLocks) {
            cachedGoals |= stateToBDD(state);
        }
        return true;
    }
    return false;
}

inline bdd ProbabilisticSearchEngine::stateToBDD(KleeneState const& state)
const {
    bdd res = bddtrue;
    for (size_t i = 0; i < KleeneState::stateSize; ++i) {
        bdd tmp = bddfalse;
        for (set<double>::iterator it = state[i].begin(); it != state[i].end();
             ++it) {
            tmp |= fdd_ithvar(i, *it);
        }
        res &= tmp;
    }
    return res;
}

inline bdd ProbabilisticSearchEngine::stateToBDD(State const& state) const {
    bdd res = bddtrue;
    for (size_t i = 0; i < State::numberOfDeterministicStateFluents; ++i) {
        res &= fdd_ithvar(i, state.deterministicStateFluent(i));
    }
    for (size_t i = 0; i < State::numberOfProbabilisticStateFluents; ++i) {
        res &=
            fdd_ithvar(State::numberOfDeterministicStateFluents + i,
                    state.probabilisticStateFluent(
                            i));
    }
    return res;
}

inline bool ProbabilisticSearchEngine::BDDIncludes(bdd BDD,
        KleeneState const& state) const {
    return (BDD & stateToBDD(state)) != bddfalse;
}

/******************************************************************
               Calculation of Final Reward and Action
******************************************************************/

void SearchEngine::calcOptimalFinalRewardWithFirstApplicableAction(
        State const& current, double& reward) const {
    // Get applicable actions
    vector<int> applicableActions = getApplicableActions(current);

    // If no action fluent occurs in the reward, the reward is the same for all
    // applicable actions, so we only need to find an applicable action
    for (size_t actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
        if (applicableActions[actionIndex] == actionIndex) {
            return calcReward(current, actionIndex, reward);
        }
    }
    assert(false);
}

void SearchEngine::calcOptimalFinalRewardAsBestOfCandidateSet(State const& current,
                                                              double& reward) const {
    // Get applicable actions
    vector<int> applicableActions = getApplicableActions(current);

    reward = -numeric_limits<double>::max();
    double tmpReward = 0.0;

    for (size_t candidateIndex = 0; candidateIndex < candidatesForOptimalFinalAction.size(); ++candidateIndex) {
        int& actionIndex = candidatesForOptimalFinalAction[candidateIndex];
        if (applicableActions[actionIndex] == actionIndex) {
            calcReward(current, actionIndex, tmpReward);

            if (MathUtils::doubleIsGreater(tmpReward, reward)) {
                reward = tmpReward;
            }
        }
    }
}

int SearchEngine::getOptimalFinalActionIndex(State const& current) const {
    if (finalRewardCalculationMethod == NOOP) {
        return 0;
    }

    // Get applicable actions
    vector<int> applicableActions = getApplicableActions(current);

    if (finalRewardCalculationMethod == FIRST_APPLICABLE) {
        // If no action fluent occurs in the reward, all rewards are the
        // same and we only need to find an applicable action
        for (size_t actionIndex = 0; actionIndex < applicableActions.size(); ++actionIndex) {
            if (applicableActions[actionIndex] == actionIndex) {
                return actionIndex;
            }
        }
        assert(false);
        return -1;
    }

    // Otherwise we compute which action in the candidate set yields the highest
    // reward
    assert(finalRewardCalculationMethod == BEST_OF_CANDIDATE_SET);
    double reward = -numeric_limits<double>::max();
    double tmpReward = 0.0;
    int optimalFinalActionIndex = -1;

    for (size_t candidateIndex = 0; candidateIndex < candidatesForOptimalFinalAction.size(); ++candidateIndex) {
        int& actionIndex = candidatesForOptimalFinalAction[candidateIndex];
        if (applicableActions[actionIndex] == actionIndex) {
            calcReward(current, actionIndex, tmpReward);

            if (MathUtils::doubleIsGreater(tmpReward, reward)) {
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

void SearchEngine::printStats(ostream& out,
                              bool const& /*printRoundStats*/,
                              string indent) const {
    out << indent << "Statistics of " << name << ":" << endl;
}


void SearchEngine::printTask(ostream& out) {
    out.unsetf(ios::floatfield);
    out.precision(numeric_limits<double>::digits10);

    out << "----------------Actions---------------" << endl << endl;
    out << "Action fluents: " << endl;
    for (size_t index = 0; index < actionFluents.size(); ++index) {
        actionFluents[index]->print(out);
        out << endl;
    }
    out << "---------------" << endl << endl;
    out << "Legal Action Combinations: " << endl;
    for (size_t index = 0; index < actionStates.size(); ++index) {
        actionStates[index].print(out);
        out << "---------------" << endl;
    }
    out << endl;
    out << "-----------------CPFs-----------------" << endl << endl;
    for (size_t index = 0; index < State::numberOfDeterministicStateFluents; ++index) {
        printDeterministicCPFInDetail(out, index);
        out << endl << "--------------" << endl;
    }

    for (size_t index = 0; index < State::numberOfProbabilisticStateFluents; ++index) {
        printProbabilisticCPFInDetail(out, index);
        out << endl << "--------------" << endl;
    }

    out << endl << "Reward CPF:" << endl;
    printRewardCPFInDetail(out);
    out << endl << endl;

    out << "------State Fluent Hash Key Map-------" << endl << endl;

    for (size_t varIndex = 0; varIndex < State::numberOfDeterministicStateFluents; ++varIndex) {
        out << "a change of deterministic state fluent " << varIndex <<
        " influences variables ";
        for (size_t influencedVarIndex = 0; influencedVarIndex < State::stateFluentHashKeysOfDeterministicStateFluents[varIndex].size(); ++influencedVarIndex) {
            out <<
            State::stateFluentHashKeysOfDeterministicStateFluents[varIndex][
                influencedVarIndex].first << " (";
            out <<
            State::stateFluentHashKeysOfDeterministicStateFluents[varIndex][
                influencedVarIndex].second << ") ";
        }
        out << endl;
    }
    out << endl;

    for (size_t varIndex = 0; varIndex < State::numberOfProbabilisticStateFluents; ++varIndex) {
        out << "a change of probabilistic state fluent " << varIndex <<
        " influences variables ";
        for (size_t influencedVarIndex = 0; influencedVarIndex < State::stateFluentHashKeysOfProbabilisticStateFluents[varIndex].size(); ++influencedVarIndex) {
            out <<
            State::stateFluentHashKeysOfProbabilisticStateFluents[varIndex][
                influencedVarIndex].first << " (";
            out <<
            State::stateFluentHashKeysOfProbabilisticStateFluents[varIndex][
                influencedVarIndex].second << ") ";
        }
        out << endl;
    }
    out << endl << endl;

    for (size_t varIndex = 0; varIndex < KleeneState::indexToStateFluentHashKeyMap.size(); ++varIndex) {
        out << "a change of variable " << varIndex <<
        " influences variables in Kleene states ";
        for (size_t influencedVarIndex = 0; influencedVarIndex < KleeneState::indexToStateFluentHashKeyMap[varIndex].size(); ++influencedVarIndex) {
            out <<
            KleeneState::indexToStateFluentHashKeyMap[varIndex][
                influencedVarIndex
            ].first << " ("
                    << KleeneState::indexToStateFluentHashKeyMap[varIndex][
                influencedVarIndex].second << ") ";
        }
        out << endl;
    }
    out << endl;

    out << "---------Action Preconditions---------" << endl << endl;
    for (size_t index = 0; index < actionPreconditions.size(); ++index) {
        printActionPreconditionInDetail(out, index);
        out << endl << "--------------" << endl;
    }

    out << "----------Initial State---------------" << endl << endl;
    initialState.print(out);
    out << endl;

    out << "Hashing of States is " <<
    (State::stateHashingPossible ? "" : "not ") << "possible." << endl;
    out << "Hashing of KleeneStates is " <<
    (KleeneState::stateHashingPossible ? "" : "not ") << "possible." << endl;

    if (rewardLockDetected) {
        if (goalTestActionIndex >= 0) {
            out <<
            "Both a goal and a dead end were found in the training phase." << endl;
        } else {
            out << "A dead end but no goal was found in the training phase." << endl;
        }
    } else {
        out << "No reward locks detected in the training phase." << endl;
    }

    if (ProbabilisticSearchEngine::hasUnreasonableActions &&
        DeterministicSearchEngine::hasUnreasonableActions) {
        out << "This task contains unreasonable actions." << endl;
    } else if (ProbabilisticSearchEngine::hasUnreasonableActions) {
        assert(false);
    } else if (DeterministicSearchEngine::hasUnreasonableActions) {
        out <<
        "This task contains unreasonable actions only in the determinization."
            << endl;
    } else {
        out << "This task does not contain unreasonable actions." << endl;
    }

    out << "The final reward is determined ";
    switch (finalRewardCalculationMethod) {
    case NOOP:
        out << "by applying NOOP." << endl;
        break;
    case FIRST_APPLICABLE:
        out << "by applying the first applicable action." << endl;
        break;
    case BEST_OF_CANDIDATE_SET:
        out << "as the maximum over the candidate set: " << endl;
        for (size_t i = 0; i < candidatesForOptimalFinalAction.size(); ++i) {
            out << "  ";
            actionStates[candidatesForOptimalFinalAction[i]].printCompact(out);
            out << endl;
        }
        break;
    }
    out << endl;
}

void SearchEngine::printDeterministicCPFInDetail(ostream& out,
                                                 int const& index) {
    printEvaluatableInDetail(out, deterministicCPFs[index]);
    out << endl;

    out << "  Domain: ";
    for (size_t i = 0; i < deterministicCPFs[index]->head->values.size(); ++i) {
        out << deterministicCPFs[index]->head->values[i] << " ";
    }
    out << endl;

    if (State::stateHashingPossible) {
        out << "  HashKeyBase: ";
        for (size_t i = 0; i < State::stateHashKeysOfDeterministicStateFluents[index].size(); ++i) {
            out << i << ": " <<
            State::stateHashKeysOfDeterministicStateFluents[index][i];
            if (i !=
                State::stateHashKeysOfDeterministicStateFluents[index].size() -
                1) {
                out << ", ";
            } else {
                out << endl;
            }
        }
    }

    if (KleeneState::stateHashingPossible) {
        out << "  KleeneHashKeyBase: " << KleeneState::hashKeyBases[index] <<
        endl;
    }
}

void SearchEngine::printProbabilisticCPFInDetail(ostream& out,
        int const& index) {
    printEvaluatableInDetail(out, probabilisticCPFs[index]);
    out << "  Determinized formula: " << endl;
    determinizedCPFs[index]->formula->print(out);
    out << endl;

    out << "  Domain: ";
    for (size_t i = 0; i < probabilisticCPFs[index]->head->values.size(); ++i) {
        out << probabilisticCPFs[index]->head->values[i] << " ";
    }
    out << endl;

    if (State::stateHashingPossible) {
        out << "  HashKeyBase: ";
        for (size_t i = 0; i < State::stateHashKeysOfProbabilisticStateFluents[index].size(); ++i) {
            out << i << ": " <<
            State::stateHashKeysOfProbabilisticStateFluents[index][i];
            if (i !=
                State::stateHashKeysOfProbabilisticStateFluents[index].size() -
                1) {
                out << ", ";
            } else {
                out << endl;
            }
        }
    }

    if (KleeneState::stateHashingPossible) {
        out << "  KleeneHashKeyBase: " <<
        KleeneState::hashKeyBases[index +
                                  State::numberOfDeterministicStateFluents] <<
        endl;
    }
}

void SearchEngine::printRewardCPFInDetail(ostream& out) {
    printEvaluatableInDetail(out, rewardCPF);

    out << "Minimal reward: " << rewardCPF->getMinVal() << endl;
    out << "Maximal reward: " << rewardCPF->getMaxVal() << endl;
    out << "Is action independent: " << rewardCPF->isActionIndependent() << endl;

    out << endl;
}

void SearchEngine::printActionPreconditionInDetail(ostream& out,
        int const& index) {
    printEvaluatableInDetail(out, actionPreconditions[index]);

    out << endl;
}

void SearchEngine::printEvaluatableInDetail(ostream& out, Evaluatable* eval) {
    out << eval->name << endl;
    out << "  HashIndex: " << eval->hashIndex << ",";

    if (!eval->isProbabilistic()) {
        out << " deterministic,";
    } else {
        out << " probabilistic,";
    }

    switch (eval->cachingType) {
    case Evaluatable::NONE:
        out << " no caching,";
        break;
    case Evaluatable::MAP:
    case Evaluatable::DISABLED_MAP:
        out << " caching in maps,";
        break;
    case Evaluatable::VECTOR:
        out << " caching in vectors,"; // << eval->evaluationCacheVector.size() << ",";
        break;
    }

    switch (eval->kleeneCachingType) {
    case Evaluatable::NONE:
        out << " no Kleene caching.";
        break;
    case Evaluatable::MAP:
    case Evaluatable::DISABLED_MAP:
        out << " Kleene caching in maps.";
        break;
    case Evaluatable::VECTOR:
        out << " Kleene caching in vectors of size " <<
        eval->kleeneEvaluationCacheVector.size() << ".";
        break;
    }

    out << endl << endl;

    if (!eval->actionHashKeyMap.empty()) {
        out << "  Action Hash Key Map: " << endl;
        for (size_t i = 0; i < eval->actionHashKeyMap.size(); ++i) {
            if (eval->actionHashKeyMap[i] != 0) {
                out << "    ";
                actionStates[i].printCompact(out);
                out << " : " << eval->actionHashKeyMap[i] << endl;
            }
        }
    } else {
        out << "  Has no positive dependencies on actions." << endl;
    }

    out << "  Formula: " << endl;
    eval->formula->print(out);
    out << endl;
}
