LogicalExpression* LogicalExpression::simplify(Simplifications& /*replace*/) {
    print(cout);
    assert(false);
    return nullptr;
}

/*****************************************************************
                           Atomics
*****************************************************************/

LogicalExpression* ParametrizedVariable::simplify(
    Simplifications& /*replace*/) {
    return this;
}

LogicalExpression* StateFluent::simplify(Simplifications& replace) {
    if (replace.find(this) != replace.end()) {
        return new NumericConstant(replace[this]);
    }
    return this;
}

LogicalExpression* ActionFluent::simplify(Simplifications& replace) {
    if (replace.find(this) != replace.end()) {
        return new NumericConstant(replace[this]);
    }
    return this;
}

LogicalExpression* NumericConstant::simplify(Simplifications& /*replace*/) {
    return this;
}

/*****************************************************************
                           Connectives
*****************************************************************/

LogicalExpression* Conjunction::simplify(Simplifications& replace) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(replace);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
        if (nc) {
            if (MathUtils::doubleIsEqual(nc->value, 0.0)) {
                // False constant element -> conjunction is always false
                return new NumericConstant(0.0);
            }
            // Otherwise, tis is a true constant element that can be omitted
        } else {
            Conjunction* conj = dynamic_cast<Conjunction*>(newExpr);
            if (conj) {
                newExprs.insert(newExprs.end(), conj->exprs.begin(),
                                conj->exprs.end());
            } else {
                newExprs.push_back(newExpr);
            }
        }
    }

    if (newExprs.empty()) {
        // All elements are constants and true -> Conjunction is always true
        return new NumericConstant(1.0);
    } else if (newExprs.size() == 1) {
        return newExprs[0];
    }

    return new Conjunction(newExprs);
}

LogicalExpression* Disjunction::simplify(Simplifications& replace) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(replace);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
        if (nc) {
            if (!MathUtils::doubleIsEqual(nc->value, 0.0)) {
                // True constant element -> disjunction is always true
                return new NumericConstant(1.0);
            }
            // Otherwise, tis is a false constant element that can be omitted
        } else {
            Disjunction* disj = dynamic_cast<Disjunction*>(newExpr);
            if (disj) {
                newExprs.insert(newExprs.end(), disj->exprs.begin(),
                                disj->exprs.end());
            } else {
                newExprs.push_back(newExpr);
            }
        }
    }

    if (newExprs.empty()) {
        // All elements are constants and false -> Disjunction is always false
        return new NumericConstant(0.0);
    } else if (newExprs.size() == 1) {
        return newExprs[0];
    }

    return new Disjunction(newExprs);
}

LogicalExpression* EqualsExpression::simplify(Simplifications& replace) {
    vector<LogicalExpression*> newExprs;
    NumericConstant* constComp = nullptr;

    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(replace);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);

        if (nc) {
            // This element is a numeric constant
            if (constComp) {
                // ...and there is already another constant one
                if (!MathUtils::doubleIsEqual(nc->value, constComp->value)) {
                    // ...which is inequal -> this is always false
                    return new NumericConstant(0.0);
                }
                // ...which is equal, so we can omit this one
            } else {
                // ...and it is the first constant one
                constComp = nc;
                newExprs.push_back(newExpr);
            }
        } else {
            // This is not constant
            newExprs.push_back(newExpr);
        }
    }

    if (newExprs.size() == 1) {
        // Either all were constant and equal to the comparator, or there was
        // only one (constant or dynamic) -> this is always true
        return new NumericConstant(1.0);
    }

    // We must keep this and check, as there are at least 2 expressions which
    // might be equal or not, depending on the state
    return new EqualsExpression(newExprs);
}

LogicalExpression* GreaterExpression::simplify(Simplifications& replace) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(replace);
    LogicalExpression* expr1 = exprs[1]->simplify(replace);

    NumericConstant* nc0 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr1);

    if (nc0 && nc1) {
        if (MathUtils::doubleIsGreater(nc0->value, nc1->value)) {
            return new NumericConstant(1.0);
        } else {
            return new NumericConstant(0.0);
        }
    }

    vector<LogicalExpression*> newExprs;
    newExprs.push_back(expr0);
    newExprs.push_back(expr1);

    return new GreaterExpression(newExprs);
}

LogicalExpression* LowerExpression::simplify(Simplifications& replace) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(replace);
    LogicalExpression* expr1 = exprs[1]->simplify(replace);

    NumericConstant* nc0 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr1);

    if (nc0 && nc1) {
        if (MathUtils::doubleIsSmaller(nc0->value, nc1->value)) {
            return new NumericConstant(1.0);
        } else {
            return new NumericConstant(0.0);
        }
    }

    vector<LogicalExpression*> newExprs;
    newExprs.push_back(expr0);
    newExprs.push_back(expr1);

    return new LowerExpression(newExprs);
}

LogicalExpression* GreaterEqualsExpression::simplify(Simplifications& replace) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(replace);
    LogicalExpression* expr1 = exprs[1]->simplify(replace);

    NumericConstant* nc0 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr1);

    if (nc0 && nc1) {
        if (MathUtils::doubleIsGreaterOrEqual(nc0->value, nc1->value)) {
            return new NumericConstant(1.0);
        } else {
            return new NumericConstant(0.0);
        }
    }

    vector<LogicalExpression*> newExprs;
    newExprs.push_back(expr0);
    newExprs.push_back(expr1);

    return new GreaterEqualsExpression(newExprs);
}

LogicalExpression* LowerEqualsExpression::simplify(Simplifications& replace) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(replace);
    LogicalExpression* expr1 = exprs[1]->simplify(replace);

    NumericConstant* nc0 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr1);

    if (nc0 && nc1) {
        if (MathUtils::doubleIsSmallerOrEqual(nc0->value, nc1->value)) {
            return new NumericConstant(1.0);
        } else {
            return new NumericConstant(0.0);
        }
    }

    vector<LogicalExpression*> newExprs;
    newExprs.push_back(expr0);
    newExprs.push_back(expr1);

    return new LowerEqualsExpression(newExprs);
}

LogicalExpression* Addition::simplify(Simplifications& replace) {
    vector<LogicalExpression*> newExprs;
    double constSum = 0.0;

    // Simplify and collect all constant expressions
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(replace);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
        if (nc) {
            constSum += nc->value;
        } else {
            newExprs.push_back(newExpr);
        }
    }

    if (newExprs.empty() && MathUtils::doubleIsEqual(constSum, 0.0)) {
        return new NumericConstant(0.0);
    } else if (newExprs.empty() && !MathUtils::doubleIsEqual(constSum, 0.0)) {
        return new NumericConstant(constSum);
    } else if (newExprs.size() == 1 &&
               MathUtils::doubleIsEqual(constSum, 0.0)) {
        return newExprs[0];
    }

    vector<LogicalExpression*> finalExprs;
    for (unsigned int i = 0; i < newExprs.size(); ++i) {
        Addition* add = dynamic_cast<Addition*>(newExprs[i]);
        if (add) {
            finalExprs.insert(finalExprs.end(), add->exprs.begin(),
                              add->exprs.end());
            // If the merged addition had an constant element it must have been
            // the last, so its also the last element in finalExprs now.
            NumericConstant* nc =
                dynamic_cast<NumericConstant*>(*finalExprs.rbegin());
            if (nc) {
                constSum += nc->value;
                finalExprs.pop_back();
            }
        } else {
            finalExprs.push_back(newExprs[i]);
        }
    }

    // TODO: Collect all Subtractions and merge

    // Add the constant part of the addition
    if (!MathUtils::doubleIsEqual(constSum, 0.0)) {
        finalExprs.push_back(new NumericConstant(constSum));
    }

    return new Addition(finalExprs);
}

LogicalExpression* Subtraction::simplify(Simplifications& replace) {
    assert(exprs.size() >= 2);
    vector<LogicalExpression*> newExprs;
    double constPart = 0.0;

    LogicalExpression* minuend = exprs[0]->simplify(replace);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(minuend);
    bool minuendIsConst = false;

    if (nc1) {
        constPart = nc1->value;
        minuendIsConst = true;
    } else {
        newExprs.push_back(minuend);
    }

    // Simplify and collect all constant expressions
    for (unsigned int i = 1; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(replace);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
        if (nc) {
            constPart -= nc->value;
        } else {
            newExprs.push_back(newExpr);
        }
    }

    // TODO: Collect all Additions and Subtractions and merge

    if (newExprs.empty()) {
        // All elements are constant!
        assert(minuendIsConst);
        return new NumericConstant(constPart);
    }

    if (minuendIsConst) {
        // The minuend is a constant -> insert it at the beginning!
        assert(!newExprs.empty());
        newExprs.insert(newExprs.begin(), new NumericConstant(constPart));
    } else {
        // The minuend is not a constant, so the const part is a subtrahend
        assert(!newExprs.empty());
        if (!MathUtils::doubleIsEqual(constPart, 0.0)) {
            newExprs.push_back(new NumericConstant(constPart * -1.0));
        }
    }

    if (newExprs.size() == 1) {
        return newExprs[0];
    }
    assert(newExprs.size() >= 2);

    return new Subtraction(newExprs);
}

LogicalExpression* Multiplication::simplify(Simplifications& replace) {
    vector<LogicalExpression*> newExprs;
    double constMult = 1.0;

    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(replace);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
        if (nc) {
            if (MathUtils::doubleIsEqual(nc->value, 0.0)) {
                return new NumericConstant(0.0);
            }
            constMult *= nc->value;
        } else {
            newExprs.push_back(newExpr);
        }
    }

    if (!MathUtils::doubleIsEqual(constMult, 1.0)) {
        newExprs.push_back(new NumericConstant(constMult));
    }

    if (newExprs.empty()) {
        return new NumericConstant(1.0);
    } else if (newExprs.size() == 1) {
        return newExprs[0];
    }

    // TODO: check if there are multiplications in newExprs

    return new Multiplication(newExprs);
}

LogicalExpression* Division::simplify(Simplifications& replace) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(replace);
    LogicalExpression* expr1 = exprs[1]->simplify(replace);

    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc2 = dynamic_cast<NumericConstant*>(expr1);

    if (nc1 && nc2) {
        return new NumericConstant((nc1->value / nc2->value));
    }

    vector<LogicalExpression*> newExprs;
    newExprs.push_back(expr0);
    newExprs.push_back(expr1);

    return new Division(newExprs);
}

/*****************************************************************
                          Unaries
*****************************************************************/

LogicalExpression* Negation::simplify(Simplifications& replace) {
    LogicalExpression* newExpr = expr->simplify(replace);

    NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
    if (nc) {
        if (MathUtils::doubleIsEqual(nc->value, 0.0)) {
            return new NumericConstant(1.0);
        }
        return new NumericConstant(0.0);
    }

    Negation* neg = dynamic_cast<Negation*>(newExpr);
    if (neg) {
        return neg->expr;
    }

    return new Negation(newExpr);
}

LogicalExpression* ExponentialFunction::simplify(Simplifications& replace) {
    LogicalExpression* newExpr = expr->simplify(replace);

    NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
    if (nc) {
        return new NumericConstant(std::exp(nc->value));
    }

    return new ExponentialFunction(newExpr);
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

LogicalExpression* KronDeltaDistribution::simplify(Simplifications& replace) {
    return expr->simplify(replace);
}

LogicalExpression* BernoulliDistribution::simplify(Simplifications& replace) {
    LogicalExpression* newExpr = expr->simplify(replace);
    NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
    if (nc) {
        if (MathUtils::doubleIsEqual(nc->value, 0.0)) {
            return new NumericConstant(0.0);
        } else if (MathUtils::doubleIsGreaterOrEqual(nc->value, 1.0) ||
                   MathUtils::doubleIsSmaller(nc->value, 0.0)) {
            return new NumericConstant(1.0);
        }
    }

    return new BernoulliDistribution(newExpr);
}

// TODO: If there is a constant probability equal to 1 or higher (or lower than
// 0), what do we do? Can we simplify such that the according value is always
// true?
LogicalExpression* DiscreteDistribution::simplify(Simplifications& replace) {
    vector<LogicalExpression*> newValues;
    vector<LogicalExpression*> newProbs;

    for (unsigned int i = 0; i < values.size(); ++i) {
        newValues.push_back(values[i]->simplify(replace));
        newProbs.push_back(probabilities[i]->simplify(replace));
    }

    // Remove all values with constant probability 0
    for (unsigned int i = 0; i < newProbs.size(); ++i) {
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newProbs[i]);
        if (nc && MathUtils::doubleIsEqual(nc->value, 0.0)) {
            swap(newValues[i], newValues[newValues.size() - 1]);
            newValues.pop_back();
            swap(newProbs[i], newProbs[newProbs.size() - 1]);
            newProbs.pop_back();
            --i;
        }
    }

    assert(!newValues.empty());

    // If only one value is left it must have probability 1 and this is a
    // KronDelta distribution
    if (newValues.size() > 1) {
        return new DiscreteDistribution(newValues, newProbs);
    } else {
        return newValues[0];
    }
}

/*****************************************************************
                         Conditionals
*****************************************************************/

LogicalExpression* IfThenElseExpression::simplify(Simplifications& replace) {
    LogicalExpression* newCondition = condition->simplify(replace);
    LogicalExpression* newValueIfTrue = valueIfTrue->simplify(replace);
    LogicalExpression* newValueIfFalse = valueIfFalse->simplify(replace);

    // Check if the condition is a constant
    NumericConstant* ncCond = dynamic_cast<NumericConstant*>(newCondition);
    if (ncCond) {
        if (MathUtils::doubleIsEqual(ncCond->value, 0.0)) {
            return newValueIfFalse;
        } else {
            return newValueIfTrue;
        }
    }

    // Check for cases of the form: "if a then 1 else 0" (and vice
    // versa) and simplify to "a" ("not a"); Also check for "If a then
    // const else const" and return "const" (TODO: This would be
    // possible in general, but then we need a comparison operator of
    // LogicalExpressions).
    NumericConstant* ncTrue = dynamic_cast<NumericConstant*>(newValueIfTrue);
    NumericConstant* ncFalse = dynamic_cast<NumericConstant*>(newValueIfFalse);

    if (ncTrue && ncFalse) {
        if (MathUtils::doubleIsEqual(ncTrue->value, 1.0) &&
            MathUtils::doubleIsEqual(ncFalse->value, 0.0)) {
            return newCondition;
        } else if (MathUtils::doubleIsEqual(ncTrue->value, 0.0) &&
                   MathUtils::doubleIsEqual(ncFalse->value, 1.0)) {
            Negation* res = new Negation(newCondition);
            return res->simplify(replace);
        } else if (MathUtils::doubleIsEqual(ncTrue->value, ncFalse->value)) {
            return ncTrue;
        }
    }

    // Check for nested IfThenElseExpressions and MultiConditionCheckers
    IfThenElseExpression* thenIf =
        dynamic_cast<IfThenElseExpression*>(newValueIfTrue);
    MultiConditionChecker* thenMCC =
        dynamic_cast<MultiConditionChecker*>(newValueIfTrue);
    IfThenElseExpression* elseIf =
        dynamic_cast<IfThenElseExpression*>(newValueIfFalse);
    MultiConditionChecker* elseMCC =
        dynamic_cast<MultiConditionChecker*>(newValueIfFalse);

    if (thenIf || thenMCC || elseIf || elseMCC) {
        vector<LogicalExpression*> conditions;
        vector<LogicalExpression*> effects;

        if (thenIf) {
            vector<LogicalExpression*> combinedConds;
            combinedConds.push_back(newCondition);
            combinedConds.push_back(thenIf->condition);
            Conjunction* combinedCond = new Conjunction(combinedConds);

            conditions.push_back(combinedCond->simplify(replace));
            effects.push_back(thenIf->valueIfTrue);

            conditions.push_back(newCondition);
            effects.push_back(thenIf->valueIfFalse);
        } else if (thenMCC) {
            for (unsigned int i = 0; i < thenMCC->conditions.size(); ++i) {
                vector<LogicalExpression*> combinedConds;
                combinedConds.push_back(newCondition);
                combinedConds.push_back(thenMCC->conditions[i]);

                Conjunction* combinedCond = new Conjunction(combinedConds);
                conditions.push_back(combinedCond->simplify(replace));
                effects.push_back(thenMCC->effects[i]);
            }
        } else {
            conditions.push_back(newCondition);
            effects.push_back(newValueIfTrue);
        }

        if (elseIf) {
            conditions.push_back(elseIf->condition);
            conditions.push_back(new NumericConstant(1.0));

            effects.push_back(elseIf->valueIfTrue);
            effects.push_back(elseIf->valueIfFalse);
        } else if (elseMCC) {
            conditions.insert(conditions.end(), elseMCC->conditions.begin(),
                              elseMCC->conditions.end());

            effects.insert(effects.end(), elseMCC->effects.begin(),
                           elseMCC->effects.end());
        } else {
            conditions.push_back(new NumericConstant(1.0));
            effects.push_back(newValueIfFalse);
        }

        return new MultiConditionChecker(conditions, effects);
    }

    return new IfThenElseExpression(newCondition, newValueIfTrue,
                                    newValueIfFalse);
}

LogicalExpression* MultiConditionChecker::simplify(Simplifications& replace) {
    vector<LogicalExpression*> newConditions;
    vector<LogicalExpression*> newEffects;

    for (unsigned int i = 0; i < conditions.size(); ++i) {
        LogicalExpression* newCond = conditions[i]->simplify(replace);
        LogicalExpression* newEff = effects[i]->simplify(replace);

        // Check if the condition is a constant
        NumericConstant* ncCond = dynamic_cast<NumericConstant*>(newCond);
        if (ncCond) {
            if (MathUtils::doubleIsEqual(ncCond->value, 0.0)) {
                continue;
            } else {
                newConditions.push_back(new NumericConstant(1.0));
                newEffects.push_back(newEff);
                break;
            }
        }

        newConditions.push_back(newCond);
        newEffects.push_back(newEff);
    }

    if (newConditions.size() == 1) {
        return newEffects[0];
    }

    return new MultiConditionChecker(newConditions, newEffects);
}
