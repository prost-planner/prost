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
    // Simplify task
    if (output) {
        cout << "    Simplify task..." << endl;
    }
    simplifyTask();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
    t.reset();

    // Determinize CPFs
    if (output) {
        cout << "    Computing determinization..." << endl;
    }
    determinize();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
    t.reset();

    // Determine some non-trivial properties
    if (output) {
        cout << "    Determining task properties..." << endl;
    }
    determineTaskProperties();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
    t.reset();

    // Initialize Hash Key Bases and Mappings
    if (output) {
        cout << "    Preparing hash keys..." << endl;
    }
    prepareStateHashKeys();
    prepareKleeneStateHashKeys();
    prepareStateFluentHashKeys();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
    t.reset();

    // Precompute results of evaluate for (some) evaluatables
    if (output) {
        cout << "    Precomputing evaluatables..." << endl;
    }
    precomputeEvaluatables();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
    t.reset();

    // Approximate or calculate the min and max reward
    if (output) {
        cout << "    Calculating min and max reward..." << endl;
    }
    calculateMinAndMaxReward();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
}

void Preprocessor::simplifyTask() {
    map<ParametrizedVariable*, LogicalExpression*> replacements;
    bool continueSimplification = true;
    while (continueSimplification) {
        cout << "Next iteration: simplify formulas" << endl;
        //  Remove state fluents whose CPF simplifies to their initial value
        simplifyFormulas(replacements);

        // Determine if there are action fluents that are statically
        // inapplicable
        cout << "compute inapplicable action fluents" << endl;
        continueSimplification = computeInapplicableActionFluents(replacements);
        if (continueSimplification) {
            continue;
        }

        // Determine which action fluents are used
        cout << "compute relevant action fluents" << endl;
        continueSimplification = computeRelevantActionFluents(replacements);
        if (continueSimplification) {
            continue;
        }

        // Generate finite domain action fluents
        // if (useFDRActionFluents) {
        //     continueSimplification =
        //         determineFiniteDomainActionFluents(replacements);
        //     if (continueSimplification) {
        //         continue;
        //     }
        // }

        // Compute actions that are potentially applicable in a reachable state
        cout << "compute actions" << endl;
        continueSimplification = computeActions(replacements);
        if (continueSimplification) {
            continue;
        }

        // Approximate domains
        cout << "approximate domains" << endl;
        continueSimplification = approximateDomains(replacements);
    }

    initializeActionStates();
}

void Preprocessor::simplifyFormulas(
    map<ParametrizedVariable*, LogicalExpression*>& replacements) {
    bool continueSimplification = true;
    while (continueSimplification) {
        continueSimplification = false;

        //  Remove state fluents whose CPF simplifies to their initial value
        for (ConditionalProbabilityFunction*& cpf : task->CPFs) {
            cpf->simplify(replacements);
            NumericConstant* nc = dynamic_cast<NumericConstant*>(cpf->formula);
            if (nc &&
                MathUtils::doubleIsEqual(cpf->head->initialValue, nc->value)) {
                continueSimplification = true;
                assert(replacements.find(cpf->head) == replacements.end());
                replacements[cpf->head] = nc;
                swap(cpf, task->CPFs[task->CPFs.size() - 1]);
                task->CPFs.pop_back();
                break;
            }
        }
    }

    // Sort CPFs for deterministic behaviour
    sort(task->CPFs.begin(), task->CPFs.end(),
         ConditionalProbabilityFunction::TransitionFunctionSort());

    // Update indices of state fluents (via their corresponding CPF)
    for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
        task->CPFs[index]->setIndex(index);
    }

    // Simplify reward
    task->rewardCPF->simplify(replacements);

    vector<LogicalExpression*> newPreconds;
    for (LogicalExpression* oldPrecond : task->SACs) {
        // Simplify this precondition
        LogicalExpression* precond = oldPrecond->simplify(replacements);

        // Split this precondition into conjuncts if possible
        Conjunction* conj = dynamic_cast<Conjunction*>(precond);
        vector<LogicalExpression*> conjuncts;
        if (conj) {
            conjuncts.insert(
                conjuncts.end(), conj->exprs.begin(), conj->exprs.end());
        } else {
            conjuncts.push_back(precond);
        }

        // Keep only preconditions that do not evaluate to the constant "true"
        for (LogicalExpression* conjunct : conjuncts) {
            NumericConstant* nc = dynamic_cast<NumericConstant*>(conjunct);
            if (nc) {
                if (MathUtils::doubleIsEqual(nc->value, 0.0)) {
                    SystemUtils::abort(
                        "Found a precond that evaluates to the constant "
                        "\"false\"!");
                } else {
                    // When converting from a number to a bool, we treat each number
                    // except for 0.0 as "true", so this evaluates to "true" and we
                    // ignore the precondition
                    continue;
                }
            } else {
                newPreconds.push_back(conjunct);
            }
        }
    }
    swap(task->SACs, newPreconds);
}

bool Preprocessor::computeInapplicableActionFluents(
    map<ParametrizedVariable*, LogicalExpression*>& replacements) {
    // Divide preconds into dynamic SACs (i.e., proper action preconditions),
    // static SACs, primitive static SACs and state invariants and check if
    // there are action fluents that are statically inapplicable (i.e.,
    // forbidden by a primitive static SAC)
    task->actionPreconds.clear();
    task->staticSACs.clear();

    vector<bool> fluentIsInapplicable(task->actionFluents.size(), false);
    vector<LogicalExpression*> preconds;
    for (size_t index = 0; index < task->SACs.size(); ++index) {
        LogicalExpression* sacLogExpr = task->SACs[index];
        ActionPrecondition* sac =
            new ActionPrecondition("SAC " + to_string(index), sacLogExpr);

        // Collect the properties of the SAC that are necessary for distinction
        // of action preconditions, (primitive) static SACs and state invariants
        sac->initialize();
        if (sac->containsStateFluent()) {
            // An SAC that contain both state and action fluents is an action
            // precondition, and an SAC that contains only state fluents is a
            // state invariant that is ignored in PROST.
            if (!sac->isActionIndependent()) {
                sac->index = task->actionPreconds.size();
                task->actionPreconds.push_back(sac);
                preconds.push_back(sacLogExpr);
            }
        } else {
            Negation* neg = dynamic_cast<Negation*>(sac->formula);
            if (neg) {
                ActionFluent* af = dynamic_cast<ActionFluent*>(neg->expr);
                if (af) {
                    // This is a "primitive" static action constraint of the
                    // form "not doSomething(params)", i.e., an sac that forbids
                    // the usage of an action fluent in general. No matter if
                    // this action fluent is used in any formula, it can be
                    // converted to the constant "false".
                    fluentIsInapplicable[af->index] = true;
                } else {
                    task->staticSACs.push_back(sac);
                    preconds.push_back(sacLogExpr);
                }
            } else {
                // An SAC that only contains action fluents is used to
                // statically forbid action combinations.
                task->staticSACs.push_back(sac);
                preconds.push_back(sacLogExpr);
            }
        }
    }
    swap(task->SACs, preconds);

    vector<ActionFluent*> actionFluents;
    int nextIndex = 0;
    bool foundInapplicabledActionFluent = false;
    for (ActionFluent* af : task->actionFluents) {
        if (fluentIsInapplicable[af->index]) {
            // cout << "action fluent " << af->fullName << " is inapplicable" << endl;
            assert(replacements.find(af) == replacements.end());
            replacements[af] = new NumericConstant(0.0);
            foundInapplicabledActionFluent = true;
        } else {
            af->index = nextIndex;
            ++nextIndex;

            actionFluents.push_back(af);
        }
    }
    swap(task->actionFluents, actionFluents);
    return foundInapplicabledActionFluent;
}

bool Preprocessor::computeRelevantActionFluents(
    map<ParametrizedVariable*, LogicalExpression*>& replacements) {
    // Check if there are action fluents that aren't used in any CPF or SAC
    // TODO: If an action fluent is only used in SACs, it has no direct
    //  influence on states and rewards, but could be necessary to apply other
    //  action fluents. It might be possible to compile the action fluent into
    //  other action fluents in this case. (e.g., if a1 isn't part of any CPF
    //  but a2 is, and there is a CPF a2 => a1, we can compile a1 and a2 into a
    //  new action fluent a' where a'=1 means a1=1 and a2=1, and a'=0 means
    //  a1=0 and a2=0 (or a1=1 and a2=0, this is irrelevant in this example).
    vector<bool> fluentIsUsed(task->actionFluents.size(), false);
    for (ActionPrecondition* precond : task->actionPreconds) {
        for (ActionFluent* af : precond->dependentActionFluents) {
            fluentIsUsed[af->index] = true;
        }
    }

    for (ActionPrecondition* sac : task->staticSACs) {
        for (ActionFluent* af : sac->dependentActionFluents) {
            fluentIsUsed[af->index] = true;
        }
    }

    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        for (ActionFluent* af : cpf->dependentActionFluents) {
            fluentIsUsed[af->index] = true;
        }
    }

    for (ActionFluent* af : task->rewardCPF->dependentActionFluents) {
        fluentIsUsed[af->index] = true;
    }

    vector<ActionFluent*> actionFluents;
    int nextIndex = 0;
    bool foundUnusedActionFluent = false;
    for (ActionFluent* af : task->actionFluents) {
        if (fluentIsUsed[af->index]) {
            af->index = nextIndex;
            ++nextIndex;

            actionFluents.push_back(af);
        } else {
            // cout << "action fluent " << af->fullName << " is unused" << endl;
            assert(replacements.find(af) == replacements.end());
            replacements[af] = new NumericConstant(0.0);
            foundUnusedActionFluent = true;
        }
    }
    swap(task->actionFluents, actionFluents);
    return foundUnusedActionFluent;
}

bool Preprocessor::actionIsApplicable(ActionState const& action) const {
    // The state is irrelevant as only staticSACs are checked
    State dummy(task->CPFs.size());
    for (ActionPrecondition* sac : task->staticSACs) {
        double res = 0.0;
        sac->formula->evaluate(res, dummy, action);
        if (MathUtils::doubleIsEqual(res, 0.0)) {
            return false;
        }
    }
    return true;
}

bool Preprocessor::computeActions(
    map<ParametrizedVariable*, LogicalExpression*>& replacements) {
    int numActionFluents = task->actionFluents.size();

    // Sort action fluents for deterministic behaviour and assign indices
    sort(task->actionFluents.begin(), task->actionFluents.end(),
         [](ActionFluent* const& lhs, ActionFluent* const& rhs) {
             return lhs->fullName < rhs->fullName;
         });
    for (int index = 0; index < numActionFluents; ++index) {
        task->actionFluents[index]->index = index;
    }

    State current(task->CPFs);
    vector<ActionState> legalActionStates;
    if (useIPC2018Rules) {
        // For IPC 2018, the rules say that an action cannot be legal unless
        // there is at least one legal action where the same action fluents are
        // "active" except for one action fluent. Actions with exactly one
        // "active" action fluent are an exception, these can be legal even if
        // noop isn't.
        ActionState noop(numActionFluents);
        if (actionIsApplicable(noop)) {
            legalActionStates.push_back(noop);
        }
        vector<ActionState> base;
        task->numberOfConcurrentActions = 1;
        while (true) {
            set<ActionState> candidates;
            calcAllActionStatesForIPC2018(base, candidates);
            vector<ActionState> addedActionStates;
            bool foundApplicableAction = false;
            for (ActionState const& actionState : candidates) {
                if (actionIsApplicable(actionState)) {
                    foundApplicableAction = true;
                    legalActionStates.push_back(actionState);
                    addedActionStates.push_back(actionState);
                }
            }
            if (!foundApplicableAction ||
                (task->numberOfConcurrentActions == numActionFluents)) {
                break;
            }
            ++task->numberOfConcurrentActions;
            base = addedActionStates;
        }
    } else {
        task->numberOfConcurrentActions =
            min(task->numberOfConcurrentActions, numActionFluents);

        // Calculate all action states with up to
        // numberOfConcurrentActions concurrent actions
        vector<ActionState> actionStateCandidates;
        calcAllActionStates(actionStateCandidates, 0, 0);

        // Remove all illegal action combinations by checking the SACs
        // that are state independent
        for (ActionState const& actionState : actionStateCandidates) {
            if (actionIsApplicable(actionState)) {
                legalActionStates.push_back(actionState);
            }
        }
    }
    swap(task->actionStates, legalActionStates);

    // Now that all legal actions have been created, we check if there are
    // action fluents that are false (TODO: that are equal to their default
    // value) in every legal action. We can safely remove those action fluents
    // entirely from each action.
    ActionState fluentIsUsed((int)task->actionFluents.size());
    for (ActionState const& actionState : task->actionStates) {
        for (size_t i = 0; i < actionState.state.size(); ++i) {
            fluentIsUsed[i] |= actionState.state[i];
        }
    }

    vector<ActionFluent*> actionFluents;
    int nextIndex = 0;
    bool foundUnusedActionFluent = false;
    for (ActionFluent* af : task->actionFluents) {
        if (fluentIsUsed[af->index]) {
            af->index = nextIndex;
            ++nextIndex;

            actionFluents.push_back(af);
        } else {
            // cout << "action fluent " << af->fullName
            //      << " is 0 in every applicable action" << endl;
            assert(replacements.find(af) == replacements.end());
            replacements[af] = new NumericConstant(0.0);
            foundUnusedActionFluent = true;
        }
    }
    swap(task->actionFluents, actionFluents);
    return foundUnusedActionFluent;
}

void Preprocessor::calcAllActionStatesForIPC2018(
    vector<ActionState>& base, set<ActionState>& result) const {
    int numActionFluents = task->actionFluents.size();
    if (base.empty()) {
        // Generate all action states with exactly one active action fluent
        for (int index = 0; index < numActionFluents; ++index) {
            ActionState state(numActionFluents);
            state[index] = 1;
            result.insert(state);
        }
        return;
    }

    for (const ActionState& baseAction : base) {
        for (unsigned int index = 0; index < numActionFluents; ++index) {
            if (!baseAction[index]) {
                result.emplace(baseAction, index);
            }
        }
    }
}

void Preprocessor::calcAllActionStates(vector<ActionState>& result,
                                       int minElement,
                                       int scheduledActions) const {
    int numActionFluents = task->actionFluents.size();
    if (result.empty()) {
        result.emplace_back(numActionFluents);
        //result.push_back(ActionState(numActionFluents));
    } else {
        int lastIndex = result.size();

        for (int i = minElement; i < lastIndex; ++i) {
            for (size_t j = 0; j < numActionFluents; ++j) {
                if (!result[i][j]) {
                    bool isExtension = true;
                    for (unsigned int k = 0; k < j; ++k) {
                        if (result[i][k]) {
                            isExtension = false;
                            break;
                        }
                    }

                    if (isExtension) {
                        result.emplace_back(result[i], j);
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

bool Preprocessor::approximateDomains(
    map<ParametrizedVariable*, LogicalExpression*>& replacements) {
    int numCPFs = task->CPFs.size();
    // Insert initial values to set of reachable values
    vector<set<double>> domains(numCPFs);
    for (size_t index = 0; index < numCPFs; ++index) {
        domains[index].insert(task->CPFs[index]->getInitialValue());
    }

    // Simulate a run in the planning task but evaluate to all possible
    // outcomes. Terminate if the fixpoint iteration doesn't change anything
    // anymore
    int currentHorizon = 0;
    while (currentHorizon < task->horizon) {
        // The values that are reachable in this step
        vector<set<double>> reachable(numCPFs);

        // Check each CPF if additional values can be derived
        for (size_t varIndex = 0; varIndex < numCPFs; ++varIndex) {
            ConditionalProbabilityFunction* cpf = task->CPFs[varIndex];
            if (domains[varIndex].size() == cpf->getDomainSize()) {
                reachable[varIndex] = cpf->domain;
                continue;
            }

            if (cpf->isActionIndependent()) {
                // If the CPF is action independent, it it sufficient to apply
                // a dummy action
                ActionState dummy(task->actionFluents.size());
                set<double> actionDependentValues;
                cpf->formula->calculateDomain(
                    domains, dummy, reachable[varIndex]);
                assert(!reachable[varIndex].empty());
            } else {
                // If the CPF is action dependent, we have to check all actions.
                // However, it is enough to check only one of all the actions
                // without an active action fluent that occurs in the CPF.
                // TODO: In fact, this could be generalized to arbitrary subsets
                //  of the action fluents that occur in the CPF.
                set<ActionFluent*>& dependentActionFluents =
                    cpf->dependentActionFluents;
                bool independentChecked = false;
                for (ActionState const& action : task->actionStates) {
                    bool sharesActiveActionFluent =
                        action.sharesActiveActionFluent(dependentActionFluents);
                    if (!independentChecked || sharesActiveActionFluent) {
                        set<double> actionDependentValues;
                        cpf->formula->calculateDomain(
                            domains, action, actionDependentValues);
                        assert(!actionDependentValues.empty());
                        reachable[varIndex].insert(actionDependentValues.begin(),
                                                   actionDependentValues.end());
                        independentChecked |= !sharesActiveActionFluent;
                    }
                }
            }
        }

        // All possible value of this step have been computed. Check if there
        // are additional values, if so insert them and continue, otherwise the
        // fixpoint iteration has reached its end
        bool someDomainChanged = false;
        for (size_t varIndex = 0; varIndex < domains.size(); ++varIndex) {
            set<double>& domain = domains[varIndex];
            set<double> const& reached = reachable[varIndex];
            int sizeBefore = domain.size();
            domain.insert(reached.begin(), reached.end());
            if (domain.size() != sizeBefore) {
                someDomainChanged = true;
            }
        }

        if (!someDomainChanged) {
            break;
        }

        ++currentHorizon;
    }

    // TODO: In the for loop below, we fill all missing values up to the
    //  maximal value that is relevant to avoid the necessity to change
    //  values in CPFs or other logical expressions (e.g., if we have a state
    //  fluent s with domain {0,1,2,3}, and only {0,2} are relevant, we also
    //  add the value "1" to the domain because we'd need to relable every
    //  occurence of s == 2 in Logical Expressions to s == 1 otherwise (domains
    //  must be the first n integers)
    for (size_t index = 0; index < numCPFs; ++index) {
        set<double>& domain = domains[index];
        double maxVal = *domain.rbegin();
        int domainSize = domain.size();
        if (domainSize > 1 && (maxVal != domainSize - 1)) {
            // cout << "State-fluent " << task->CPFs[index]->head->fullName
            //      << " has a domain size of " << domains[index].size()
            //      << " and a max val of " << maxVal << endl;
            // cout << "Inserting values into domain of state-fluent "
            //      << task->CPFs[index]->head->fullName << endl;
            for (size_t val = 0; val < maxVal; ++val) {
                domain.insert(val);
            }
        }
    }

    // Set domains
    bool foundConstantCPF = false;
    for (size_t index = 0; index < numCPFs; ++index) {
        if (domains[index].size() == 1) {
            foundConstantCPF = true;

            assert(replacements.find(task->CPFs[index]->head) ==
                   replacements.end());
            replacements[task->CPFs[index]->head] =
                new NumericConstant(*domains[index].begin());
            swap(task->CPFs[index], task->CPFs[numCPFs - 1]);
            task->CPFs.pop_back();
            swap(domains[index], domains[domains.size() - 1]);
            domains.pop_back();
            --index;
            --numCPFs;
        } else {
            task->CPFs[index]->setDomain(domains[index]);
        }
    }

    if (foundConstantCPF) {
        // sort CPFs for deterministic behaviour
        sort(task->CPFs.begin(), task->CPFs.end(),
             ConditionalProbabilityFunction::TransitionFunctionSort());

        // update indices of state fluents (via their corresponding CPF)
        for (unsigned int index = 0; index < numCPFs; ++index) {
            task->CPFs[index]->setIndex(index);
        }
    }
    return foundConstantCPF;
}

void Preprocessor::initializeActionStates() {
    // Sort action states again for deterministic behaviour
    sort(task->actionStates.begin(), task->actionStates.end(),
         ActionState::ActionStateSort());

    // Set inidices and calculate properties of action states
    for (size_t index = 0; index < task->actionStates.size(); ++index) {
        ActionState& actionState = task->actionStates[index];
        actionState.index = index;

        for (size_t i = 0; i < actionState.state.size(); ++i) {
            if (actionState.state[i]) {
                actionState.scheduledActionFluents.push_back(
                    task->actionFluents[i]);
            }
        }

        // TODO: I am very uncertain if the following is correct.
        // Determine which dynamic SACs are relevant for this action
        for (ActionPrecondition* precond : task->actionPreconds) {
            if (precond->containsArithmeticFunction()) {
                // If the precondition contains an arithmetic function we treat
                // it as if it influenced this action.
                // TODO: Implement a function that actually checks this
                actionState.relevantSACs.push_back(precond);
            } else if (sacContainsNegativeActionFluent(precond, actionState)) {
                // If the precondition contains one of this ActionStates' action
                // fluents negatively it might forbid this action.
                actionState.relevantSACs.push_back(precond);
            } else if (sacContainsAdditionalPositiveActionFluent(
                precond, actionState)) {
                // If the precondition contains action fluents positively that
                // are not in this ActionStates' action fluents it might enforce
                // that action fluent (and thereby forbid this action)
                actionState.relevantSACs.push_back(precond);
            }
        }
    }
}

bool Preprocessor::sacContainsNegativeActionFluent(
    ActionPrecondition* const& sac, ActionState const& actionState) const {
    set<ActionFluent*> const& actionFluents = sac->negativeActionDependencies;

    return actionState.sharesActiveActionFluent(actionFluents);
}

bool Preprocessor::sacContainsAdditionalPositiveActionFluent(
    ActionPrecondition* const& sac, ActionState const& actionState) const {
    set<ActionFluent*> const& actionFluents = sac->positiveActionDependencies;
    for (ActionFluent* af : actionFluents) {
        if (!actionState[af->index]) {
            return true;
        }
    }
    return false;
}



void Preprocessor::determinize() {
    // Calculate determinzation of CPFs.
    Simplifications replacementsDummy;
    for (unsigned int index = 0; index < task->CPFs.size(); ++index) {
        if (task->CPFs[index]->isProbabilistic()) {
            task->CPFs[index]->determinization =
                task->CPFs[index]->formula->determinizeMostLikely(
                    task->actionStates);
            task->CPFs[index]->determinization =
                task->CPFs[index]->determinization->simplify(replacementsDummy);
        }
    }
}

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
