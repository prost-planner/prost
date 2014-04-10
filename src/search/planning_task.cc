#include "planning_task.h"

#include "search_engine.h"
#include "logical_expressions.h"

#include <cassert>
#include <iostream>

using namespace std;

/*****************************************************************
                         Initialization
*****************************************************************/

// TODO: Move those that exist for KleeneStates and States to a new class
// AbstractState

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

string PlanningTask::name;
vector<State> PlanningTask::trainingSet;
vector<ActionFluent*> PlanningTask::actionFluents;
vector<ActionState> PlanningTask::actionStates;
vector<StateFluent*> PlanningTask::stateFluents;
vector<ConditionalProbabilityFunction*> PlanningTask::CPFs;
RewardFunction* PlanningTask::rewardCPF = NULL;
vector<Evaluatable*> PlanningTask::actionPreconditions;
bool PlanningTask::isDeterministic = true;
State PlanningTask::initialState;
int PlanningTask::horizon = numeric_limits<int>::max();
double PlanningTask::discountFactor = 1.0;
int PlanningTask::numberOfActions = -1;
int PlanningTask::firstProbabilisticVarIndex = -1;
PlanningTask::FinalRewardCalculationMethod PlanningTask::finalRewardCalculationMethod = NOOP;
vector<int> PlanningTask::candidatesForOptimalFinalAction;

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

/******************************************************************
                            Printers
******************************************************************/

void PlanningTask::print(ostream& out) {
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

    //out << "This task is " << (isPruningEquivalentToDet? "" : "not ") << "pruning equivalent to its most likely determinization" << endl;

    out << "Hashing of States is " << (State::stateHashingPossible? "" : "not ") << "possible." << endl;
    out << "Hashing of KleeneStates is " << (KleeneState::stateHashingPossible? "" : "not ") << "possible." << endl;
    // if(!deterministic) {
    //     out << "Hashing of PDStates is " << (pdStateHashingPossible? "" : "not ") << "possible." << endl;
    // }

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
        out << "This task contains unreasonable actions in both the original and the determinization." << endl;
    } else if(ProbabilisticSearchEngine::hasUnreasonableActions) {
        assert(false);
    } else if(DeterministicSearchEngine::hasUnreasonableActions) {
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

void PlanningTask::printCPFInDetail(ostream& out, int const& index) {
    printEvaluatableInDetail(out, CPFs[index]);

    out << "  Domain: ";
    for(unsigned int i = 0; i < CPFs[index]->head->values.size(); ++i) {
        out << CPFs[index]->head->values[i] << " ";
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

void PlanningTask::printRewardCPFInDetail(ostream& out) {
    printEvaluatableInDetail(out, rewardCPF);

    out << "Min Reward: " << rewardCPF->getMinVal() << endl;
    out << "Max Reward: " << rewardCPF->getMaxVal() << endl;
}

void PlanningTask::printActionPreconditionInDetail(ostream& out, int const& index) {
    printEvaluatableInDetail(out, actionPreconditions[index]);
}

void PlanningTask::printEvaluatableInDetail(ostream& out, Evaluatable* eval) {
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

    if(eval->isProbabilistic()) {
        out << "  Determinized formula: " << endl;
        out << "    ";
        eval->determinizedFormula->print(out);
        out << endl;
    }

    out << endl;
}
