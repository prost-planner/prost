#include "preprocessor.h"

#include "prost_planner.h"
#include "planning_task.h"

#include <iostream>
#include <algorithm>

using namespace std;

PlanningTask* Preprocessor::preprocess(map<string, int>& stateVariableIndices, vector<vector<string> >& stateVariableValues) {
    // Create and initialize SACs
    vector<Evaluatable*> dynamicSACs;
    vector<Evaluatable*> staticSACs;
    vector<Evaluatable*> stateInvariants;
    prepareSACs(dynamicSACs, staticSACs, stateInvariants);

    // Create and initialize CPFs and rewardCPF
    vector<ConditionalProbabilityFunction*> cpfs;
    RewardFunction* rewardCPF = NULL;
    prepareCPFs(cpfs, rewardCPF);

    // Create action fluents and action states
    vector<ActionFluent*> actionFluents;
    vector<ActionState> actionStates;
    prepareActions(cpfs, staticSACs, dynamicSACs, actionFluents, actionStates);

    // Calculate domains of CPFs and rewardCPF
    calculateDomains(cpfs, rewardCPF, actionStates);

    // Finalize CPFs, rewardCPF, and SACs
    finalizeFormulas(dynamicSACs, stateInvariants, cpfs, rewardCPF);

    // Create initial state
    State initialState(cpfs.size(), task->horizon, cpfs.size() + dynamicSACs.size() + 1);
    for(unsigned int i = 0; i < cpfs.size(); ++i) {
        initialState[i] = cpfs[i]->getInitialValue();
    }

    probPlanningTask->initialize(cpfs,
                                 rewardCPF,
                                 initialState,
                                 dynamicSACs,
                                 staticSACs,
                                 stateInvariants,
                                 actionFluents,
                                 actionStates,
                                 task->numberOfConcurrentActions,
                                 task->horizon,
                                 task->discountFactor,
                                 stateVariableIndices,
                                 stateVariableValues);

    return probPlanningTask->determinizeMostLikely();
}

void Preprocessor::prepareSACs(vector<Evaluatable*>& dynamicSACs, vector<Evaluatable*>& staticSACs, vector<Evaluatable*>& stateInvariants) {
    // Create and initialize SACs, then divide them into dynamic SACs, static
    // SACs and state invariants.
    for(unsigned int index = 0; index < task->SACs.size(); ++index) {
        // This simplification is only to get rid of stuff like KronDelta, it
        // doesn't do anything else
        map<StateFluent*, double> replacements;
        task->SACs[index] = task->SACs[index]->simplify(replacements);
        stringstream name;
        name << "SAC " << index;
        Evaluatable* sac = new Evaluatable(name.str(), task->SACs[index]);
        sac->initialize();
        if(sac->containsStateFluent()) {
             if(sac->containsActionFluent()) {
                 // An SAC that contain both state and action fluents
                 // must be evaluated like a precondition for actions.
                 dynamicSACs.push_back(sac);
            } else {
                // An SAC that only contains state fluents represents
                // a state invariant that must be true in every state.
                stateInvariants.push_back(sac);
            }
    	} else {
            // An SAC that only contains action fluents is used to
            // statically forbid action combinations.
            staticSACs.push_back(sac);
    	}
    }
}

void Preprocessor::prepareCPFs(vector<ConditionalProbabilityFunction*>& cpfs, RewardFunction*& rewardCPF) {
    // Before we create the CPFs we remove those that simplify to their initial
    // value (we simplify again later, though)
    map<StateFluent*, double> replacements;
    bool simplifyAgain = true;
    while(simplifyAgain) {
        simplifyAgain = false;
        for(unsigned int i = 0; i < task->CPFs.size(); ++i) {
            task->CPFs[i].second = task->CPFs[i].second->simplify(replacements);
            NumericConstant* nc = dynamic_cast<NumericConstant*>(task->CPFs[i].second);
            if(nc && MathUtils::doubleIsEqual(task->CPFs[i].first->initialValue, nc->value)) {
                simplifyAgain = true;
                assert(replacements.find(task->CPFs[i].first) == replacements.end());
                replacements[task->CPFs[i].first] = nc->value;
                swap(task->CPFs[i], task->CPFs[task->CPFs.size()-1]);
                task->CPFs.pop_back();
                break;
            }
        }
    }

    // Initialize reward CPF
    task->rewardCPF = task->rewardCPF->simplify(replacements);
    rewardCPF = new RewardFunction(task->rewardCPF);
    rewardCPF->initialize();

    // Create and initialize CPFs
    vector<ConditionalProbabilityFunction*> tmpCPFs;
    for(unsigned int i = 0; i < task->CPFs.size(); ++i) {
        ConditionalProbabilityFunction* cpf = new ConditionalProbabilityFunction(task->CPFs[i].first, task->CPFs[i].second);
        cpf->initialize();
        tmpCPFs.push_back(cpf);
    }

    // Sort CPFs for deterministic behaviour
    sort(tmpCPFs.begin(), tmpCPFs.end(), ConditionalProbabilityFunction::TransitionFunctionSort());

    // Distinguish deterministic and probabilistic CPFs
    vector<ConditionalProbabilityFunction*> probCPFs;
    for(unsigned int index = 0; index < tmpCPFs.size(); ++index) {
        if(tmpCPFs[index]->isProbabilistic()) {
            probCPFs.push_back(tmpCPFs[index]);
        } else {
            cpfs.push_back(tmpCPFs[index]);
        }
    }

    cpfs.insert(cpfs.end(), probCPFs.begin(), probCPFs.end());
 
    // Set (temporal) indices
    for(unsigned int i = 0; i < cpfs.size(); ++i) {
        cpfs[i]->setIndex(i);
    }
}

 void Preprocessor::prepareActions(vector<ConditionalProbabilityFunction*> const& cpfs,
                                   vector<Evaluatable*> const& staticSACs,
                                   vector<Evaluatable*> const& dynamicSACs,
                                   vector<ActionFluent*>& actionFluents,
                                   vector<ActionState>& actionStates) {
     // Initialize action fluents
     unsigned int index = 0;
     for(map<string, ActionFluent*>::iterator it = task->actionFluents.begin(); it != task->actionFluents.end(); ++it) {
         it->second->index = index;
         ++index;
         actionFluents.push_back(it->second);
     }

    // Calculate all possible action combinations with up to
    // numberOfConcurrentActions concurrent actions
    list<vector<int> > actionCombinations;    
    calcPossiblyLegalActionStates(actionFluents, task->numberOfConcurrentActions, actionCombinations);

    State current(cpfs.size(), 0, 0);
    for(unsigned int i = 0; i < cpfs.size(); ++i) {
        current[i] = cpfs[i]->getInitialValue();
    }

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
            double res = 0.0;
            staticSACs[i]->evaluate(res, current, actionState);
            if(MathUtils::doubleIsEqual(res, 0.0)) {
                isLegal = false;
                break;
            }
        }

        if(isLegal) {
            actionStates.push_back(actionState);
        }
    }

    // Sort action states for deterministic behaviour
    sort(actionStates.begin(), actionStates.end(), ActionState::ActionStateSort());

    // Set inidices and calculate properties of action states
    for(unsigned int i = 0; i < actionStates.size(); ++i) {
        ActionState& actionState = actionStates[i];
        actionState.index = i;

        for(unsigned int i = 0; i < actionState.state.size(); ++i) {
            if(actionState.state[i]) {
                actionState.scheduledActionFluents.push_back(actionFluents[i]);
            }
        }

        // Determine which dynamic SACs might make this action state
        // inapplicable
        for(unsigned int i = 0; i < dynamicSACs.size(); ++i) {
            if(dynamicSACs[i]->containsArithmeticFunction()) {
                // If the SAC contains an arithmetic function we treat it as if
                // it influenced this action. TODO: Implement a function that
                // checks if it does influence this!
                actionState.relevantSACs.push_back(dynamicSACs[i]);
            } else if(sacContainsNegativeActionFluent(dynamicSACs[i], actionState)) {
                // If the SAC contains one of this ActionStates' action fluents
                // negatively it might forbid this action.
                actionState.relevantSACs.push_back(dynamicSACs[i]);
            } else if(sacContainsAdditionalPositiveActionFluent(dynamicSACs[i], actionState)) {
                // If the SAC contains action fluents positively that are not in
                // this ActionStates' action fluents it might enforce that
                // action fluent (and thereby forbid this action)
                actionState.relevantSACs.push_back(dynamicSACs[i]);
            }
        }
    }
}

void Preprocessor::calcPossiblyLegalActionStates(vector<ActionFluent*> const& actionFluents, int actionsToSchedule, list<vector<int> >& result, vector<int> addTo) const {
    int nextVal = 0;
    result.push_back(addTo);

    if(!addTo.empty()) {
        nextVal = addTo[addTo.size()-1]+1;
    }

    for(unsigned int i = nextVal; i < actionFluents.size(); ++i) {
        vector<int> copy = addTo;
        copy.push_back(i);
        if(actionsToSchedule > 0) {
            calcPossiblyLegalActionStates(actionFluents, actionsToSchedule -1, result, copy);
        }
    }
}

bool Preprocessor::sacContainsNegativeActionFluent(Evaluatable* const& sac, ActionState const& actionState) const {
    set<ActionFluent*> const& actionFluents = sac->getNegativeDependentActionFluents();

    for(unsigned int index = 0; index < actionState.scheduledActionFluents.size(); ++index) {
        if(actionFluents.find(actionState.scheduledActionFluents[index]) != actionFluents.end()) {
            return true;
        }
    }
    return false;
}

bool Preprocessor::sacContainsAdditionalPositiveActionFluent(Evaluatable* const& sac, ActionState const& actionState) const {
    set<ActionFluent*> const& actionFluents = sac->getPositiveDependentActionFluents();

    for(set<ActionFluent*>::iterator it = actionFluents.begin(); it != actionFluents.end(); ++it) {
        bool isScheduledActionFluent = false;
        for(unsigned int index = 0; index < actionState.scheduledActionFluents.size(); ++index) {
            if((*it) == actionState.scheduledActionFluents[index]) {
                isScheduledActionFluent = true;
                break;
            }
        }
        if(!isScheduledActionFluent) {
            return true;
        }
    }

    return false;
}

void Preprocessor::calculateDomains(vector<ConditionalProbabilityFunction*>& cpfs,
                                    RewardFunction*& rewardCPF,
                                    vector<ActionState> const& actionStates) {
    // Insert initial values to set of reachable values
    vector<set<double> > domains(cpfs.size());
    for(unsigned int index = 0; index < cpfs.size(); ++index) {
        domains[index].insert(cpfs[index]->getInitialValue());
    }

    // Simulate a run in the planning task but evaluate to all possible
    // outcomes. Terminate if the fixpoint iteration doesn't change anything
    // anymore
    int currentHorizon = 0;
    while(currentHorizon < task->horizon) {
        // The values that are reachable in this step
        vector<set<double> > reachable(cpfs.size());

        // For each cpf, apply all actions and add the reachable values to the
        // according vector
        for(unsigned int varIndex = 0; varIndex < cpfs.size(); ++varIndex) {
            for(unsigned int actionIndex = 0; actionIndex < actionStates.size(); ++actionIndex) {
                set<double> actionDependentValues;
                cpfs[varIndex]->formula->calculateDomain(domains, actionStates[actionIndex], actionDependentValues);
                assert(!actionDependentValues.empty());
                reachable[varIndex].insert(actionDependentValues.begin(), actionDependentValues.end());
            }
        }

        // All possible value of this step have been computed. Check if there
        // are additional values, if so insert them and continue, otherwise the
        // fixpoint iteration has reached its end
        bool someDomainChanged = false;
        for(unsigned int varIndex = 0; varIndex < domains.size(); ++varIndex) {
            int sizeBefore = domains[varIndex].size();
            domains[varIndex].insert(reachable[varIndex].begin(), reachable[varIndex].end());
            if(domains[varIndex].size() != sizeBefore) {
                someDomainChanged = true;
            }
        }

        if(!someDomainChanged) {
            break;
        }

        ++currentHorizon;
    }

    // Albeit for different reasons (deadend / goal detection), we also need the
    // domain of the rewardCPF. TODO: Move this where it belongs, and introduce
    // a class for the rewardCPF.
    set<double> rewardDomain;
    for(unsigned int actionIndex = 0; actionIndex < actionStates.size(); ++actionIndex) {
        set<double> actionDependentValues;
        rewardCPF->formula->calculateDomain(domains, actionStates[actionIndex], actionDependentValues);
        rewardDomain.insert(actionDependentValues.begin(), actionDependentValues.end());
    }

    // Set domains of CPFs and rewardCPF
    for(unsigned int index = 0; index < cpfs.size(); ++index) {
        cpfs[index]->setDomain(domains[index]);
    }
    rewardCPF->setDomain(rewardDomain);
}

void Preprocessor::finalizeFormulas(vector<Evaluatable*>& dynamicSACs,
                                    vector<Evaluatable*>& stateInvariants,
                                    vector<ConditionalProbabilityFunction*>& cpfs,
                                    RewardFunction*& rewardCPF) {
    // All CPFs with a domain that only includes its initial value are removed
    map<StateFluent*, double> replacements;
    for(vector<ConditionalProbabilityFunction*>::iterator it = cpfs.begin(); it != cpfs.end(); ++it) {
        assert(!(*it)->getDomainSize() == 0);

        if((*it)->getDomainSize() == 1) {
            // As the initial value must be included, the only value must be the
            // initial value
            replacements[(*it)->getHead()] = (*it)->getInitialValue();
            cpfs.erase(it);
            --it;
        }
    }

    // Reset all indices and simplify cpfs by replacing all previously removed
    // CPFS with their constant initial value
    for(unsigned int index = 0; index < cpfs.size(); ++index) {
        cpfs[index]->setIndex(index);
        cpfs[index]->formula = cpfs[index]->formula->simplify(replacements);
        assert(!dynamic_cast<NumericConstant*>(cpfs[index]->formula));
    }

    // Simplify rewardCPF
    rewardCPF->formula = rewardCPF->formula->simplify(replacements);

    // Simplify dynamicSACs and check if they have become a state invariant
    for(unsigned int i = 0; i < dynamicSACs.size(); ++i) {
        dynamicSACs[i]->formula = dynamicSACs[i]->formula->simplify(replacements);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(dynamicSACs[i]->formula);
        if(nc) {
            assert(!MathUtils::doubleIsEqual(nc->value, 0.0));
            // This SAC is not dynamic anymore after simplification as it
            // simplifies to a state invariant
            stateInvariants.push_back(dynamicSACs[i]);
            swap(dynamicSACs[i], dynamicSACs[dynamicSACs.size()-1]);
            dynamicSACs.pop_back();
            --i;
        }
    }

    // Calculate Kleene domain of CPFs
    for(unsigned int index = 0; index < cpfs.size(); ++index) {
        // The number of possible values of this variable in Kleene states is 2^0 +
        // 2^1 + ... + 2^{n-1} = 2^n -1 with n = CPFs[i]->domain.size()
        long kleeneDomainSize = 2;
        if(!MathUtils::toThePowerOfWithOverflowCheck(kleeneDomainSize, cpfs[index]->getDomainSize())) {
            kleeneDomainSize = 0;
        } else {
            --kleeneDomainSize;
        }
        cpfs[index]->setKleeneDomainSize(kleeneDomainSize);
    }

    // Calculate PDState domain
}

