#include "simplifier.h"

#include "evaluatables.h"
#include "rddl.h"

#include "utils/system_utils.h"
#include "utils/timer.h"

#include "z3++.h"

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
            cout << "    Compute inapplicable action fluents (" << iteration
                 << ")..." << endl;
        }
        continueSimplification =
            computeInapplicableActionFluents(replacements);
        if (output) {
            cout << "    ...finished (" << t() << ")" << endl;
        }
        t.reset();
        if (continueSimplification) {
            continue;
        }

        // Determine which action fluents are used
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

        // Generate finite domain action fluents
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
    vector<LogicalExpression*> SACs;
    for (size_t index = 0; index < task->SACs.size(); ++index) {
        LogicalExpression* sac = task->SACs[index];
        ActionPrecondition* precond =
            new ActionPrecondition("SAC " + to_string(index), sac);

        // Collect the properties of the SAC that are necessary for distinction
        // of action preconditions, (primitive) static SACs and state invariants
        precond->initialize();
        if (precond->containsStateFluent()) {
            // An SAC that contain both state and action fluents is an action
            // precondition, and an SAC that contains only state fluents is a
            // state invariant that is ignored in PROST.
            if (!precond->isActionIndependent()) {
                precond->index = task->actionPreconds.size();
                task->actionPreconds.push_back(precond);
                SACs.push_back(sac);
            }
        } else {
            Negation* neg = dynamic_cast<Negation*>(precond->formula);
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
                    task->staticSACs.push_back(precond);
                    SACs.push_back(sac);
                }
            } else {
                // An SAC that only contains action fluents is used to
                // statically forbid action combinations.
                task->staticSACs.push_back(precond);
                SACs.push_back(sac);
            }
        }
    }
    swap(task->SACs, SACs);

    vector<ActionFluent*> actionFluents;
    int nextIndex = 0;
    bool foundInapplicableActionFluent = false;
    for (ActionFluent* af : task->actionFluents) {
        if (fluentIsInapplicable[af->index]) {
            // cout << "action fluent " << af->fullName << " is inapplicable" << endl;
            assert(replacements.find(af) == replacements.end());
            replacements[af] = new NumericConstant(0.0);
            foundInapplicableActionFluent = true;
        } else {
            af->index = nextIndex;
            ++nextIndex;

            actionFluents.push_back(af);
        }
    }
    swap(task->actionFluents, actionFluents);
    return foundInapplicableActionFluent;
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
    int numActionFluents = task->actionFluents.size();
    int numConcurrentActions = task->numberOfConcurrentActions;
    // If there is only one action fluent (left), there is nothing else to do
    if (numActionFluents == 1) {
        return false;
    }

    // If max-nondef-actions doesn't constrain action applicability and there
    // are no other preconditions, no pair of action fluents can be mutex
    if ((numConcurrentActions > 1) && task->actionPreconds.empty() &&
        task->staticSACs.empty()) {
         return false;
    }

    vector<set<int>> mutexGroups;
    z3::context c;
    z3::solver s(c);
    vector<z3::expr> sfExprs;
    vector<z3::expr> afExprs;
    task->buildCSP(c, s, sfExprs, afExprs, true);

    if (numConcurrentActions == 1) {
        // If max-nondef-actions is 1, all action fluents are pairwise mutex
        // and they can be combined to a single FDR action fluent
        set<int> mutexGroup;
        for (size_t index = 0; index < numActionFluents; ++index) {
            mutexGroup.insert(index);
        }
        mutexGroups.push_back(mutexGroup);
    } else {
        // TODO: Everything below here (or maybe even including the generation of
        //  the CSP above) should be outsourced to another class that can be
        //  replaced if we feel the need to implement a different mutex generator.

        vector<set<int>> mutexes =
            computeActionFluentMutexes(s, afExprs);

        // Given a set of mutexes, it is non-trivial to decide which variables to
        // combine to a single FDR variable, and it is not clear which combination
        // it the "best". We therefore use a cheap, greedy approach here that
        // starts with the first variable (we could use a random one instead, but
        // that introduces unnecessary non-determinism), greedily adds variables
        // to form a mutex group and continues in the same way with the first
        // variable that had not been added this way.
        for (size_t index1 = 0; index1 < numActionFluents; ++index1) {
            set<int>& mutexes1 = mutexes[index1];
            if (mutexes1.empty()) {
                // This variable has already been added to a mutex group
                continue;
            }
            set<int> mutexGroup;
            mutexGroup.insert(index1);
            // mutexes1.clear();

            for (int index2 : mutexes1) {
                // Indices smaller than index1 have been checked before
                if (index2 <= index1) {
                    continue;
                }

                // Check if index2 is mutex with all action fluents in mutexGroup
                set<int>& mutexes2 = mutexes[index2];
                if (all_of(mutexGroup.begin(), mutexGroup.end(), [&](int index) {
                    return (index == index2) || (mutexes2.find(index) != mutexes2.end());
                })) {
                    mutexGroup.insert(index2);
                    mutexes2.clear();
                }
            }
            mutexes1.clear();
            mutexGroups.push_back(mutexGroup);
        }
    }

    if (mutexGroups.size() == numActionFluents) {
        return false;
    }

    vector<ActionFluent*> actionFluents;
    cout << "        Determined the following mutex groups:" << endl;
    for (set<int> const& mutexGroup : mutexGroups) {
        if (mutexGroup.size() > 1) {
            string name = "FDR-action-fluent-" +
                          to_string(numGeneratedFDRActionFluents);
            ++numGeneratedFDRActionFluents;
            cout << "        " << name << ": ";

            Type* valueType =
                new Type(name + "-type", nullptr);
            ActionFluent* finiteDomainAF =
                new ActionFluent(name, valueType);



            // TODO: The implementation we had here isn't working, presumably
            //  because there are some parts of the code that treat the value
            //  '0' as a special value that corresponds to noop. We therefore
            //  had to comment the check if the 'none-of-those' value is
            //  required and now always create it. It would of course be better
            //  if the value is only created when it is actually needed. A good
            //  domain to test this is earth-observation-2018.

            // Check if at least one of these action fluents must be selected or
            // if there is also a "none-of-those" value
            int valIndex = 0;
            // s.push();
            // for (int id : mutexGroup) {
            //     s.add(af_exprs[id] == 0);
            // }
            // if (s.check() == z3::sat) {
            valueType->objects.push_back(
                new Object("none-of-those", valueType, valIndex));
            ++valIndex;
            // }
            // s.pop();

            for (int id : mutexGroup) {
                ActionFluent* af = task->actionFluents[id];

                // Remove spaces from action name
                string name = af->fullName;
                cout << name << " ";
                replace(name.begin(), name.end(), ' ', '~');

                // Generate object that represents the new value
                valueType->objects.push_back(
                    new Object(name, valueType, valIndex));

                // Replace all occurences of (old) binary action fluent with (new)
                // FDR action fluent
                vector<LogicalExpression*> eq = {finiteDomainAF, new NumericConstant(valIndex)};
                replacements[af] = new EqualsExpression(eq);

                ++valIndex;
            }
            cout << endl;
            actionFluents.push_back(finiteDomainAF);
        } else {
            int id = *mutexGroup.begin();
            ActionFluent* af = task->actionFluents[id];
            cout << af->fullName;
            actionFluents.push_back(af);
        }
    }
    swap(task->actionFluents, actionFluents);
    sortActionFluents();
    return true;
}

vector<set<int>> Simplifier::computeActionFluentMutexes(
    z3::solver& s, vector<z3::expr>& af_exprs) const {
    // TODO: Order the action fluent pairs in a way that the most promising ones
    //  are considered earlier, and such that we can stop this if it takes too
    //  much time (the code below is quadratic in the number of action fluents)

    int numActionFluents = task->actionFluents.size();
    vector<set<int>> result(numActionFluents);

    for (size_t i = 0; i < numActionFluents; ++i) {
        ActionFluent* af1 = task->actionFluents[i];
        // We use an empty mutex set later on to determine that an action
        // fluent has already been assigned to a mutex group, and add every
        // action fluent to its own mutex group to distinguish the case where
        // an action fluent is not mutex from any other action fluent.
        result[i].insert(i);

        // We do not consider action fluents that are already in FDR
        if (af1->valueType->objects.size() > 2) {
            continue;
        }

        for (size_t j = i + 1; j < numActionFluents; ++j) {
            ActionFluent* af2 = task->actionFluents[j];
            // We do not consider action fluents that are already in FDR
            if (af2->valueType->objects.size() > 2) {
                continue;
            }

            // Add the constraint that both action fluents are true and check
            // if the CSP has a solution. If it hasn't, the action fluents are
            // mutex.
            s.push();
            s.add((af_exprs[i] == 1) && (af_exprs[j] == 1));
            if (s.check() != z3::sat) {
                cout << "        " << af1->fullName << " and " << af2->fullName
                     << " are mutex" << endl;
                result[i].insert(j);
                result[j].insert(i);
            }
            s.pop();
        }
    }
    return result;
}

bool Simplifier::computeActions(
    map<ParametrizedVariable*, LogicalExpression*>& replacements) {
    sortActionFluents();
    int numActionFluents = task->actionFluents.size();
    vector<ActionState> legalActionStates;
    
    z3::context c;
    z3::solver s(c);
    vector<z3::expr> sfExprs;
    vector<z3::expr> afExprs;
    task->buildCSP(c, s, sfExprs, afExprs, true);

    // We compute models (i.e., assignments to the state and action variables)
    // for the planning task, use the assigned action variables as legal action
    // state (being part of the model means there is at least one state where
    // the assignment is legal) and invalidate the action assignment for the
    // next iteration. The procedure terminates when there are no more models.
    while (s.check() == z3::sat) {
        z3::model model = s.get_model();

        // Add solution to actions and invalidate it for next iteration
        vector<int> action(numActionFluents);
        z3::expr block = c.bool_val(false);
        for (size_t index = 0; index < numActionFluents; ++index) {
            z3::expr const& afExpr = afExprs[index];
            // The internal representation of numbers in Z3 does not use ints,
            // so the conversion is non-trivial. A way that is typically
            // recommended is to convert to a string and from there to an int.
            int value = atoi(model.eval(afExpr).to_string().c_str());

            action[index] = value;
            block = block || (afExpr != value);
        }
        legalActionStates.emplace_back(move(action));
        s.add(block);
    }
    swap(task->actionStates, legalActionStates);

    // Check if there are action fluents that are false in every legal action.
    // We can safely remove those action fluents from the problem.
    vector<bool> fluentIsUsed(numActionFluents, false);
    for (ActionState const& actionState : task->actionStates) {
        vector<int> const& state = actionState.state;
        for (unsigned int index = 0; index < state.size(); ++index) {
            fluentIsUsed[index] =
                fluentIsUsed[index] || static_cast<bool>(state[index]);
        }
    }

    bool foundUnusedActionFluent = false;
    vector<ActionFluent*> newActionFluents;
    for (ActionFluent* af : task->actionFluents) {
        if (fluentIsUsed[af->index]) {
            int newIndex = newActionFluents.size();
            newActionFluents.push_back(af);
            af->index = newIndex;
        } else {
            assert(replacements.find(af) == replacements.end());
            replacements[af] = new NumericConstant(0.0);
            foundUnusedActionFluent = true;
        }
    }
    task->actionFluents = newActionFluents;

    return foundUnusedActionFluent;
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

    z3::context c;
    z3::solver s(c);
    vector<z3::expr> sfExprs;
    vector<z3::expr> afExprs;
    task->buildCSP(c, s, sfExprs, afExprs, false);

    // Check for each action state and precondition if that precondition could
    // forbid the application of the action state. We do this by checking if
    // there is a state where the precondition could be evaluated to false given
    // the action is applied.
    int numActions = task->actionStates.size();
    int numPreconds = task->actionPreconds.size();
    vector<bool> precondIsRelevant(numPreconds, false);
    for (size_t index = 0; index < numActions; ++index) {
        s.push();
        ActionState& actionState = task->actionStates[index];
        actionState.index = index;

        for (size_t i = 0; i < actionState.state.size(); ++i) {
            if (actionState.state[i]) {
                actionState.scheduledActionFluents.push_back(
                    task->actionFluents[i]);
            }
            s.add(afExprs[i] == actionState.state[i]);
        }

        for (ActionPrecondition* precond : task->actionPreconds) {
            s.push();
            s.add(precond->formula->toZ3Formula(c, sfExprs, afExprs) == 0);
            if (s.check() == z3::sat) {
                actionState.relevantSACs.push_back(precond);
                precondIsRelevant[precond->index] = true;
            }
            s.pop();
        }
        s.pop();
    }

    // Remove irrelevant preconds
    vector<ActionPrecondition*> finalPreconds;
    for (ActionPrecondition* precond : task->actionPreconds) {
        if (precondIsRelevant[precond->index]) {
            precond->index = finalPreconds.size();
            finalPreconds.push_back(precond);
        }
    }
    swap(finalPreconds, task->actionPreconds);
}
