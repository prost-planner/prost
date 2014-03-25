#include "preprocessor.h"

#include "planning_task.h"
#include "evaluatables.h"

#include "utils/math_utils.h"

#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;

void Preprocessor::preprocess() {
    // Create and initialize CPFs, rewardCPF, and SACs
    prepareEvaluatables();

    // Create action fluents and calculate legal action states
    prepareActions();

    // Approximate reachable values (domains) of CPFs and rewardCPF
    calculateDomains();

    // Remove CPFs with only one reachable value (i.e. a domain size of 1) and
    // simplify remaining CPFs, rewardCPF, and SACs
    finalizeEvaluatables();

    // Determinize CPFs
    determinize();

    // Determine some non-trivial properties
    determineTaskProperties();

    // Initialize Hash Key Bases and Mappings
    prepareStateHashKeys();
    prepareKleeneStateHashKeys();
    prepareStateFluentHashKeys();
}

/*****************************************************************
                       Basic Preprocesses
*****************************************************************/

void Preprocessor::prepareEvaluatables() {
    // Before we create the CPFs we remove those that simplify to their initial
    // value and replace them by their initial value in all other formulas
    map<StateFluent*, double> replacements;
    bool simplifyAgain = true;
    while(simplifyAgain) {
        simplifyAgain = false;
        for(unsigned int i = 0; i < task->CPFs.size(); ++i) {
            task->CPFs[i]->formula = task->CPFs[i]->formula->simplify(replacements);
            NumericConstant* nc = dynamic_cast<NumericConstant*>(task->CPFs[i]->formula);
            if(nc && MathUtils::doubleIsEqual(task->CPFs[i]->head->initialValue, nc->value)) {
                simplifyAgain = true;
                assert(replacements.find(task->CPFs[i]->head) == replacements.end());
                replacements[task->CPFs[i]->head] = nc->value;
                swap(task->CPFs[i], task->CPFs[task->CPFs.size()-1]);
                task->CPFs.pop_back();
                break;
            }
        }
    }

    // Remove state fluents that simplify to their initial value from the reward
    // and collect properties of reward
    task->rewardCPF->formula = task->rewardCPF->formula->simplify(replacements);
    task->rewardCPF->initialize();

    // Collect properties of CPFs
    for(unsigned int i = 0; i < task->CPFs.size(); ++i) {
        task->CPFs[i]->initialize();
    }

    // Sort CPFs for deterministic behaviour
    sort(task->CPFs.begin(), task->CPFs.end(), ConditionalProbabilityFunction::TransitionFunctionSort());

    // Distinguish deterministic and probabilistic CPFs
    vector<ConditionalProbabilityFunction*> detCPFs;
    vector<ConditionalProbabilityFunction*> probCPFs;
    for(unsigned int index = 0; index < task->CPFs.size(); ++index) {
        if(task->CPFs[index]->isProbabilistic()) {
            probCPFs.push_back(task->CPFs[index]);
        } else {
            detCPFs.push_back(task->CPFs[index]);
        }
    }
    task->CPFs.clear();
    task->CPFs.insert(task->CPFs.end(), detCPFs.begin(), detCPFs.end());
    task->CPFs.insert(task->CPFs.end(), probCPFs.begin(), probCPFs.end());
 
    // We set these indices as we have to evaluate formulas in the next step to
    // determine legal action combinations. These indices are still temporal,
    // though, as we remove state fluents with a rechable domain that only
    // includes the initial value later
    for(unsigned int i = 0; i < task->CPFs.size(); ++i) {
        task->CPFs[i]->setIndex(i);
    }

    // Create and initialize SACs, then divide them into dynamic SACs, static
    // SACs and state invariants.
    for(unsigned int index = 0; index < task->SACs.size(); ++index) {
        // Remove the state fluents from the SAC formula that simplify to their
        // initial value
        task->SACs[index] = task->SACs[index]->simplify(replacements);
        stringstream name;
        name << "SAC " << index;
        ActionPrecondition* sac = new ActionPrecondition(name.str(), task->SACs[index]);

        // Collect the properties of the SAC that are necessary to distinguish
        // action preconditions, static SACs and state invariants
        sac->initialize();
        if(sac->containsStateFluent()) {
             if(sac->containsActionFluent()) {
                 // An SAC that contain both state and action fluents
                 // must be evaluated like a precondition for actions.
                 sac->index = task->dynamicSACs.size();
                 task->dynamicSACs.push_back(sac);
            } else {
                // An SAC that only contains state fluents represents
                // a state invariant that must be true in every state.
                task->stateInvariants.push_back(sac);
            }
    	} else {
            // An SAC that only contains action fluents is used to
            // statically forbid action combinations.
            task->staticSACs.push_back(sac);
    	}
    }
}

void Preprocessor::prepareActions() {
    // Sort action fluents for deterministic behaviour and assign indices
    sort(task->actionFluents.begin(), task->actionFluents.end(), ActionFluent::ActionFluentSort());
    for(unsigned int index = 0; index < task->actionFluents.size(); ++index) {
        task->actionFluents[index]->index = index;
    }

    // Calculate all possible action combinations with up to
    // numberOfConcurrentActions concurrent actions
    list<vector<int> > actionCombinations;    
    calcPossiblyLegalActionStates(task->numberOfConcurrentActions, actionCombinations);

    State current(task->CPFs);

    // Remove all illegal action combinations by checking the SACs
    // that are state independent
    for(list<vector<int> >::iterator it = actionCombinations.begin(); it != actionCombinations.end(); ++it) {
        vector<int>& tmp = *it;

        ActionState actionState((int)task->actionFluents.size());
        for(unsigned int i = 0; i < tmp.size(); ++i) {
            actionState[tmp[i]] = 1;
        }

        bool isLegal = true;
        for(unsigned int i = 0; i < task->staticSACs.size(); ++i) {
            double res = 0.0;
            task->staticSACs[i]->formula->evaluate(res, current, actionState);
            if(MathUtils::doubleIsEqual(res, 0.0)) {
                isLegal = false;
                break;
            }
        }

        if(isLegal) {
            task->actionStates.push_back(actionState);
        }
    }

    // Sort action states for deterministic behaviour
    sort(task->actionStates.begin(), task->actionStates.end(), ActionState::ActionStateSort());

    // Set inidices and calculate properties of action states
    for(unsigned int i = 0; i < task->actionStates.size(); ++i) {
        ActionState& actionState = task->actionStates[i];
        actionState.index = i;

        for(unsigned int i = 0; i < actionState.state.size(); ++i) {
            if(actionState.state[i]) {
                actionState.scheduledActionFluents.push_back(task->actionFluents[i]);
            }
        }

        // Determine which dynamic SACs might make this action state
        // inapplicable
        for(unsigned int i = 0; i < task->dynamicSACs.size(); ++i) {
            if(task->dynamicSACs[i]->containsArithmeticFunction()) {
                // If the SAC contains an arithmetic function we treat it as if
                // it influenced this action. TODO: Implement a function that
                // checks if it does influence this!
                actionState.relevantSACs.push_back(task->dynamicSACs[i]);
            } else if(sacContainsNegativeActionFluent(task->dynamicSACs[i], actionState)) {
                // If the SAC contains one of this ActionStates' action fluents
                // negatively it might forbid this action.
                actionState.relevantSACs.push_back(task->dynamicSACs[i]);
            } else if(sacContainsAdditionalPositiveActionFluent(task->dynamicSACs[i], actionState)) {
                // If the SAC contains action fluents positively that are not in
                // this ActionStates' action fluents it might enforce that
                // action fluent (and thereby forbid this action)
                actionState.relevantSACs.push_back(task->dynamicSACs[i]);
            }
        }
    }
}

void Preprocessor::calcPossiblyLegalActionStates(int actionsToSchedule, list<vector<int> >& result, vector<int> addTo) const {
    int nextVal = 0;
    result.push_back(addTo);

    if(!addTo.empty()) {
        nextVal = addTo[addTo.size()-1]+1;
    }

    for(unsigned int i = nextVal; i < task->actionFluents.size(); ++i) {
        vector<int> copy = addTo;
        copy.push_back(i);
        if(actionsToSchedule > 0) {
            calcPossiblyLegalActionStates(actionsToSchedule -1, result, copy);
        }
    }
}

bool Preprocessor::sacContainsNegativeActionFluent(ActionPrecondition* const& sac, ActionState const& actionState) const {
    set<ActionFluent*> const& actionFluents = sac->getNegativeDependentActionFluents();

    for(unsigned int index = 0; index < actionState.scheduledActionFluents.size(); ++index) {
        if(actionFluents.find(actionState.scheduledActionFluents[index]) != actionFluents.end()) {
            return true;
        }
    }
    return false;
}

bool Preprocessor::sacContainsAdditionalPositiveActionFluent(ActionPrecondition* const& sac, ActionState const& actionState) const {
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

/*****************************************************************
             Domain Calculation / Reachability Analysis
*****************************************************************/

void Preprocessor::calculateDomains() {
    // Insert initial values to set of reachable values
    vector<set<double> > domains(task->CPFs.size());
    for(unsigned int index = 0; index < task->CPFs.size(); ++index) {
        domains[index].insert(task->CPFs[index]->getInitialValue());
    }

    // Simulate a run in the planning task but evaluate to all possible
    // outcomes. Terminate if the fixpoint iteration doesn't change anything
    // anymore
    int currentHorizon = 0;
    while(currentHorizon < task->horizon) {
        // The values that are reachable in this step
        vector<set<double> > reachable(task->CPFs.size());

        // For each cpf, apply all actions and collect all reachable values
        for(unsigned int varIndex = 0; varIndex < task->CPFs.size(); ++varIndex) {
            for(unsigned int actionIndex = 0; actionIndex < task->actionStates.size(); ++actionIndex) {
                set<double> actionDependentValues;
                task->CPFs[varIndex]->formula->calculateDomain(domains, task->actionStates[actionIndex], actionDependentValues);
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

    // We also need the domain of the rewardCPF for deadend / goal detection (in
    // fact, we only need upper and lower bounds, but we use the same function
    // here)
    set<double> rewardDomain;
    for(unsigned int actionIndex = 0; actionIndex < task->actionStates.size(); ++actionIndex) {
        set<double> actionDependentValues;
        task->rewardCPF->formula->calculateDomain(domains, task->actionStates[actionIndex], actionDependentValues);
        rewardDomain.insert(actionDependentValues.begin(), actionDependentValues.end());
    }

    // Set domains of CPFs and rewardCPF
    for(unsigned int index = 0; index < task->CPFs.size(); ++index) {
        task->CPFs[index]->domain = domains[index];
    }
    task->rewardCPF->domain = rewardDomain;
}

void Preprocessor::finalizeEvaluatables() {
    // All CPFs with a domain that only includes its initial value are removed
    map<StateFluent*, double> replacements;
    for(vector<ConditionalProbabilityFunction*>::iterator it = task->CPFs.begin(); it != task->CPFs.end(); ++it) {
        assert(!(*it)->getDomainSize() == 0);

        if((*it)->getDomainSize() == 1) {
            // As the initial value must be included, the only value must be the
            // initial value
            replacements[(*it)->head] = (*it)->getInitialValue();
            task->CPFs.erase(it);
            --it;
        }
    }

    // Reset all indices and simplify cpfs by replacing all previously removed
    // CPFS with their constant initial value
    for(unsigned int index = 0; index < task->CPFs.size(); ++index) {
        task->CPFs[index]->setIndex(index);
        task->CPFs[index]->formula = task->CPFs[index]->formula->simplify(replacements);
        assert(!dynamic_cast<NumericConstant*>(task->CPFs[index]->formula));
    }

    // Simplify rewardCPF
    task->rewardCPF->formula = task->rewardCPF->formula->simplify(replacements);
    //task->rewardCPF->determinizedFormula = task->rewardCPF->formula;

    // Simplify dynamicSACs and check if they have become a state invariant
    for(unsigned int i = 0; i < task->dynamicSACs.size(); ++i) {
        task->dynamicSACs[i]->formula = task->dynamicSACs[i]->formula->simplify(replacements);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(task->dynamicSACs[i]->formula);
        if(nc) {
            assert(!MathUtils::doubleIsEqual(nc->value, 0.0));
            // This SAC is not dynamic anymore after simplification as it
            // simplifies to a state invariant
            task->stateInvariants.push_back(task->dynamicSACs[i]);
            swap(task->dynamicSACs[i], task->dynamicSACs[task->dynamicSACs.size()-1]);
            task->dynamicSACs.pop_back();
            --i;
        } //else {
            //task->dynamicSACs[i]->determinizedFormula = task->dynamicSACs[i]->formula;
        //}
    }

    for(unsigned int index = 0; index < task->dynamicSACs.size(); ++index) {
        task->dynamicSACs[index]->index = index;
    }
}

/*****************************************************************
                        Determinization
*****************************************************************/

void Preprocessor::determinize() {
    // Calculate determinzation of CPFs. Bernoulli statements are replaced with
    // <= randomNumberReplacement. It is variable because it might be
    // interesting to try other values as well.
    NumericConstant* randomNumberReplacement = new NumericConstant(0.5);

    map<StateFluent*, double> replacements;
    for(unsigned int index = 0; index < task->CPFs.size(); ++index) {
        if(task->CPFs[index]->isProbabilistic()) {
            task->CPFs[index]->determinization = task->CPFs[index]->formula->determinizeMostLikely(randomNumberReplacement);
            task->CPFs[index]->determinization = task->CPFs[index]->determinization->simplify(replacements);
        }
    }
}

/*****************************************************************
               Calculation of non-trivial properties
*****************************************************************/

void Preprocessor::determineTaskProperties() {
    // TODO: goalTestActionIndex should actually be a vector that contains all
    // actions indices that could potentially be the action that is necessary to
    // stay in a goal. Currently, we just set it to noop or to -1, as there is
    // no domain with a "stayInGoal" action that is different from noop.
    // Moreover, it could also be that there are several such actions, e.g.
    // stayInGoal_1 and stayInGoal_2 which are used for different goal states.
    // What we do currently is sound but makes the rewardLockDetection even more
    // incomplete.
    if(!task->rewardCPF->getPositiveDependentActionFluents().empty()) {
        // If the reward can be influenced positively by action fluents, we omit
        // reward lock detection. TODO: In a first step, we could check if all
        // positive action fluents are only positive and if the action state
        // that consist of all positive action fluents is always applicable.
        // Then we can use that action state.
        task->noopIsOptimalFinalAction = false;
        task->rewardFormulaAllowsRewardLockDetection = false;
    } else {
        task->rewardFormulaAllowsRewardLockDetection = true;

        if(task->actionStates[0].scheduledActionFluents.empty() && task->actionStates[0].relevantSACs.empty()) {
            // If no action fluent occurs positively in the reward, if noop is
            // not forbidden due to static SACs, and if noop has no SACs that
            // might make it inapplicable, noop is always at least as good as
            // other actions in the final state transition.
            task->noopIsOptimalFinalAction = true;
        } else {
            task->noopIsOptimalFinalAction = false;
        }
    }
}

/*****************************************************************
                         HashKeys
*****************************************************************/

// Check if hashing of States is possible, and assign hash key bases if so
void Preprocessor::prepareStateHashKeys() {
    bool stateHashingPossible = true;
    long nextHashKeyBase = 1;

    for(unsigned int index = 0; index < task->CPFs.size(); ++index) {
        task->stateHashKeys.push_back(vector<long>(task->CPFs[index]->getDomainSize()));
        for(unsigned int valueIndex = 0; valueIndex < task->CPFs[index]->getDomainSize(); ++valueIndex) {
            task->stateHashKeys[index][valueIndex] = (valueIndex * nextHashKeyBase);
        }

        if(!task->CPFs[index]->hasFiniteDomain() || !MathUtils::multiplyWithOverflowCheck(nextHashKeyBase, task->CPFs[index]->getDomainSize())) {
            stateHashingPossible = false;
            break;
        }
    }

    if(!stateHashingPossible) {
        task->stateHashKeys.clear();
    }
}

// Check if hashing of KleeneStates is possible, and assign hash key bases if so
void Preprocessor::prepareKleeneStateHashKeys() {
    bool kleeneStateHashingPossible = true;

    // We start by calculating the kleene
    for(unsigned int index = 0; index < task->CPFs.size(); ++index) {
        // The number of possible values of this variable in Kleene states is
        // 2^0 + 2^1 + ... + 2^{n-1} = 2^n -1 with n = CPFs[i]->domain.size()
        task->CPFs[index]->kleeneDomainSize = 2;
        if(!MathUtils::toThePowerOfWithOverflowCheck(task->CPFs[index]->kleeneDomainSize, task->CPFs[index]->getDomainSize())) {
            // KleeneState hashing is impossible because the kleeneDomain of
            // this CPF is too large. We nevertheless compute the other
            // kleenDomainSizes as they can still be used for
            // kleeneStateFluentHashKeys
            kleeneStateHashingPossible = false;
            task->CPFs[index]->kleeneDomainSize = 0;
        } else {
            --task->CPFs[index]->kleeneDomainSize;
        }
    }

    if(!kleeneStateHashingPossible) {
        return;
    }

    long nextHashKeyBase = 1;
    for(unsigned int index = 0; index < task->CPFs.size(); ++index) {
        // Set the kleeneStateHashKeyBase
        task->kleeneStateHashKeyBases.push_back(nextHashKeyBase);

        if(!MathUtils::multiplyWithOverflowCheck(nextHashKeyBase, task->CPFs[index]->kleeneDomainSize)) {
            kleeneStateHashingPossible = false;
            break;
        }
    }

    if(!kleeneStateHashingPossible) {
        task->kleeneStateHashKeyBases.clear();
    }
}

// Check if state hashing is possible with states as probability
// distributuions, and assign prob hash key bases if so
// void Preprocessor::preparePDStateHashKeys() {
    //REPAIR
    //pdStateHashingPossible = false;
    //return;
    // // Assign hash key bases
    // pdStateHashingPossible = true;
    // long nextHashKeyBase = 1;
    // vector<long> hashKeyBases;
    // for(unsigned int i = 0; i < CPFs.size(); ++i) {
    //     hashKeyBases.push_back(nextHashKeyBase);
    //     if(!MathUtils::multiplyWithOverflowCheck(nextHashKeyBase, CPFs[i]->probDomainMap.size())) {
    //         pdStateHashingPossible = false;
    //         return;
    //     }
    // }

    // assert(hashKeyBases.size() == CPFs.size());

    // for(unsigned int index = 0; index < hashKeyBases.size(); ++index) {
    //     for(map<double, long>::iterator it = CPFs[index]->probDomainMap.begin(); it != CPFs[index]->probDomainMap.end(); ++it) {
    //         it->second *= hashKeyBases[index];
    //     }
    // }
// }

void Preprocessor::prepareStateFluentHashKeys() {
    task->indexToStateFluentHashKeyMap.resize(task->CPFs.size());
    task->indexToKleeneStateFluentHashKeyMap.resize(task->CPFs.size());

    int hashIndex = 0;

    for(; hashIndex < task->CPFs.size(); ++hashIndex) {
        task->CPFs[hashIndex]->hashIndex = hashIndex;
        task->CPFs[hashIndex]->initializeHashKeys(task);
    }
    task->rewardCPF->hashIndex = hashIndex;
    task->rewardCPF->initializeHashKeys(task);

    for(unsigned int i = 0; i < task->dynamicSACs.size(); ++i) {
        ++hashIndex;
        task->dynamicSACs[i]->hashIndex = hashIndex;
        task->dynamicSACs[i]->initializeHashKeys(task);
    }
}
