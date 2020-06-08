#include "simplifier.h"

#include "evaluatables.h"
#include "rddl.h"

#include "utils/system_utils.h"
#include "utils/timer.h"

#include <algorithm>
#include <iostream>

using namespace std;

void Simplifier::simplify(bool generateFDRActionFluents, bool output) {
    Timer t;
    map<ParametrizedVariable*, LogicalExpression*> replacements;
    bool continueSimplification = true;
    int iteration = 0;
    while (continueSimplification) {
        ++iteration;

        //  Remove state fluents whose CPF simplifies to their initial value
        if (output) {
            cout << "    Simplify formulas (" << iteration << ")..." << endl;
        }
        simplifyFormulas(replacements);
        if (output) {
            cout << "    ...finished (" << t() << ")" << endl;
        }
        t.reset();

        // Determine if there are action fluents that are statically
        // inapplicable
        if (output) {
            cout << "    Compute inapplicable action fluents (" << iteration << ")..." << endl;
        }
        continueSimplification = computeInapplicableActionFluents(replacements);
        if (output) {
            cout << "    ...finished (" << t() << ")" << endl;
        }
        t.reset();
        if (continueSimplification) {
            continue;
        }

        // Determine which action fluents are used
        if (output) {
            cout << "    Compute relevant action fluents (" << iteration << ")..." << endl;
        }
        continueSimplification = computeRelevantActionFluents(replacements);
        if (output) {
            cout << "    ...finished (" << t() << ")" << endl;
        }
        t.reset();
        if (continueSimplification) {
            continue;
        }

        // Generate finite domain action fluents
        if (generateFDRActionFluents) {
            continueSimplification =
                determineFiniteDomainActionFluents(replacements);
            if (continueSimplification) {
                continue;
            }
        }

        // Compute actions that are potentially applicable in a reachable state
        if (output) {
            cout << "    Compute actions (" << iteration << ")..." << endl;
        }
        continueSimplification = computeActions(replacements);
        if (output) {
            cout << "    ...finished (" << t() << ")" << endl;
        }
        t.reset();
        if (continueSimplification) {
            continue;
        }

        // Approximate domains
        if (output) {
            cout << "    Approximate domains (" << iteration << ")..." << endl;
        }
        continueSimplification = approximateDomains(replacements);
        if (output) {
            cout << "    ...finished (" << t() << ")" << endl;
        }
        t.reset();
    }

    if (output) {
        cout << "    Initialize action states..." << endl;
    }
    initializeActionStates();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
}

void Simplifier::simplifyFormulas(
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

bool Simplifier::computeInapplicableActionFluents(
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

bool Simplifier::computeRelevantActionFluents(
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

bool Simplifier::actionIsApplicable(ActionState const& action) const {
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

bool Simplifier::determineFiniteDomainActionFluents(
    map<ParametrizedVariable*, LogicalExpression*>& /*replacements*/) {
    return false;
}

bool Simplifier::computeActions(
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

void Simplifier::calcAllActionStatesForIPC2018(
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

void Simplifier::calcAllActionStates(vector<ActionState>& result,
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

bool Simplifier::approximateDomains(
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

void Simplifier::initializeActionStates() {
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

bool Simplifier::sacContainsNegativeActionFluent(
    ActionPrecondition* const& sac, ActionState const& actionState) const {
    set<ActionFluent*> const& actionFluents = sac->negativeActionDependencies;

    return actionState.sharesActiveActionFluent(actionFluents);
}

bool Simplifier::sacContainsAdditionalPositiveActionFluent(
    ActionPrecondition* const& sac, ActionState const& actionState) const {
    set<ActionFluent*> const& actionFluents = sac->positiveActionDependencies;
    for (ActionFluent* af : actionFluents) {
        if (!actionState[af->index]) {
            return true;
        }
    }
    return false;
}