LogicalExpression* LogicalExpression::simplify(UnprocessedPlanningTask* /*task*/, map<StateFluent*, NumericConstant*>& /*replacements*/) {
    assert(false);
    return NULL;
}

LogicalExpression* StateFluent::simplify(UnprocessedPlanningTask* /*task*/, map<StateFluent*, NumericConstant*>& replacements) {
    if(replacements.find(this) != replacements.end()) {
        return replacements[this];
    }
    return this;
}

LogicalExpression* AtomicLogicalExpression::simplify(UnprocessedPlanningTask* /*task*/, map<StateFluent*, NumericConstant*>& /*replacements*/) {
    return this;
}

LogicalExpression* NumericConstant::simplify(UnprocessedPlanningTask* /*task*/, map<StateFluent*, NumericConstant*>& /*replacements*/) {
    return this;
}

LogicalExpression* Conjunction::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(task, replacements);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
        if(nc) {
            if(MathUtils::doubleIsEqual(nc->value,0.0)) {
                return NumericConstant::falsity();//false constant element -> Conjunction is always false
            }
            //else true constant element -> can be omitted
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
        return NumericConstant::truth();//all elements are constants and true -> Conjunction is always true
    } else if(newExprs.size() == 1) {
        return newExprs[0];
    }

    return new Conjunction(newExprs);    
}

LogicalExpression* Disjunction::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(task, replacements);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
        if(nc) {
            if(!MathUtils::doubleIsEqual(nc->value,0.0)) {
                return NumericConstant::truth();//true constant element -> Disjunction is always true
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
        return NumericConstant::falsity();//all elements are constants and false -> Disjunction is always false
    } else if(newExprs.size() == 1) {
        return newExprs[0];
    }

    return new Disjunction(newExprs);   
}

LogicalExpression* EqualsExpression::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    vector<LogicalExpression*> newExprs;
    NumericConstant* constComp = NULL;

    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(task, replacements);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);

        if(nc) {
            if(constComp) {
                //...and there is already another constant one
                if(!MathUtils::doubleIsEqual(nc->value, constComp->value)) {
                    //...which is inequal -> this is always false
                    return NumericConstant::falsity();
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
       return NumericConstant::truth();
    }

    //we must keep this and check, as there are at least 2 expressions which
    //might be equal or not, depending on the state
    return new EqualsExpression(newExprs);
}

LogicalExpression* GreaterExpression::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(task, replacements);
    LogicalExpression* expr1 = exprs[1]->simplify(task, replacements);

    NumericConstant* nc0 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr1);

    if(nc0 && nc1) {
        if(MathUtils::doubleIsGreater(nc0->value,nc1->value)) {
            return NumericConstant::truth();
        } else {
            return NumericConstant::falsity();
        }
    }

    vector<LogicalExpression*> newExprs;
    newExprs.push_back(expr0);
    newExprs.push_back(expr1);

    return new GreaterExpression(newExprs);
}

LogicalExpression* LowerExpression::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(task, replacements);
    LogicalExpression* expr1 = exprs[1]->simplify(task, replacements);

    NumericConstant* nc0 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr1);

    if(nc0 && nc1) {
        if(MathUtils::doubleIsSmaller(nc0->value,nc1->value)) {
            return NumericConstant::truth();
        } else {
            return NumericConstant::falsity();
        }
    }

    vector<LogicalExpression*> newExprs;
    newExprs.push_back(expr0);
    newExprs.push_back(expr1);

    return new LowerExpression(newExprs);
}

LogicalExpression* GreaterEqualsExpression::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(task, replacements);
    LogicalExpression* expr1 = exprs[1]->simplify(task, replacements);

    NumericConstant* nc0 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr1);

    if(nc0 && nc1) {
        if(MathUtils::doubleIsGreaterOrEqual(nc0->value,nc1->value)) {
            return NumericConstant::truth();
        } else {
            return NumericConstant::falsity();
        }
    }

    vector<LogicalExpression*> newExprs;
    newExprs.push_back(expr0);
    newExprs.push_back(expr1);

    return new GreaterEqualsExpression(newExprs);
}

LogicalExpression* LowerEqualsExpression::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(task, replacements);
    LogicalExpression* expr1 = exprs[1]->simplify(task, replacements);

    NumericConstant* nc0 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr1);

    if(nc0 && nc1) {
        if(MathUtils::doubleIsSmallerOrEqual(nc0->value,nc1->value)) {
            return NumericConstant::truth();
        } else {
            return NumericConstant::falsity();
        }
    }

    vector<LogicalExpression*> newExprs;
    newExprs.push_back(expr0);
    newExprs.push_back(expr1);

    return new LowerEqualsExpression(newExprs);
}

LogicalExpression* Addition::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    vector<LogicalExpression*> newExprs;
    double constSum = 0.0;

    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(task, replacements);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
        if(nc) {
            constSum += nc->value;
        } else {
            newExprs.push_back(newExpr);
        }
    }

    if(newExprs.empty() && MathUtils::doubleIsEqual(constSum, 0.0)) {
        return NumericConstant::falsity(); // TODO: is an empty addition equal to 1 or to 0?
    } else if(newExprs.empty() && !MathUtils::doubleIsEqual(constSum, 0.0)) {
        return task->getConstant(constSum);
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
        finalExprs.push_back(task->getConstant(constSum));
    }

    return new Addition(finalExprs);
}

LogicalExpression* Subtraction::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    assert(exprs.size() >= 2);
    vector<LogicalExpression*> newExprs;
    double constPart = 0.0;

    LogicalExpression* firstExpr = exprs[0]->simplify(task, replacements);
    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(firstExpr);
    bool firstPartIsConst = false;

    if(nc1) {
        constPart = nc1->value;
        firstPartIsConst = true;
    } else {
        newExprs.push_back(firstExpr);
    }
    
    for(unsigned int i = 1; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(task, replacements);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
        if(nc) {
            constPart -= nc->value;
        } else {
            newExprs.push_back(newExpr);
        }
    }

    if(newExprs.empty()) {//all elements are constant!
        assert(firstPartIsConst);
        return task->getConstant(constPart);
    }

    if(firstPartIsConst) {//the first element is a constant -> insert it at the beginning!
        assert(!newExprs.empty());
        newExprs.insert(newExprs.begin(),task->getConstant(constPart));
    } else {//the first element is not a constant
        assert(!newExprs.empty());
        if(!MathUtils::doubleIsEqual(constPart,0.0)) {
            newExprs.push_back(task->getConstant(constPart*-1.0));
        }
    }

    if(newExprs.size() == 1) {
        return newExprs[0];
    }
    assert(newExprs.size() >= 2);

    return new Subtraction(newExprs);
}

LogicalExpression* Multiplication::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    vector<LogicalExpression*> newExprs;
    double constMult = 1.0;

    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->simplify(task, replacements);
        NumericConstant* nc  = dynamic_cast<NumericConstant*>(newExpr);
        if(nc) {
            if(MathUtils::doubleIsEqual(nc->value, 0.0)) {
                return NumericConstant::falsity();
            }
            constMult *= nc->value;
        } else {
            newExprs.push_back(newExpr);
        }
    }

    if(!MathUtils::doubleIsEqual(constMult, 1.0)) {
        newExprs.push_back(task->getConstant(constMult));
    }

    if(newExprs.empty()) {
        return NumericConstant::truth(); // TODO: is an empty multiplication equal to 1 or to 0?
    } else if(newExprs.size() == 1) {
        return newExprs[0];
    }

    //TODO: check if there are multiplications in newExprs

    return new Multiplication(newExprs);
}

LogicalExpression* Division::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    assert(exprs.size() == 2);
    LogicalExpression* expr0 = exprs[0]->simplify(task, replacements);
    LogicalExpression* expr1 = exprs[1]->simplify(task, replacements);

    NumericConstant* nc1 = dynamic_cast<NumericConstant*>(expr0);
    NumericConstant* nc2 = dynamic_cast<NumericConstant*>(expr1);

    if(nc1 && nc2) {
        return task->getConstant((nc1->value / nc2->value));
    }

    vector<LogicalExpression*> newExprs;
    newExprs.push_back(expr0);
    newExprs.push_back(expr1);

    return new Division(newExprs);
}

LogicalExpression* BernoulliDistribution::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    LogicalExpression* newExpr = expr->simplify(task, replacements);
   NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
   if(nc) {
       if(!MathUtils::doubleIsGreater(nc->value,0.0)) {
           return NumericConstant::falsity();
       } else if(!MathUtils::doubleIsSmaller(nc->value,1.0)) {
           return NumericConstant::truth();
       }
   }

   AtomicLogicalExpression* sf = dynamic_cast<AtomicLogicalExpression*>(newExpr);
   if(sf) {
       return newExpr; //TODO: remove this for real-valued competition!!!!!!!!!!!
   }

   return new BernoulliDistribution(newExpr);
}

LogicalExpression* KronDeltaDistribution::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    return expr->simplify(task, replacements);
}

LogicalExpression* IfThenElseExpression::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    LogicalExpression* newCondition = condition->simplify(task, replacements);
    LogicalExpression* newValueIfTrue = valueIfTrue->simplify(task, replacements);
    LogicalExpression* newValueIfFalse = valueIfFalse->simplify(task, replacements);

    NumericConstant* ncCond = dynamic_cast<NumericConstant*>(newCondition);
    if(ncCond) {
        if(MathUtils::doubleIsEqual(ncCond->value, 0.0)) {
            return newValueIfFalse;
        } else {
            return newValueIfTrue;
        }
    }

    NumericConstant* ncTrue = dynamic_cast<NumericConstant*>(newValueIfTrue);
    NumericConstant* ncFalse = dynamic_cast<NumericConstant*>(newValueIfFalse);

    //check for cases of the form: if a then 1 else 0 (and vice versa)
    //these can be simplified to: a (not a)
    if(ncTrue && ncFalse) {
        if(MathUtils::doubleIsEqual(ncTrue->value,1.0) && MathUtils::doubleIsEqual(ncFalse->value,0.0)) {
            return newCondition;
        } else if(MathUtils::doubleIsEqual(ncTrue->value,0.0) && MathUtils::doubleIsEqual(ncFalse->value,1.0)) {
            NegateExpression* res = new NegateExpression(newCondition);
            return res->simplify(task,replacements);
        } else if(MathUtils::doubleIsEqual(ncTrue->value,ncFalse->value)) {
            return ncTrue;
        }
    }

    //check if this is part of if a then b else if ...
    IfThenElseExpression* elseIf = dynamic_cast<IfThenElseExpression*>(newValueIfFalse);

    if(elseIf) {
        vector<LogicalExpression*> conditions;
        conditions.push_back(newCondition);
        conditions.push_back(elseIf->condition);
        conditions.push_back(NumericConstant::truth());

        vector<LogicalExpression*> effects;
        effects.push_back(newValueIfTrue);
        effects.push_back(elseIf->valueIfTrue);
        effects.push_back(elseIf->valueIfFalse);

        MultiConditionChecker* mc = new MultiConditionChecker(conditions,effects);
        return mc->simplify(task, replacements);
    }

    MultiConditionChecker* elseMCC = dynamic_cast<MultiConditionChecker*>(newValueIfFalse);
    if(elseMCC) {
        vector<LogicalExpression*> conditions;
        conditions.push_back(newCondition);
        conditions.insert(conditions.end(),elseMCC->conditions.begin(), elseMCC->conditions.end());

        vector<LogicalExpression*> effects;
        effects.push_back(newValueIfTrue);
        effects.insert(effects.end(),elseMCC->effects.begin(), elseMCC->effects.end());

        MultiConditionChecker* mc = new MultiConditionChecker(conditions,effects);
        return mc->simplify(task, replacements);
    }

    return new IfThenElseExpression(newCondition, newValueIfTrue, newValueIfFalse);
}

LogicalExpression* MultiConditionChecker::simplify(UnprocessedPlanningTask* task, std::map<StateFluent*,NumericConstant*>& replacements) {
    vector<LogicalExpression*> newConditions;
    vector<LogicalExpression*> newEffects;

    for(unsigned int i = 0; i < conditions.size(); ++i) {
        newConditions.push_back(conditions[i]->simplify(task, replacements));
        newEffects.push_back(effects[i]->simplify(task, replacements));
    }

    return new MultiConditionChecker(newConditions, newEffects);
}

LogicalExpression* NegateExpression::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    LogicalExpression* newExpr = expr->simplify(task, replacements);

    NumericConstant* nc = dynamic_cast<NumericConstant*>(newExpr);
    if(nc) {
        if(MathUtils::doubleIsEqual(nc->value,0.0)) {
            return NumericConstant::truth();
        }
        return NumericConstant::falsity();
    }

    NegateExpression* neg = dynamic_cast<NegateExpression*>(newExpr);
    if(neg) {
        return neg->expr;
    }

    return new NegateExpression(newExpr);
}
