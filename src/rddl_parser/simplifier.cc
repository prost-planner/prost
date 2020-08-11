#include "simplifier.h"

#include "csp.h"
#include "evaluatables.h"
#include "fdr/fdr_generation.h"
#include "fdr/mutex_detection.h"
#include "rddl.h"
#include "reachability_analysis.h"

#include "utils/system_utils.h"
#include "utils/timer.h"

#include <algorithm>
#include <iostream>

using namespace std;

namespace prost::parser {
void Simplifier::simplify(bool generateFDRActionFluents, bool output) {
    utils::Timer t;
    Simplifications replacements;
    bool continueSimplification = true;
    int iteration = 0;
    while (continueSimplification) {
        ++iteration;

        if (output) {
            cout << "    Simplify formulas (" << iteration << ")..." << endl;
        }
        simplifyFormulas(replacements);
        if (output) {
            cout << "    ...finished (" << t() << ")" << endl;
        }
        t.reset();

        if (output) {
            cout << "    Compute inapplicable action fluents (" << iteration
                 << ")..." << endl;
        }
        continueSimplification = computeInapplicableActionFluents(replacements);
        if (output) {
            cout << "    ...finished (" << t() << ")" << endl;
        }
        t.reset();
        if (continueSimplification) {
            continue;
        }

        if (output) {
            cout << "    Compute relevant action fluents (" << iteration
                 << ")..." << endl;
        }
        continueSimplification = computeRelevantActionFluents(replacements);
        if (output) {
            cout << "    ...finished (" << t() << ")" << endl;
        }
        t.reset();
        if (continueSimplification) {
            continue;
        }

        if (generateFDRActionFluents) {
            if (output) {
                cout << "    Determine FDR action fluents (" << iteration
                     << ")..." << endl;
            }
            continueSimplification =
                determineFiniteDomainActionFluents(replacements);
            if (output) {
                cout << "    ...finished (" << t() << ")" << endl;
            }
            t.reset();
            if (continueSimplification) {
                continue;
            }
        }

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
    determineRelevantPreconditions();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
}

void Simplifier::simplifyFormulas(Simplifications& replacements) {
    simplifyCPFs(replacements);
    task->rewardCPF->simplify(replacements);
    simplifyPreconditions(replacements);
}

void Simplifier::simplifyCPFs(Simplifications& replacements) {
    bool continueSimplification = true;
    while (continueSimplification) {
        continueSimplification = false;
        for (auto it = task->CPFs.begin(); it != task->CPFs.end(); ++it) {
            ConditionalProbabilityFunction* cpf = *it;
            // Simplify by replacing state fluents whose CPF simplifies to their
            // initial value with their initial value
            cpf->simplify(replacements);

            // Check if this CPF now also simplifies to its initial value
            auto nc = dynamic_cast<NumericConstant*>(cpf->formula);
            double initialValue = cpf->head->initialValue;
            if (nc &&
                utils::MathUtils::doubleIsEqual(initialValue, nc->value)) {
                assert(replacements.find(cpf->head) == replacements.end());
                replacements[cpf->head] = nc;
                task->CPFs.erase(it);
                continueSimplification = true;
                break;
            }
        }
    }
    task->sortCPFs();
}

void Simplifier::simplifyPreconditions(Simplifications& replacements) {
    vector<ActionPrecondition*> sPreconds;
    for (ActionPrecondition* precond : task->preconds) {
        vector<ActionPrecondition*> sPrecond =
            simplifyPrecondition(precond, replacements);
        sPreconds.insert(sPreconds.end(), sPrecond.begin(), sPrecond.end());
    }
    task->preconds = move(sPreconds);
}

vector<ActionPrecondition*> Simplifier::simplifyPrecondition(
    ActionPrecondition* precond, Simplifications& replacements) {
    vector<ActionPrecondition*> result;
    LogicalExpression* simplified = precond->formula->simplify(replacements);
    if (auto conj = dynamic_cast<Conjunction*>(simplified)) {
        // Split the conjunction into separate preconditions
        for (LogicalExpression* expr : conj->exprs) {
            ActionPrecondition* sPrec = new ActionPrecondition(expr);
            sPrec->initialize();
            result.push_back(move(sPrec));
        }
    } else if (auto nc = dynamic_cast<NumericConstant*>(simplified)) {
        // This precond is either never satisfied or always
        if (utils::MathUtils::doubleIsEqual(nc->value, 0.0)) {
            utils::SystemUtils::abort(
                "Found a precond that is never satisified!");
        }
    } else {
        ActionPrecondition* sPrec = new ActionPrecondition(simplified);
        sPrec->initialize();
        result.push_back(move(sPrec));
    }
    return result;
}

bool Simplifier::computeInapplicableActionFluents(
    Simplifications& replacements) {
    RDDLTaskCSP csp(task);
    csp.addPreconditions();
    Z3Expressions const& actionVars = csp.getActionVarSet();
    vector<bool> fluentIsApplicable(task->actionFluents.size(), true);
    for (ActionFluent* af : task->actionFluents) {
        if (af->isFDR) {
            // TODO: We should also minimize the domains of FDR action variables
            continue;
        }

        csp.push();
        csp.addConstraint(actionVars[af->index] == 1);
        if (!csp.hasSolution()) {
            fluentIsApplicable[af->index] = false;
        }
        csp.pop();
    }
    return filterActionFluents(fluentIsApplicable, replacements);
}

bool Simplifier::computeRelevantActionFluents(Simplifications& replacements) {
    // TODO: If an action fluent is only used in preconditions, it has no direct
    //  influence on states and rewards, but could be necessary to apply other
    //  action fluents. It might be possible to compile the action fluent into
    //  other action fluents in this case. (e.g., if a1 isn't part of any CPF
    //  but a2 is, and there is an SAC a2 => a1, we can compile a1 and a2 into a
    //  new action fluent a' where a'=1 means a1=1 and a2=1, and a'=0 means
    //  a1=0 and a2=0 (or a1=1 and a2=0, this is irrelevant in this example).
    vector<bool> fluentIsUsed(task->actionFluents.size(), false);
    for (ActionPrecondition* precond : task->preconds) {
        for (ActionFluent* af : precond->dependentActionFluents) {
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
    return filterActionFluents(fluentIsUsed, replacements);
}

bool Simplifier::determineFiniteDomainActionFluents(
    Simplifications& replacements) {
    fdr::TaskMutexInfo mutexInfo = fdr::computeActionVarMutexes(task);
    if (!mutexInfo.hasMutexVarPair()) {
        return false;
    }

    fdr::FDRGenerator fdrGen(task);
    task->actionFluents = fdrGen.generateFDRVars(mutexInfo, replacements,
                                                 fdr::GreedyPartitioning());
    task->sortActionFluents();
    return true;
}

bool Simplifier::computeActions(Simplifications& replacements) {
    task->sortActionFluents();
    task->actionStates = computeApplicableActions();
    task->sortActionStates();

    // Check if there are action fluents that have the same value in every legal
    // action. We can remove those action fluents from the problem if the value
    // is 0 (allowing other values is addressed in issue 112)
    vector<set<int>> usedValues(task->actionFluents.size());
    for (ActionState const& actionState : task->actionStates) {
        vector<int> const& state = actionState.state;
        for (size_t i = 0; i < state.size(); ++i) {
            usedValues[i].insert(state[i]);
        }
    }

    bool foundUnusedActionFluent = false;
    vector<ActionFluent*> relevantActionFluents;
    for (ActionFluent* var : task->actionFluents) {
        set<int> const& usedVals = usedValues[var->index];
        assert(!usedVals.empty());
        if (usedVals.size() == 1 && (usedVals.find(0) != usedVals.end())) {
            assert(replacements.find(var) == replacements.end());
            replacements[var] = new NumericConstant(0.0);
            foundUnusedActionFluent = true;
        } else {
            int newIndex = relevantActionFluents.size();
            relevantActionFluents.push_back(var);
            var->index = newIndex;
        }
    }
    task->actionFluents = move(relevantActionFluents);

    return foundUnusedActionFluent;
}

vector<ActionState> Simplifier::computeApplicableActions() const {
    // Compute models (i.e., assignments to the state and action variables) for
    // the planning task and use the assigned action variables as legal action
    // state (being part of the model means there is at least one state where
    // the assignment is legal) and invalidate the action assignment for the
    // next iteration. Terminates when there are no more models.
    vector<ActionState> result;
    RDDLTaskCSP csp(task);
    csp.addPreconditions();
    while (csp.hasSolution()) {
        result.emplace_back(csp.getActionModel());
        csp.invalidateActionModel();
    }
    return result;
}

bool Simplifier::approximateDomains(Simplifications& replacements) {
    MinkowskiReachabilityAnalyser r(task);
    vector<set<double>> domains = r.determineReachableFacts();
    return removeConstantStateFluents(domains, replacements);
}

bool Simplifier::removeConstantStateFluents(vector<set<double>> const& domains,
                                            Simplifications& replacements) {
    vector<ConditionalProbabilityFunction*> cpfs;
    bool sfRemoved = false;
    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        set<double> const& domain = domains[cpf->head->index];
        assert(!domain.empty());
        if (domain.size() == 1) {
            assert(replacements.find(cpf->head) == replacements.end());
            replacements[cpf->head] = new NumericConstant(*domain.begin());
            sfRemoved = true;
        } else {
            // We currently do not support removal of values, so we add all
            // values up to maxVal to the domain (see issue 115)
            cpf->setDomainSize(static_cast<int>(*domain.rbegin() + 1));
            cpfs.push_back(cpf);
        }
    }
    task->CPFs = move(cpfs);
    task->sortCPFs();
    return sfRemoved;
}

void Simplifier::determineRelevantPreconditions() {
    // Remove static preconditions
    auto keep = [](ActionPrecondition* precond) {
        return precond->containsStateFluent();
    };
    auto partIt =
        stable_partition(task->preconds.begin(), task->preconds.end(), keep);
    task->preconds.erase(partIt, task->preconds.end());
    for (size_t i = 0; i < task->preconds.size(); ++i) {
        task->preconds[i]->index = i;
    }

    // Determine relevant preconditions for each action
    RDDLTaskCSP csp(task);
    int numActions = task->actionStates.size();
    int numPreconds = task->preconds.size();
    vector<bool> precondIsRelevant(numPreconds, false);
    for (size_t i = 0; i < numActions; ++i) {
        ActionState& action = task->actionStates[i];
        action.index = i;

        csp.push();
        csp.assignActionVarSet(action.state);

        for (ActionPrecondition* precond : task->preconds) {
            csp.push();
            csp.addConstraint(precond->formula->toZ3Formula(csp, 0) == 0);
            if (csp.hasSolution()) {
                action.relevantSACs.push_back(precond);
                precondIsRelevant[precond->index] = true;
            }
            csp.pop();
        }
        csp.pop();
    }

    // Remove preconditions that aren't relevant for at least one action
    vector<ActionPrecondition*> finalPreconds;
    for (ActionPrecondition* precond : task->preconds) {
        if (precondIsRelevant[precond->index]) {
            precond->index = finalPreconds.size();
            finalPreconds.push_back(precond);
        }
    }
    task->preconds = move(finalPreconds);
}

bool Simplifier::filterActionFluents(vector<bool> const& filter,
                                     Simplifications& replacements) {
    vector<ActionFluent*>& actionFluents = task->actionFluents;
    auto keep = [&](ActionFluent* af) { return filter[af->index]; };
    // Separate action fluents that are kept from discarded ones
    auto partIt = partition(actionFluents.begin(), actionFluents.end(), keep);
    bool result = (partIt != actionFluents.end());
    // Add discarded action fluents to replacements
    for (auto it = partIt; it != actionFluents.end(); ++it) {
        assert(replacements.find(*it) == replacements.end());
        replacements[*it] = new NumericConstant(0.0);
    }
    // Erase discarded action fluents
    task->actionFluents.erase(partIt, task->actionFluents.end());
    // Sort action fluents that are kept and assign indices
    task->sortActionFluents();
    return result;
}
} // namespace prost::parser
