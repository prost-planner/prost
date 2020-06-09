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
            if (output) {
                cout << "    Compute FDR action fluents (" << iteration << ")..." << endl;
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

// Sort action fluents for deterministic behaviour and assign indices
inline void Simplifier::sortActionFluents() {
    int numActionFluents = task->actionFluents.size();

    // Sort action fluents for deterministic behaviour and assign indices
    sort(task->actionFluents.begin(), task->actionFluents.end(),
         [](ActionFluent* const& lhs, ActionFluent* const& rhs) {
             return lhs->fullName < rhs->fullName;
         });
    for (int index = 0; index < numActionFluents; ++index) {
        task->actionFluents[index]->index = index;
    }
}

bool Simplifier::determineFiniteDomainActionFluents(
    map<ParametrizedVariable*, LogicalExpression*>& replacements) {

    if (task->numberOfConcurrentActions > 1 && task->actionPreconds.empty()) {
        // If there are no action preconditions and max-nondef-actions does not
        // limit the number of concurrent actions to 1, it is impossible that
        // there are mutex action fluents.
        return false;
    }

    if (task->numberOfConcurrentActions == 1 && task->actionFluents.size() > 1) {
        // All action fluents are mutually exclusive, so we can combine them to
        // a single FDR action fluent

        Type* valueType =
            new Type("finite-domain-action-fluent-0-type", nullptr);
        ActionFluent* finiteDomainAF =
            new ActionFluent("finite-domain-action-fluent-0", valueType);
        finiteDomainAF->index = 0;

        //TODO: Check if noop is really applicable
        int index = 0;
        valueType->objects.push_back(
            new Object("none-of-those", valueType, index));

        for (ActionFluent* af : task->actionFluents) {
            ++index;
            // Remove spaces from action name
            string name = af->fullName;
            replace(name.begin(), name.end(), ' ', '~');

            // Generate object that represents the new value
            valueType->objects.push_back(
                new Object(name, valueType, index));

            // Replace all occurences of (old) binary action fluent with (new)
            // FDR action fluent
            vector<LogicalExpression*> eq = {finiteDomainAF, new NumericConstant(index)};
            replacements[af] = new EqualsExpression(eq);
        }
        task->actionFluents = {finiteDomainAF};
        return true;
    }

    return false;
    // cout << "determining finite domain action fluents with "
    //      << task->actionFluents.size() << " action fluents" << endl;
    //
    // z3::context c;
    // z3::solver s(c);
    // vector<z3::expr> sf_exprs;
    // vector<z3::expr> af_exprs;
    // buildCSP(c, s, sf_exprs, af_exprs);
    //
    // vector<set<int>> mutexes(task->actionFluents.size());
    //
    // for (size_t i = 0; i < task->actionFluents.size(); ++i) {
    //     if (!task->actionFluents[i]->isBinary()) {
    //         // We do not alter action fluents that are already finite domain
    //         continue;
    //     }
    //     for (size_t j = i + 1; j < task->actionFluents.size(); ++j) {
    //         if (!task->actionFluents[j]->isBinary()) {
    //             // We do not alter action fluents that are already finite domain
    //             continue;
    //         }
    //         s.push();
    //         s.add((af_exprs[i] == 1) && (af_exprs[j] == 1));
    //         if (s.check() != z3::sat) {
    //             cout << "fluent " << task->actionFluents[i]->fullName
    //                  << " and fluent " << task->actionFluents[j]->fullName
    //                  << " are mutually exclusive!" << endl;
    //             mutexes[i].insert(j);
    //             mutexes[j].insert(i);
    //         }
    //         s.pop();
    //     }
    // }
    //
    // vector<set<int>> mutexGroups;
    // // TODO: The following implementation works but is very adhoc. A version
    // // that generates all maximal mutex groups would be cleaner.
    // bool hasMore = true;
    // while (hasMore) {
    //     hasMore = false;
    //     set<int> mutexGroup;
    //     for (size_t i = 0; i < task->actionFluents.size(); ++i) {
    //         if (extendsMutexGroup(i, mutexGroup, mutexes)) {
    //             for (int af : mutexGroup) {
    //                 mutexes[af].erase(i);
    //                 mutexes[i].erase(af);
    //             }
    //             mutexGroup.insert(i);
    //         }
    //     }
    //     if (!mutexGroup.empty()) {
    //         mutexGroups.push_back(mutexGroup);
    //     }
    //     for (size_t i = 0; i < mutexes.size(); ++i) {
    //         if (!mutexes[i].empty()) {
    //             hasMore = true;
    //             break;
    //         }
    //     }
    // }
    //
    // if (mutexGroups.empty()) {
    //     cout << "num action fluents: " << task->actionFluents.size() << endl;
    //     return false;
    // }
    //
    // cout << "Determined the following mutex groups:" << endl;
    // for (set<int> const& mutexGroup : mutexGroups) {
    //     for (int id : mutexGroup) {
    //         cout << task->actionFluents[id]->fullName << " ";
    //     }
    //     cout << endl;
    // }
    //
    // // A minimum hitting set over the cliques is (presumably) the best possible
    // // way to derive finite-domain vars. Due to the NP-completeness of the
    // // underlying poblem, we approximate a minimum hitting set by iteratively
    // // selecting the largest mutex group and removing all elements from the
    // // selected mutex group from all other mutex groups until all action fluents
    // // are hit
    //
    // vector<bool> fluentIsHit(task->actionFluents.size(), false);
    // vector<set<int>> selectedMutexGroups;
    // while (!mutexGroups.empty()) {
    //     // sort mutex groups by their size
    //     std::sort(mutexGroups.begin(), mutexGroups.end(),
    //               [](set<int> const& lhs, set<int> const& rhs) {
    //                   return lhs.size() < rhs.size();
    //               });
    //
    //     // pick the largest mutex group and erase all of its elements from all
    //     // other mutex groups
    //     set<int> selectedMutexGroup = mutexGroups.back();
    //     mutexGroups.pop_back();
    //     for (int id : selectedMutexGroup) {
    //         assert(!fluentIsHit[id]);
    //         fluentIsHit[id] = true;
    //         for (set<int>& mutexGroup : mutexGroups) {
    //             mutexGroup.erase(id);
    //         }
    //     }
    //     selectedMutexGroups.push_back(selectedMutexGroup);
    //
    //     // remove all mutex groups with zero or one elements
    //     vector<set<int>> tmp;
    //     for (set<int> mutexGroup : mutexGroups) {
    //         if (mutexGroup.size() > 1) {
    //             tmp.push_back(mutexGroup);
    //         }
    //     }
    //     swap(mutexGroups, tmp);
    // }
    //
    // vector<ActionFluent*> finiteDomainActionFluents;
    // for (set<int> const& selectedMutexGroup : selectedMutexGroups) {
    //     stringstream ss;
    //     ss << "finite-domain-action-fluent-"
    //        << finiteDomainActionFluents.size();
    //     string afName = ss.str();
    //     ss << "-type" << endl;
    //     string afTypeName = ss.str();
    //     ss.str("");
    //     Type* valueType = new Type(afTypeName, nullptr);
    //
    //     int domainSize = selectedMutexGroup.size();
    //     int index = 0;
    //
    //     s.push();
    //     // Check if at least one of these action fluents must be selected
    //     for (int id : selectedMutexGroup) {
    //         s.add(af_exprs[id] == 0);
    //     }
    //     // It's possible to select neither of these action fluents, so we add
    //     // a "none-of-those" value as first domain value
    //     if (s.check() == z3::sat) {
    //         valueType->objects.push_back(
    //             new Object("none-of-those", valueType, index++));
    //         ++domainSize;
    //     }
    //     s.pop();
    //
    //     for (int id : selectedMutexGroup) {
    //         // Remove spaces from action name
    //         string name = task->actionFluents[id]->fullName;
    //         std::replace(name.begin(), name.end(), ' ', '~');
    //         std::cout << name << std::endl;
    //         valueType->objects.push_back(new Object(name, valueType, index++));
    //     }
    //
    //     ActionFluent* fdaf = new ActionFluent(afName, valueType, domainSize);
    //     finiteDomainActionFluents.push_back(fdaf);
    //
    //     // Replace binary action fluent with new action fluent / value
    //     // combination. Adapt index in case none-of-those is possible
    //     index = (domainSize == selectedMutexGroup.size()) ? 0 : 1;
    //     for (int id : selectedMutexGroup) {
    //         vector<LogicalExpression*> eq = {fdaf, new NumericConstant(index)};
    //         replacements[task->actionFluents[id]] = new EqualsExpression(eq);
    //         ++index;
    //     }
    // }
    //
    // // keep all action fluents that are not part of a mutex group
    // for (size_t i = 0; i < task->actionFluents.size(); ++i) {
    //     if (!fluentIsHit[i]) {
    //         finiteDomainActionFluents.push_back(task->actionFluents[i]);
    //     }
    // }
    //
    // std::sort(finiteDomainActionFluents.begin(),
    //           finiteDomainActionFluents.end(),
    //           [](ActionFluent* const& lhs, ActionFluent* const& rhs) {
    //               return lhs->fullName > rhs->fullName;
    //           });
    //
    // for (int index = 0; index < finiteDomainActionFluents.size(); ++index) {
    //     finiteDomainActionFluents[index]->index = index;
    // }
    //
    // // replace old action fluents with new ones
    // task->actionFluents = finiteDomainActionFluents;
    // cout << "num action fluents: " << task->actionFluents.size() << endl;
    return true;
}

bool Simplifier::computeActions(
    map<ParametrizedVariable*, LogicalExpression*>& replacements) {
    sortActionFluents();

    int numActionFluents = task->actionFluents.size();

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
        // Generate all action states with exactly one action fluent with value
        // different from 0
        for (ActionFluent* af : task->actionFluents) {
            int numValues = af->valueType->objects.size();
            int afIndex = af->index;
            for (int val = 1; val < numValues; ++val) {
                ActionState state(numActionFluents);
                state[afIndex] = val;
                result.insert(state);
            }
        }
        return;
    }

    for (const ActionState& baseAction : base) {
        for (ActionFluent* af : task->actionFluents) {
            if (!baseAction[af->index]) {
                int numValues = af->valueType->objects.size();
                for (int val = 1; val < numValues; ++val) {
                    result.emplace(baseAction, af->index, val);
                }
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
                        int numValues =
                            task->actionFluents[j]->valueType->objects.size();
                        for (int val = 1; val < numValues; ++val) {
                            result.emplace_back(result[i], j, val);
                        }
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