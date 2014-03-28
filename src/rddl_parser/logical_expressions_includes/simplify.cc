LogicalExpression* LogicalExpression::simplify(map<StateFluent*, double>& /*replacements*/) {
    print(cout);
    assert(false);
    return NULL;
}

/*****************************************************************
                           Atomics
*****************************************************************/

LogicalExpression* ParametrizedVariable::simplify(map<StateFluent*, double>& /*replacements*/) {
    return this;
}

LogicalExpression* StateFluent::simplify(map<StateFluent*, double>& replacements) {
    if(replacements.find(this) != replacements.end()) {
        return new NumericConstant(replacements[this]);
    }
    return this;
}

LogicalExpression* NumericConstant::simplify(map<StateFluent*, double>& /*replacements*/) {
    return this;
}

/*****************************************************************
                           Connectives
*****************************************************************/

LogicalExpression* Conjunction::simplify(map<StateFluent*, double>& replacements) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(replacements);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
        if(nc) {
            if(MathUtils::doubleIsEqual(nc->value,0.0)) {
                // False constant element -> Conjunction is always false
                return new NumericConstant(0.0);
            }
            // else true constant element -> can be omitted
        } else {
            Conjunction* conj = dynamic_cast<Conjunction*>(newExpr);
            if(conj) {
                newExprs.insert(newExprs.end(),conj->exprs.begin(),conj->exprs.end());
            } else {
                newExprs.push_back(newExpr);
            }
        }
    }

    if(newExprs.empty()) {
        return new NumericConstant(1.0);//all elements are constants and true -> Conjunction is always true
    } else if(newExprs.size() == 1) {
        return newExprs[0];
    }

    return new Conjunction(newExprs);    
}

LogicalExpression* Disjunction::simplify(map<StateFluent*, double>& replacements) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(replacements);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
        if(nc) {
            if(!MathUtils::doubleIsEqual(nc->value,0.0)) {
                return new NumericConstant(1.0);//true constant element -> Disjunction is always true
            }
        } else {
            Disjunction* disj = dynamic_cast<Disjunction*>(newExpr);
            if(disj) {
                newExprs.insert(newExprs.end(),disj->exprs.begin(),disj->exprs.end());
            } else {
                newExprs.push_back(newExpr);
            }
        }
    }

    if(newExprs.empty()) {
        return new NumericConstant(0.0);//all elements are constants and false -> Disjunction is always false
    } else if(newExprs.size() == 1) {
        return newExprs[0];
    }

    return new Disjunction(newExprs);   
}

LogicalExpression* EqualsExpression::simplify(map<StateFluent*, double>& replacements) {
    vector<LogicalExpression*> newExprs;
    NumericConstant* constComp = NULL;

    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(replacements);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);

        if(nc) {
            if(constComp) {
                //...and there is already another constant one
                if(!MathUtils::doubleIsEqual(nc->value, constComp->value)) {
                    //...which is inequal -> this is always false
                    return new NumericConstant(0.0);
                }
                //...which is equal -> we dont need to compare to the second
                //one as it is the same as the one stored in constComp
            } else {
                //...and the first constant one
                constComp = nc;
                newExprs.push_back(newExpr);
            }
        } else {
            //this is not constant
           newExprs.push_back(newExpr);
        } 
    }

    if(newExprs.size() == 1) {
        //either all were constant and equal to the comparator, or there was only
        //one (constant or dynamic) -> this is always true
       return new NumericConstant(1.0);
    }

    //we must keep this and check, as there are at least 2 expressions which
    //might be equal or not, depending on the state
    return new EqualsExpression(newExprs);
}

LogicalExpression* GreaterExpression::simplify(map<StateFluent*, double>& replacements) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(replacements);
    LogicalExpression* expr1 = exprs[1]->simplify(replacements);

    NumericConstant* nc0 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr1);

    if(nc0 && nc1) {
        if(MathUtils::doubleIsGreater(nc0->value,nc1->value)) {
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

LogicalExpression* LowerExpression::simplify(map<StateFluent*, double>& replacements) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(replacements);
    LogicalExpression* expr1 = exprs[1]->simplify(replacements);

    NumericConstant* nc0 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr1);

    if(nc0 && nc1) {
        if(MathUtils::doubleIsSmaller(nc0->value,nc1->value)) {
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

LogicalExpression* GreaterEqualsExpression::simplify(map<StateFluent*, double>& replacements) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(replacements);
    LogicalExpression* expr1 = exprs[1]->simplify(replacements);

    NumericConstant* nc0 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr1);

    if(nc0 && nc1) {
        if(MathUtils::doubleIsGreaterOrEqual(nc0->value,nc1->value)) {
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

LogicalExpression* LowerEqualsExpression::simplify(map<StateFluent*, double>& replacements) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(replacements);
    LogicalExpression* expr1 = exprs[1]->simplify(replacements);

    NumericConstant* nc0 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr1);

    if(nc0 && nc1) {
        if(MathUtils::doubleIsSmallerOrEqual(nc0->value,nc1->value)) {
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

LogicalExpression* Addition::simplify(map<StateFluent*, double>& replacements) {
    vector<LogicalExpression*> newExprs;
    double constSum = 0.0;

    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(replacements);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
        if(nc) {
            constSum += nc->value;
        } else {
            newExprs.push_back(newExpr);
        }
    }

    if(newExprs.empty() && MathUtils::doubleIsEqual(constSum, 0.0)) {
        return new NumericConstant(0.0);
    } else if(newExprs.empty() && !MathUtils::doubleIsEqual(constSum, 0.0)) {
        return new NumericConstant(constSum);
    } else if(newExprs.size() == 1 && MathUtils::doubleIsEqual(constSum, 0.0)) {
        return newExprs[0];
    }

    vector<LogicalExpression*> finalExprs;
    for(unsigned int i = 0; i < newExprs.size(); ++i) {
        Addition* add = dynamic_cast<Addition*>(newExprs[i]);
        if(add) {
            finalExprs.insert(finalExprs.end(),add->exprs.begin(),add->exprs.end());
             //if the merged addition had an constant element it must have been the last!
            NumericConstant* nc = dynamic_cast<NumericConstant*>(finalExprs[finalExprs.size()-1]);
            if(nc) {
                constSum += nc->value;
                finalExprs.pop_back();
            }
        } else {
            finalExprs.push_back(newExprs[i]);
        }
    }

    if(!MathUtils::doubleIsEqual(constSum, 0.0)) {
        finalExprs.push_back(new NumericConstant(constSum));
    }

    return new Addition(finalExprs);
}

LogicalExpression* Subtraction::simplify(map<StateFluent*, double>& replacements) {
    assert(exprs.size() >= 2);
    vector<LogicalExpression*> newExprs;
    double constPart = 0.0;

    LogicalExpression* firstExpr = exprs[0]->simplify(replacements);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(firstExpr);
    bool firstPartIsConst = false;

    if(nc1) {
        constPart = nc1->value;
        firstPartIsConst = true;
    } else {
        newExprs.push_back(firstExpr);
    }
    
    for(unsigned int i = 1; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(replacements);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
        if(nc) {
            constPart -= nc->value;
        } else {
            newExprs.push_back(newExpr);
        }
    }

    if(newExprs.empty()) {//all elements are constant!
        assert(firstPartIsConst);
        return new NumericConstant(constPart);
    }

    if(firstPartIsConst) {//the first element is a constant -> insert it at the beginning!
        assert(!newExprs.empty());
        newExprs.insert(newExprs.begin(), new NumericConstant(constPart));
    } else {//the first element is not a constant
        assert(!newExprs.empty());
        if(!MathUtils::doubleIsEqual(constPart,0.0)) {
            newExprs.push_back(new NumericConstant(constPart*-1.0));
        }
    }

    if(newExprs.size() == 1) {
        return newExprs[0];
    }
    assert(newExprs.size() >= 2);

    return new Subtraction(newExprs);
}

LogicalExpression* Multiplication::simplify(map<StateFluent*, double>& replacements) {
    vector<LogicalExpression*> newExprs;
    double constMult = 1.0;

    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(replacements);
        NumericConstant* nc  = dynamic_cast<NumericConstant*>(newExpr);
        if(nc) {
            if(MathUtils::doubleIsEqual(nc->value, 0.0)) {
                return new NumericConstant(0.0);
            }
            constMult *= nc->value;
        } else {
            newExprs.push_back(newExpr);
        }
    }

    if(!MathUtils::doubleIsEqual(constMult, 1.0)) {
        newExprs.push_back(new NumericConstant(constMult));
    }

    if(newExprs.empty()) {
        return new NumericConstant(1.0); // TODO: is an empty multiplication equal to 1 or to 0?
    } else if(newExprs.size() == 1) {
        return newExprs[0];
    }

    //TODO: check if there are multiplications in newExprs

    return new Multiplication(newExprs);
}

LogicalExpression* Division::simplify(map<StateFluent*, double>& replacements) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(replacements);
    LogicalExpression* expr1 = exprs[1]->simplify(replacements);

    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc2 = dynamic_cast<NumericConstant*>(expr1);

    if(nc1 && nc2) {
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

LogicalExpression* Negation::simplify(map<StateFluent*, double>& replacements) {
    LogicalExpression* newExpr = expr->simplify(replacements);

    NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
    if(nc) {
        if(MathUtils::doubleIsEqual(nc->value,0.0)) {
            return new NumericConstant(1.0);
        }
        return new NumericConstant(0.0);
    }

    Negation* neg = dynamic_cast<Negation*>(newExpr);
    if(neg) {
        return neg->expr;
    }

    return new Negation(newExpr);
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

LogicalExpression* KronDeltaDistribution::simplify(map<StateFluent*, double>& replacements) {
    return expr->simplify(replacements);
}

LogicalExpression* BernoulliDistribution::simplify(map<StateFluent*, double>& replacements) {
    LogicalExpression* newExpr = expr->simplify(replacements);
    NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
    if(nc) {
        if(MathUtils::doubleIsEqual(nc->value, 0.0)) {
            return new NumericConstant(0.0);
        } else if(MathUtils::doubleIsGreaterOrEqual(nc->value, 1.0) || MathUtils::doubleIsSmaller(nc->value, 0.0)) {
            return new NumericConstant(1.0);
        }
    }

    return new BernoulliDistribution(newExpr);
}

// TODO: If there is a constant probability equal to 1 or higher (or lower than
// 0), what do we do? Can we simplify such that the according value is always
// true?
LogicalExpression* DiscreteDistribution::simplify(map<StateFluent*,double>& replacements) {
    vector<LogicalExpression*> newValues;
    vector<LogicalExpression*> newProbs;

    for(unsigned int i = 0; i < values.size(); ++i) {
        newValues.push_back(values[i]->simplify(replacements));
        newProbs.push_back(probabilities[i]->simplify(replacements));
    }

    // Remove all values with constant probability 0
    for(unsigned int i = 0; i < newProbs.size(); ++i) {
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newProbs[i]);
        if(nc && MathUtils::doubleIsEqual(nc->value, 0.0)) {
            swap(newValues[i], newValues[newValues.size()-1]);
            newValues.pop_back();
            swap(newProbs[i], newProbs[newProbs.size()-1]);
            newProbs.pop_back();
            --i;
        }
    }

    assert(!newValues.empty());

    // If only one value is left it must have probability 1 and this is a
    // KronDelta distribution
    if(newValues.size() > 1) {
        return new DiscreteDistribution(newValues, newProbs);
    } else {
        return newValues[0];
    }
}

/*****************************************************************
                         Conditionals
*****************************************************************/

LogicalExpression* IfThenElseExpression::simplify(map<StateFluent*, double>& replacements) {
    LogicalExpression* newCondition = condition->simplify(replacements);
    LogicalExpression* newValueIfTrue = valueIfTrue->simplify(replacements);
    LogicalExpression* newValueIfFalse = valueIfFalse->simplify(replacements);

    // Check if the condition is a constant
    NumericConstant* ncCond = dynamic_cast<NumericConstant*>(newCondition);
    if(ncCond) {
        if(MathUtils::doubleIsEqual(ncCond->value, 0.0)) {
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

    if(ncTrue && ncFalse) {
        if(MathUtils::doubleIsEqual(ncTrue->value, 1.0) && MathUtils::doubleIsEqual(ncFalse->value, 0.0)) {
            return newCondition;
        } else if(MathUtils::doubleIsEqual(ncTrue->value, 0.0) && MathUtils::doubleIsEqual(ncFalse->value, 1.0)) {
            Negation* res = new Negation(newCondition);
            return res->simplify(replacements);
        } else if(MathUtils::doubleIsEqual(ncTrue->value, ncFalse->value)) {
            return ncTrue;
        }
    }

    // Check for cases of the form "If a then b else if ..." and
    // simplify to a MultiConditionChecker
    IfThenElseExpression* elseIf = dynamic_cast<IfThenElseExpression*>(newValueIfFalse);
    if(elseIf) {
        vector<LogicalExpression*> conditions;
        conditions.push_back(newCondition);
        conditions.push_back(elseIf->condition);
        conditions.push_back(new NumericConstant(1.0));

        vector<LogicalExpression*> effects;
        effects.push_back(newValueIfTrue);
        effects.push_back(elseIf->valueIfTrue);
        effects.push_back(elseIf->valueIfFalse);

        MultiConditionChecker* mc = new MultiConditionChecker(conditions,effects);
        return mc->simplify(replacements);
    }

    // Check if there is a condition checker nested inside this
    MultiConditionChecker* elseMCC = dynamic_cast<MultiConditionChecker*>(newValueIfFalse);
    if(elseMCC) {
        vector<LogicalExpression*> conditions;
        conditions.push_back(newCondition);
        conditions.insert(conditions.end(),elseMCC->conditions.begin(), elseMCC->conditions.end());

        vector<LogicalExpression*> effects;
        effects.push_back(newValueIfTrue);
        effects.insert(effects.end(),elseMCC->effects.begin(), elseMCC->effects.end());

        MultiConditionChecker* mc = new MultiConditionChecker(conditions,effects);
        return mc->simplify(replacements);
    }

    return new IfThenElseExpression(newCondition, newValueIfTrue, newValueIfFalse);
}

LogicalExpression* MultiConditionChecker::simplify(map<StateFluent*,double>& replacements) {
    vector<LogicalExpression*> newConditions;
    vector<LogicalExpression*> newEffects;

    for(unsigned int i = 0; i < conditions.size(); ++i) {
        LogicalExpression* newCond = conditions[i]->simplify(replacements);
        LogicalExpression* newEff = effects[i]->simplify(replacements);

        // Check if the condition is a constant
        NumericConstant* ncCond = dynamic_cast<NumericConstant*>(newCond);
        if(ncCond) {
            if(MathUtils::doubleIsEqual(ncCond->value, 0.0)) {
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

    return new MultiConditionChecker(newConditions, newEffects);
}
