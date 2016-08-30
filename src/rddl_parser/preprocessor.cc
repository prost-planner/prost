#include "preprocessor.h"

#include "evaluatables.h"

#include "rddl.h"

#include "utils/math_utils.h"
#include "utils/system_utils.h"
#include "utils/timer.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "utils/timer.h"

using namespace std;

void Preprocessor::preprocess(bool const& output) {
    Timer t;
    // Create and initialize CPFs, rewardCPF, and SACs
    if (output)
        cout << "    Preparing evaluatables..." << endl;
    prepareEvaluatables();
    if (output)
        cout << "    ...finished (" << t() << ")" << endl;
    t.reset();

    // Create action fluents and calculate legal action states
    if (output)
        cout << "    Preparing actions..." << endl;
    prepareActions();
    if (output)
        cout << "    ...finished (" << t() << ")" << endl;
    t.reset();

    // Approximate reachable values (domains) of CPFs
    if (output)
        cout << "    Calculating CPF domain..." << endl;
    calculateCPFDomains();
    if (output)
        cout << "    ...finished (" << t() << ")" << endl;
    t.reset();

    // Remove CPFs with only one reachable value (i.e. a domain size of 1) and
    // simplify remaining CPFs, rewardCPF, and SACs
    if (output)
        cout << "    Finalizing evaluatables..." << endl;
    finalizeEvaluatables();
    if (output)
        cout << "    ...finished (" << t() << ")" << endl;
    t.reset();

    // Determinize CPFs
    if (output)
        cout << "    Computing determinization..." << endl;
    determinize();
    if (output)
        cout << "    ...finished (" << t() << ")" << endl;
    t.reset();

    // Determine some non-trivial properties
    if (output)
        cout << "    Determining task properties..." << endl;
    determineTaskProperties();
    if (output)
        cout << "    ...finished (" << t() << ")" << endl;
    t.reset();

    // Initialize Hash Key Bases and Mappings
    if (output)
        cout << "    Preparing hash keys..." << endl;
    prepareStateHashKeys();
    prepareKleeneStateHashKeys();
    prepareStateFluentHashKeys();
    if (output)
        cout << "    ...finished (" << t() << ")" << endl;
    t.reset();

    // Precompute results of evaluate for (some) evaluatables
    if (output)
        cout << "    Precomputing evaluatables..." << endl;
    precomputeEvaluatables();
    if (output)
        cout << "    ...finished (" << t() << ")" << endl;
    t.reset();

    // Approximate or calculate the min and max reward
    if (output)
        cout << "    Calculating min and max reward..." << endl;
    calculateMinAndMaxReward();
    if (output)
        cout << "    ...finished (" << t() << ")" << endl;
}

/*****************************************************************
                       Basic Preprocesses
*****************************************************************/

void Preprocessor::prepareEvaluatables() {
    // Before we create the CPFs we remove those that simplify to their initial
    // value and replace them by their initial value in all other evaluatables
    map<ParametrizedVariable*, double> replacements;
    bool simplifyAgain = true;
    while (simplifyAgain) {
        simplifyAgain = false;
        for (ConditionalProbabilityFunction*& cpf : task->CPFs) {
            cpf->simplify(replacements);
            NumericConstant* nc = dynamic_cast<NumericConstant*>(cpf->formula);
            if (nc &&
                MathUtils::doubleIsEqual(cpf->head->initialValue, nc->value)) {
                simplifyAgain = true;
                assert(replacements.find(cpf->head) == replacements.end());
                replacements[cpf->head] = nc->value;
                swap(cpf, task->CPFs[task->CPFs.size() - 1]);
                task->CPFs.pop_back();
                break;
            }
        }
    }

    // Remove state fluents that simplify to their initial value from the reward
    // and collect properties of reward
    task->rewardCPF->simplify(replacements);

    // Sort CPFs
    sort(task->CPFs.begin(), task->CPFs.end(),
         ConditionalProbabilityFunction::TransitionFunctionSort());

    // We set the CPF indices as we have to evaluate formulas in the next step
    // to determine legal action combinations. The indices are still temporal,
    // though, as it is possible that we remove additional state fluents later.
    for (unsigned int i = 0; i < task->CPFs.size(); ++i) {
        task->CPFs[i]->setIndex(i);
    }

    vector<LogicalExpression*> preconds;
    // Create and initialize SACs,then split all conjunctions in their elements
    for (unsigned int index = 0; index < task->SACs.size(); ++index) {
        // Remove the state fluents from the SAC formula that simplify to their
        // initial value
        task->SACs[index] = task->SACs[index]->simplify(replacements);

        // Split conjunctions into their elements

        Conjunction* conj = dynamic_cast<Conjunction*>(task->SACs[index]);
        if (conj) {
            preconds.insert(preconds.end(), conj->exprs.begin(),
                            conj->exprs.end());
        } else {
            preconds.push_back(task->SACs[index]);
        }
    }

    // Divide preconds into dynamic SACs, static SACs, primitive static SACs and
    // state invariants.
    for (unsigned int index = 0; index < preconds.size(); ++index) {
        stringstream name;
        name << "SAC " << index;
        ActionPrecondition* sac =
            new ActionPrecondition(name.str(), preconds[index]);

        // Collect the properties of the SAC that are necessary for distinction
        // of
        // action preconditions, (primitive) static SACs and state invariants
        sac->initialize();
        if (sac->containsStateFluent()) {
            // An SAC that contain both state and action fluents is an action
            // precondition, and an SAC that contains only state fluents is a
            // state invariant that is ignored in PROST.
            if (!sac->isActionIndependent()) {
                sac->index = task->actionPreconds.size();
                task->actionPreconds.push_back(sac);
            }
        } else {
            Negation* neg = dynamic_cast<Negation*>(sac->formula);
            if (neg) {
                ActionFluent* act = dynamic_cast<ActionFluent*>(neg->expr);
                if (act) {
                    // This is a "primitive" static action constraint of the
                    // form "not doSomething(params)", i.e., an sac that forbids
                    // the usage of an action fluent in general. We extract
                    // these early to make sure that the number of possibly
                    // legal action states in prepareActions() doesn't become
                    // too big.
                    task->primitiveStaticSACs.insert(act);
                } else {
                    task->staticSACs.push_back(sac);
                }
            } else {
                // An SAC that only contains action fluents is used to
                // statically forbid action combinations.
                task->staticSACs.push_back(sac);
            }
        }
    }
}

void Preprocessor::removeInapplicableActionFluents(
    bool const& updateActionStates) {
    // Check if there are action fluents that aren't used in any CPF or SAC
    vector<bool> fluentIsUsed(task->actionFluents.size(), false);

    for (unsigned int i = 0; i < task->actionPreconds.size(); ++i) {
        for (set<ActionFluent*>::const_iterator it =
                 task->actionPreconds[i]->dependentActionFluents.begin();
             it != task->actionPreconds[i]->dependentActionFluents.end();
             ++it) {
            fluentIsUsed[(*it)->index] = true;
        }
    }

    for (unsigned int i = 0; i < task->staticSACs.size(); ++i) {
        for (set<ActionFluent*>::const_iterator it =
                 task->staticSACs[i]->dependentActionFluents.begin();
             it != task->staticSACs[i]->dependentActionFluents.end(); ++it) {
            fluentIsUsed[(*it)->index] = true;
        }
    }

    for (unsigned int i = 0; i < task->CPFs.size(); ++i) {
        for (set<ActionFluent*>::const_iterator it =
                 task->CPFs[i]->dependentActionFluents.begin();
             it != task->CPFs[i]->dependentActionFluents.end(); ++it) {
            fluentIsUsed[(*it)->index] = true;
        }
    }

    for (set<ActionFluent*>::const_iterator it =
             task->rewardCPF->dependentActionFluents.begin();
         it != task->rewardCPF->dependentActionFluents.end(); ++it) {
        fluentIsUsed[(*it)->index] = true;
    }

    vector<ActionFluent*> afCopy = task->actionFluents;
    task->actionFluents.clear();
    int nextIndex = 0;

    for (unsigned int i = 0; i < fluentIsUsed.size(); ++i) {
        if (fluentIsUsed[i]) {
            afCopy[i]->index = nextIndex;
            ++nextIndex;

            task->actionFluents.push_back(afCopy[i]);
        }
    }

    if (updateActionStates) {
        set<ActionState> newActionStates;
        for (unsigned int i = 0; i < task->actionStates.size(); ++i) {
            ActionState& state = task->actionStates[i];
            ActionState newState(nextIndex);
            int newVarIndex = 0;

            assert(fluentIsUsed.size() == state.state.size());
            for (unsigned int j = 0; j < fluentIsUsed.size(); ++j) {
                if (fluentIsUsed[j]) {
                    newState[newVarIndex] = state[j];
                    ++newVarIndex;
                }
            }
            newActionStates.insert(newState);
        }

        task->actionStates.clear();
        task->actionStates.insert(task->actionStates.end(),
                                  newActionStates.begin(),
                                  newActionStates.end());

        initializeActionStates();
    } else {
        assert(task->actionStates.empty());
    }
}

void Preprocessor::prepareActions() {
    // Remove action fluents that are never reasonable
    removeInapplicableActionFluents(false);

    // Check if there are action fluents that can never be set to a nondefault
    // value due to a primitive static SAC (i.e., an action precondition of the
    // form ~a).
    vector<ActionFluent*> finalActionFluents;
    map<ParametrizedVariable*, double> replacements;

    if (task->primitiveStaticSACs.empty()) {
        finalActionFluents = task->actionFluents;
    } else {
        for (unsigned int index = 0; index < task->actionFluents.size();
             ++index) {
            if (task->primitiveStaticSACs.find(task->actionFluents[index]) ==
                task->primitiveStaticSACs.end()) {
                finalActionFluents.push_back(task->actionFluents[index]);
            } else {
                replacements[task->actionFluents[index]] = 0.0;
            }
        }

        task->actionFluents.clear();
        task->actionFluents = finalActionFluents;

        // Simplify all evaluatables by removing the unused action fluents from
        // their formula
        for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
            task->CPFs[index]->simplify(replacements);
        }

        task->rewardCPF->simplify(replacements);

        for (unsigned int index = 0; index < task->actionPreconds.size();
             ++index) {
            task->actionPreconds[index]->simplify(replacements);
            // TODO: What if this became static?
        }

        for (unsigned int index = 0; index < task->staticSACs.size(); ++index) {
            task->staticSACs[index]->simplify(replacements);
            // TODO: Check if this became a state invariant
        }

        replacements.clear();
    }

    // Sort action fluents for deterministic behaviour and assign (possibly
    // temporary) indices
    sort(task->actionFluents.begin(), task->actionFluents.end(),
         ActionFluent::ActionFluentSort());
    for (unsigned int index = 0; index < task->actionFluents.size(); ++index) {
        task->actionFluents[index]->index = index;
    }

    // Calculate all action states with up to numberOfConcurrentActions
    // concurrent actions TODO: Make sure this stops also if an static SAC is
    // violated that constrains the number of concurrently applicable actions.
    // Currently, if max-nondef-actions is not used, this will always produce
    // the power set over all action fluents.
    vector<ActionState> allActionStates;
    calcAllActionStates(allActionStates);

    // Remove all illegal action combinations by checking the SACs that are
    // state independent
    State current(task->CPFs);
    vector<ActionState> legalActionStates;

    for (ActionState const& actionState : allActionStates) {
        bool isLegal = true;
        for (unsigned int i = 0; i < task->staticSACs.size(); ++i) {
            double res = 0.0;
            task->staticSACs[i]->formula->evaluate(res, current, actionState);
            if (MathUtils::doubleIsEqual(res, 0.0)) {
                isLegal = false;
                break;
            }
        }

        if (isLegal) {
            legalActionStates.push_back(actionState);
        }
    }

    // Now that all legal actions have been created, we check if there are
    // action fluents that are false (TODO: that are equal to their default
    // value) in every legal action. We can safely remove those action fluents
    // entirely from each action.
    ActionState usedFluents((int)task->actionFluents.size());
    for (unsigned int i = 0; i < legalActionStates.size(); ++i) {
        for (unsigned int j = 0; j < legalActionStates[i].state.size(); ++j) {
            usedFluents[j] |= legalActionStates[i].state[j];
        }
    }

    // Assign new indices
    int newIndex = 0;
    for (unsigned int i = 0; i < usedFluents.state.size(); ++i) {
        if (usedFluents[i]) {
            task->actionFluents[i]->index = newIndex;
            ++newIndex;
        } else {
            task->actionFluents[i]->index = -1;
        }
    }

    if (newIndex < task->actionFluents.size()) {
        // Remove values of unused action fluents from action states
        for (unsigned int i = 0; i < legalActionStates.size(); ++i) {
            ActionState state(newIndex); // newIndex is equal to the number of
                                         // used action fluents
            for (unsigned int j = 0; j < legalActionStates[i].state.size();
                 ++j) {
                if (task->actionFluents[j]->index != -1) {
                    assert(task->actionFluents[j]->index < state.state.size());
                    state[task->actionFluents[j]->index] =
                        legalActionStates[i][j];
                } else {
                    replacements[task->actionFluents[j]] = 0.0;
                }
            }
            task->actionStates.push_back(state);
        }

        // Remove unused action fluents
        finalActionFluents.clear();
        for (unsigned int i = 0; i < task->actionFluents.size(); ++i) {
            if (task->actionFluents[i]->index != -1) {
                assert(task->actionFluents[i]->index ==
                       finalActionFluents.size());
                finalActionFluents.push_back(task->actionFluents[i]);
            }
        }
        task->actionFluents.clear();
        task->actionFluents = finalActionFluents;

        // Simplify all evaluatables by removing unused action fluents from the
        // formulas
        for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
            task->CPFs[index]->simplify(replacements);
        }

        task->rewardCPF->simplify(replacements);

        for (unsigned int index = 0; index < task->actionPreconds.size();
             ++index) {
            task->actionPreconds[index]->index = index;
        }
    } else {
        task->actionStates = legalActionStates;
    }

    initializeActionStates();
}

void Preprocessor::initializeActionStates() {
    // Sort action states again for deterministic behaviour
    sort(task->actionStates.begin(), task->actionStates.end(),
         ActionState::ActionStateSort());

    // Set inidices and calculate properties of action states
    for (unsigned int i = 0; i < task->actionStates.size(); ++i) {
        ActionState& actionState = task->actionStates[i];
        actionState.index = i;

        for (unsigned int i = 0; i < actionState.state.size(); ++i) {
            if (actionState.state[i]) {
                actionState.scheduledActionFluents.push_back(
                    task->actionFluents[i]);
            }
        }

        // Determine which dynamic SACs are relevant for this action
        for (unsigned int i = 0; i < task->actionPreconds.size(); ++i) {
            if (task->actionPreconds[i]->containsArithmeticFunction()) {
                // If the SAC contains an arithmetic function we treat it as if
                // it influenced this action. TODO: Implement a function that
                // checks if it does influence this!
                actionState.relevantSACs.push_back(task->actionPreconds[i]);
            } else if (sacContainsNegativeActionFluent(task->actionPreconds[i],
                                                       actionState)) {
                // If the SAC contains one of this ActionStates' action fluents
                // negatively it might forbid this action.
                actionState.relevantSACs.push_back(task->actionPreconds[i]);
            } else if (sacContainsAdditionalPositiveActionFluent(
                           task->actionPreconds[i], actionState)) {
                // If the SAC contains action fluents positively that are not in
                // this ActionStates' action fluents it might enforce that
                // action fluent (and thereby forbid this action)
                actionState.relevantSACs.push_back(task->actionPreconds[i]);
            }
        }
    }
}

void Preprocessor::calcAllActionStates(vector<ActionState>& result,
                                       int minElement,
                                       int scheduledActions) const {
    if (result.empty()) {
        result.push_back(ActionState((int)task->actionFluents.size()));
    } else {
        int lastIndex = result.size();

        for (unsigned int i = minElement; i < lastIndex; ++i) {
            for (unsigned int j = 0; j < task->actionFluents.size(); ++j) {
                if (!result[i][j]) {
                    bool isExtension = true;
                    for (unsigned int k = 0; k < j; ++k) {
                        if (result[i][k]) {
                            isExtension = false;
                            break;
                        }
                    }

                    if (isExtension) {
                        ActionState copy(result[i]);
                        copy[j] = 1;
                        result.push_back(copy);
                    }
                }
            }
        }
        minElement = lastIndex;
    }

    ++scheduledActions;
    if (scheduledActions <= task->numberOfConcurrentActions) {
        calcAllActionStates(result, minElement, scheduledActions);
    }
}

bool Preprocessor::sacContainsNegativeActionFluent(
    ActionPrecondition* const& sac, ActionState const& actionState) const {
    set<ActionFluent*> const& actionFluents = sac->negativeActionDependencies;

    for (unsigned int index = 0;
         index < actionState.scheduledActionFluents.size(); ++index) {
        if (actionFluents.find(actionState.scheduledActionFluents[index]) !=
            actionFluents.end()) {
            return true;
        }
    }
    return false;
}

bool Preprocessor::sacContainsAdditionalPositiveActionFluent(
    ActionPrecondition* const& sac, ActionState const& actionState) const {
    set<ActionFluent*> const& actionFluents = sac->positiveActionDependencies;

    for (set<ActionFluent*>::iterator it = actionFluents.begin();
         it != actionFluents.end(); ++it) {
        bool isScheduledActionFluent = false;
        for (unsigned int index = 0;
             index < actionState.scheduledActionFluents.size(); ++index) {
            if ((*it) == actionState.scheduledActionFluents[index]) {
                isScheduledActionFluent = true;
                break;
            }
        }
        if (!isScheduledActionFluent) {
            return true;
        }
    }

    return false;
}

/*****************************************************************
             Domain Calculation / Reachability Analysis
*****************************************************************/

void Preprocessor::calculateCPFDomains() {
    // Insert initial values to set of reachable values
    vector<set<double>> domains(task->CPFs.size());
    for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
        domains[index].insert(task->CPFs[index]->getInitialValue());
    }

    // Simulate a run in the planning task but evaluate to all possible
    // outcomes. Terminate if the fixpoint iteration doesn't change anything
    // anymore
    int currentHorizon = 0;
    while (currentHorizon < task->horizon) {
        // The values that are reachable in this step
        vector<set<double>> reachable(task->CPFs.size());

        // For each cpf, apply all actions and collect all reachable values
        for (unsigned int varIndex = 0; varIndex < task->CPFs.size();
             ++varIndex) {
            for (unsigned int actionIndex = 0;
                 actionIndex < task->actionStates.size(); ++actionIndex) {
                set<double> actionDependentValues;
                task->CPFs[varIndex]->formula->calculateDomain(
                    domains, task->actionStates[actionIndex],
                    actionDependentValues);
                assert(!actionDependentValues.empty());
                reachable[varIndex].insert(actionDependentValues.begin(),
                                           actionDependentValues.end());
            }
        }

        // All possible value of this step have been computed. Check if there
        // are additional values, if so insert them and continue, otherwise the
        // fixpoint iteration has reached its end
        bool someDomainChanged = false;
        for (unsigned int varIndex = 0; varIndex < domains.size(); ++varIndex) {
            int sizeBefore = domains[varIndex].size();
            domains[varIndex].insert(reachable[varIndex].begin(),
                                     reachable[varIndex].end());
            if (domains[varIndex].size() != sizeBefore) {
                someDomainChanged = true;
            }
        }

        if (!someDomainChanged) {
            break;
        }

        ++currentHorizon;
    }

    // Set domains
    for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
        task->CPFs[index]->domain = domains[index];
    }
}

void Preprocessor::finalizeEvaluatables() {
    // Remove all CPFs with a domain that only includes their initial value
    map<ParametrizedVariable*, double> replacements;
    for (vector<ConditionalProbabilityFunction*>::iterator it =
             task->CPFs.begin();
         it != task->CPFs.end(); ++it) {
        assert(!(*it)->getDomainSize() == 0);

        if ((*it)->getDomainSize() == 1) {
            // As the initial value must be included, the only value must be the
            // initial value
            assert(MathUtils::doubleIsEqual(*((*it)->domain.begin()),
                                            (*it)->getInitialValue()));

            replacements[(*it)->head] = (*it)->getInitialValue();
            task->CPFs.erase(it);
            --it;
        }
    }

    // Simplify CPFs by replacing all previously removed CPFs with their
    // constant initial value, then sort again and reset indices.
    for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
        task->CPFs[index]->simplify(replacements);
    }

    sort(task->CPFs.begin(), task->CPFs.end(),
         ConditionalProbabilityFunction::TransitionFunctionSort());

    for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
        task->CPFs[index]->setIndex(index);
    }

    // Simplify rewardCPF
    task->rewardCPF->simplify(replacements);

    // Simplify actionPreconds and check if they have become a state invariant
    for (unsigned int i = 0; i < task->actionPreconds.size(); ++i) {
        task->actionPreconds[i]->simplify(replacements);
        NumericConstant* nc =
            dynamic_cast<NumericConstant*>(task->actionPreconds[i]->formula);
        if (nc) {
            assert(!MathUtils::doubleIsEqual(nc->value, 0.0));
            // This SAC is not dynamic anymore after simplification as it
            // simplifies to a state invariant
            swap(task->actionPreconds[i],
                 task->actionPreconds[task->actionPreconds.size() - 1]);
            task->actionPreconds.pop_back();
            --i;
        }
    }

    for (unsigned int index = 0; index < task->actionPreconds.size(); ++index) {
        task->actionPreconds[index]->index = index;
    }

    // Finalize action fluents
    removeInapplicableActionFluents(true);
}

/*****************************************************************
                        Determinization
*****************************************************************/

void Preprocessor::determinize() {
    // Calculate determinzation of CPFs.
    map<ParametrizedVariable*, double> replacementsDummy;
    for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
        if (task->CPFs[index]->isProbabilistic()) {
            task->CPFs[index]->determinization =
                task->CPFs[index]->formula->determinizeMostLikely();
            task->CPFs[index]->determinization =
                task->CPFs[index]->determinization->simplify(replacementsDummy);
        }
    }
}

/*****************************************************************
               Calculation of non-trivial properties
*****************************************************************/

void Preprocessor::determineTaskProperties() {
    // Determine if there is a single action that could be goal maintaining,
    // i.e., that could always yield the maximal reward. TODO: We could use an
    // action that contains as many positive and no negative occuring fluents as
    // possible, but we should first figure out if reward lock detection still
    // pays off after switching to FDDs from BDDs.
    if (task->rewardCPF->positiveActionDependencies.empty() &&
        task->actionStates[0].scheduledActionFluents.empty()) {
        task->rewardFormulaAllowsRewardLockDetection = true;
    } else {
        task->rewardFormulaAllowsRewardLockDetection = false;
    }

    if (task->rewardCPF->positiveActionDependencies.empty() &&
        task->actionStates[0].scheduledActionFluents.empty() &&
        task->actionStates[0].relevantSACs.empty()) {
        // The first action is noop, noop is always applicable and action
        // fluents occur in the reward only as costs -> noop is always optimal
        // as final action
        task->finalRewardCalculationMethod = "NOOP";
    } else if (task->rewardCPF->isActionIndependent()) {
        // The reward formula does not contain any action fluents -> all actions
        // yield the same reward, so any action that is applicable is optimal
        task->finalRewardCalculationMethod = "FIRST_APPLICABLE";
    } else {
        task->finalRewardCalculationMethod = "BEST_OF_CANDIDATE_SET";
        // Determine the actions that suffice to be applied in the final step.
        for (unsigned int i = 0; i < task->actionStates.size(); ++i) {
            if (!actionStateIsDominated(i)) {
                addDominantState(i);
            }
        }
    }
}

void Preprocessor::addDominantState(int stateIndex) const {
    // cout << "Adding action state " <<
    // task->actionStates[stateIndex].getName() << endl;
    for (vector<int>::iterator it =
             task->candidatesForOptimalFinalAction.begin();
         it != task->candidatesForOptimalFinalAction.end(); ++it) {
        if (actionStateDominates(task->actionStates[stateIndex],
                                 task->actionStates[*it])) {
            // cout << "It dominates " << task->actionStates[*it].getName() <<
            // endl;
            task->candidatesForOptimalFinalAction.erase(it);
            --it;
        }
    }
    task->candidatesForOptimalFinalAction.push_back(stateIndex);
}

bool Preprocessor::actionStateIsDominated(int stateIndex) const {
    for (unsigned int i = 0; i < task->candidatesForOptimalFinalAction.size();
         ++i) {
        if (actionStateDominates(
                task->actionStates[task->candidatesForOptimalFinalAction[i]],
                task->actionStates[stateIndex])) {
            return true;
        }
    }
    return false;
}

bool Preprocessor::actionStateDominates(ActionState const& lhs,
                                        ActionState const& rhs) const {
    // An action state with preconditions cannot dominate another action state
    if (!lhs.relevantSACs.empty()) {
        return false;
    }

    // Determine all fluents of both action states that influence the reward
    // positively or negatively
    set<ActionFluent*> lhsPos;
    for (unsigned int i = 0; i < lhs.scheduledActionFluents.size(); ++i) {
        if (task->rewardCPF->positiveActionDependencies.find(
                lhs.scheduledActionFluents[i]) !=
            task->rewardCPF->positiveActionDependencies.end()) {
            lhsPos.insert(lhs.scheduledActionFluents[i]);
        }
    }

    set<ActionFluent*> rhsPos;
    for (unsigned int i = 0; i < rhs.scheduledActionFluents.size(); ++i) {
        if (task->rewardCPF->positiveActionDependencies.find(
                rhs.scheduledActionFluents[i]) !=
            task->rewardCPF->positiveActionDependencies.end()) {
            rhsPos.insert(rhs.scheduledActionFluents[i]);
        }
    }

    set<ActionFluent*> lhsNeg;
    for (unsigned int i = 0; i < lhs.scheduledActionFluents.size(); ++i) {
        if (task->rewardCPF->negativeActionDependencies.find(
                lhs.scheduledActionFluents[i]) !=
            task->rewardCPF->negativeActionDependencies.end()) {
            lhsNeg.insert(lhs.scheduledActionFluents[i]);
        }
    }

    set<ActionFluent*> rhsNeg;
    for (unsigned int i = 0; i < rhs.scheduledActionFluents.size(); ++i) {
        if (task->rewardCPF->negativeActionDependencies.find(
                rhs.scheduledActionFluents[i]) !=
            task->rewardCPF->negativeActionDependencies.end()) {
            rhsNeg.insert(rhs.scheduledActionFluents[i]);
        }
    }

    // Action state lhs dominates rhs if lhs contains all action fluents that
    // influence the reward positively of rhs, and if rhs contains all action
    // fluents that influence the reward negatively of lhs
    for (set<ActionFluent*>::iterator it = rhsPos.begin(); it != rhsPos.end();
         ++it) {
        if (lhsPos.find(*it) == lhsPos.end()) {
            return false;
        }
    }

    for (set<ActionFluent*>::iterator it = lhsNeg.begin(); it != lhsNeg.end();
         ++it) {
        if (rhsNeg.find(*it) == rhsNeg.end()) {
            return false;
        }
    }

    return true;
}

/*****************************************************************
                         HashKeys
*****************************************************************/

// Check if hashing of States is possible, and assign hash key bases if so
void Preprocessor::prepareStateHashKeys() {
    bool stateHashingPossible = true;
    long nextHashKeyBase = 1;

    for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
        ConditionalProbabilityFunction* cpf = task->CPFs[index];
        task->stateHashKeys.push_back(vector<long>(cpf->getDomainSize()));
        for (unsigned int valueIndex = 0; valueIndex < cpf->getDomainSize();
             ++valueIndex) {
            task->stateHashKeys[index][valueIndex] =
                (valueIndex * nextHashKeyBase);
        }

        if (!cpf->hasFiniteDomain() ||
            !MathUtils::multiplyWithOverflowCheck(nextHashKeyBase,
                                                  cpf->getDomainSize())) {
            stateHashingPossible = false;
            break;
        }
    }

    if (!stateHashingPossible) {
        task->stateHashKeys.clear();
    }
}

// Check if hashing of KleeneStates is possible, and assign hash key bases if so
void Preprocessor::prepareKleeneStateHashKeys() {
    bool kleeneStateHashingPossible = true;

    // We start by calculating the kleene
    for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
        // The number of possible values of this variable in Kleene states is
        // 2^0 + 2^1 + ... + 2^{n-1} = 2^n -1 with n = CPFs[i]->domain.size()
        task->CPFs[index]->kleeneDomainSize = 2;
        if (!MathUtils::toThePowerOfWithOverflowCheck(
                task->CPFs[index]->kleeneDomainSize,
                task->CPFs[index]->getDomainSize())) {
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

    if (!kleeneStateHashingPossible) {
        return;
    }

    long nextHashKeyBase = 1;
    for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
        // Set the kleeneStateHashKeyBase
        task->kleeneStateHashKeyBases.push_back(nextHashKeyBase);

        if (!MathUtils::multiplyWithOverflowCheck(
                nextHashKeyBase, task->CPFs[index]->kleeneDomainSize)) {
            kleeneStateHashingPossible = false;
            break;
        }
    }

    if (!kleeneStateHashingPossible) {
        task->kleeneStateHashKeyBases.clear();
    }
}

void Preprocessor::prepareStateFluentHashKeys() {
    task->indexToStateFluentHashKeyMap.resize(task->CPFs.size());
    task->indexToKleeneStateFluentHashKeyMap.resize(task->CPFs.size());

    int hashIndex = 0;

    for (; hashIndex < task->CPFs.size(); ++hashIndex) {
        task->CPFs[hashIndex]->hashIndex = hashIndex;
        task->CPFs[hashIndex]->initializeHashKeys(task);
    }
    task->rewardCPF->hashIndex = hashIndex;
    task->rewardCPF->initializeHashKeys(task);

    for (unsigned int i = 0; i < task->actionPreconds.size(); ++i) {
        ++hashIndex;
        task->actionPreconds[i]->hashIndex = hashIndex;
        task->actionPreconds[i]->initializeHashKeys(task);
    }
}

/*****************************************************************
                    Precalculate evaluatables
*****************************************************************/

void Preprocessor::precomputeEvaluatables() {
    for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
        if (task->CPFs[index]->cachingType == "VECTOR") {
            precomputeEvaluatable(task->CPFs[index]);
        }
    }

    if (task->rewardCPF->cachingType == "VECTOR") {
        precomputeEvaluatable(task->rewardCPF);
    }

    for (unsigned int index = 0; index < task->actionPreconds.size(); ++index) {
        if (task->actionPreconds[index]->cachingType == "VECTOR") {
            precomputeEvaluatable(task->actionPreconds[index]);
        }
    }
}

void Preprocessor::precomputeEvaluatable(Evaluatable* eval) {
    vector<StateFluent*> dependentStateFluents;
    for (set<StateFluent*>::iterator it = eval->dependentStateFluents.begin();
         it != eval->dependentStateFluents.end(); ++it) {
        dependentStateFluents.push_back(*it);
    }

    vector<State> relevantStates;
    if (!dependentStateFluents.empty()) {
        createRelevantStates(dependentStateFluents, relevantStates);
    } else {
        State s(task->CPFs.size());
        relevantStates.push_back(s);
    }

    for (unsigned int i = 0; i < relevantStates.size(); ++i) {
        long hashKey = calculateStateFluentHashKey(eval, relevantStates[i]);
        set<long> usedActionHashKeys;
        for (unsigned int j = 0; j < task->actionStates.size(); ++j) {
            long& actionHashKey = eval->actionHashKeyMap[j];
            if (usedActionHashKeys.find(actionHashKey) ==
                usedActionHashKeys.end()) {
                usedActionHashKeys.insert(actionHashKey);

                if (eval->isProbabilistic()) {
                    double res;
                    eval->determinization->evaluate(res, relevantStates[i],
                                                    task->actionStates[j]);
                    assert(MathUtils::doubleIsMinusInfinity(
                        eval->precomputedResults[hashKey + actionHashKey]));
                    eval->precomputedResults[hashKey + actionHashKey] = res;

                    DiscretePD pdRes;
                    eval->formula->evaluateToPD(pdRes, relevantStates[i],
                                                task->actionStates[j]);
                    assert(eval->precomputedPDResults[hashKey + actionHashKey]
                               .isUndefined());
                    eval->precomputedPDResults[hashKey + actionHashKey] = pdRes;
                } else {
                    double res;
                    eval->formula->evaluate(res, relevantStates[i],
                                            task->actionStates[j]);
                    assert(MathUtils::doubleIsMinusInfinity(
                        eval->precomputedResults[hashKey + actionHashKey]));
                    eval->precomputedResults[hashKey + actionHashKey] = res;
                }
            }
        }
    }
}

void Preprocessor::createRelevantStates(
    vector<StateFluent*>& dependentStateFluents, vector<State>& result) {
    StateFluent* fluent =
        dependentStateFluents[dependentStateFluents.size() - 1];
    dependentStateFluents.pop_back();

    int domainSize = -1;
    for (unsigned int i = 0; i < task->CPFs.size(); ++i) {
        if (task->CPFs[i]->head == fluent) {
            domainSize = task->CPFs[i]->domain.size();
            break;
        }
    }

    if (result.empty()) {
        for (unsigned int val = 0; val < domainSize; ++val) {
            State base(task->CPFs.size());
            base[fluent->index] = val;
            result.push_back(base);
        }
    } else {
        int size = result.size();
        for (unsigned int i = 0; i < size; ++i) {
            // The state with value '0' is already in the set, so we add append
            // all states with other values
            for (unsigned int val = 1; val < domainSize; ++val) {
                State copy(result[i]);
                assert(copy[fluent->index] == 0);
                copy[fluent->index] = val;
                result.push_back(copy);
            }
        }
    }

    if (!dependentStateFluents.empty()) {
        createRelevantStates(dependentStateFluents, result);
    }
}

long Preprocessor::calculateStateFluentHashKey(Evaluatable* eval,
                                               State const& state) const {
    long res = 0;
    for (unsigned int i = 0; i < eval->stateFluentHashKeyBases.size(); ++i) {
        res += state[eval->stateFluentHashKeyBases[i].first] *
               eval->stateFluentHashKeyBases[i].second;
    }
    return res;
}

void Preprocessor::calculateMinAndMaxReward() const {
    // Compute the domain of the rewardCPF for deadend / goal detection (we only
    // need upper and lower bounds, but we use the same function here). If the
    // cachingType is vector, we can use the non-approximated values from the
    // precomputation further below.
    double minVal = numeric_limits<double>::max();
    double maxVal = -numeric_limits<double>::max();
    if (task->rewardCPF->cachingType != "VECTOR") {
        // If the reward cannot be cached in vectors, we have not precomputed it
        // for all relevant fact combinations and approximate the domain by
        // using interval arithmetic. Otherwise, we use the min and max values
        // from the precomputation further above.

        vector<set<double>> domains(task->CPFs.size());
        for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
            domains[index] = task->CPFs[index]->domain;
        }

        for (ActionState const& action : task->actionStates) {
            double min = numeric_limits<double>::max();
            double max = -numeric_limits<double>::max();
            task->rewardCPF->formula->calculateDomainAsInterval(domains, action,
                                                                min, max);
            if (MathUtils::doubleIsSmaller(min, minVal)) {
                minVal = min;
            }
            if (MathUtils::doubleIsGreater(max, maxVal)) {
                maxVal = max;
            }
        }

        task->rewardCPF->domain.insert(minVal);
        task->rewardCPF->domain.insert(maxVal);

    } else {
        for (unsigned int i = 0; i < task->rewardCPF->precomputedResults.size();
             ++i) {
            if (MathUtils::doubleIsSmaller(
                    task->rewardCPF->precomputedResults[i], minVal)) {
                minVal = task->rewardCPF->precomputedResults[i];
            }

            if (MathUtils::doubleIsGreater(
                    task->rewardCPF->precomputedResults[i], maxVal)) {
                maxVal = task->rewardCPF->precomputedResults[i];
            }
        }
        task->rewardCPF->domain.insert(minVal);
        task->rewardCPF->domain.insert(maxVal);
    }
}
