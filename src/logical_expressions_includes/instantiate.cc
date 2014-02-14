LogicalExpression* LogicalExpression::instantiate(UnprocessedPlanningTask* /*task*/, map<string, Object*>& /*replacements*/) {
    assert(false);
    return NULL;
}

/*****************************************************************
                         Schematics
*****************************************************************/

LogicalExpression* UninstantiatedVariable::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    vector<Object*> newParams;
    for(unsigned int i = 0; i < params.size(); ++i) {
        if(replacements.find(params[i]) != replacements.end()) {
            newParams.push_back(replacements[params[i]]);
        } else {
            newParams.push_back(task->getObject(params[i]));
        }
    }

    name = parent->name + "(";
    for(unsigned int i = 0; i < newParams.size(); ++i) {
        name += newParams[i]->name;
        if(i != newParams.size()-1) {
            name += ", ";
        }
    }
    name += ")";
    
    switch(parent->variableType) {
    case VariableDefinition::STATE_FLUENT:
    case VariableDefinition::INTERM_FLUENT:
        return task->getStateFluent(this);
        break;
    case VariableDefinition::ACTION_FLUENT:
        return task->getActionFluent(this);
        break;
    case VariableDefinition::NON_FLUENT:
        return new NumericConstant(task->getNonFluent(this)->initialValue);
        break;
    }
    assert(false);
    return NULL;
}

LogicalExpression* AtomicLogicalExpression::instantiate(UnprocessedPlanningTask* /*task*/, map<string, Object*>& /*replacements*/) {
    return this;
}

/*****************************************************************
                           Atomics
*****************************************************************/

LogicalExpression* NumericConstant::instantiate(UnprocessedPlanningTask* /*task*/, map<string, Object*>& /*replacements*/) {
    return this;
}

/*****************************************************************
                           Connectives
*****************************************************************/

LogicalExpression* Conjunction::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    vector<LogicalExpression*> newExpr;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new Conjunction(newExpr);
}
LogicalExpression* Disjunction::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    vector<LogicalExpression*> newExpr;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new Disjunction(newExpr);
}

LogicalExpression* EqualsExpression::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    vector<LogicalExpression*> newExpr;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new EqualsExpression(newExpr);
}

LogicalExpression* GreaterExpression::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    vector<LogicalExpression*> newExpr;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new GreaterExpression(newExpr);
}

LogicalExpression* LowerExpression::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    vector<LogicalExpression*> newExpr;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new LowerExpression(newExpr);
}

LogicalExpression* GreaterEqualsExpression::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    vector<LogicalExpression*> newExpr;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new GreaterEqualsExpression(newExpr);
}

LogicalExpression* LowerEqualsExpression::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    vector<LogicalExpression*> newExpr;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new LowerEqualsExpression(newExpr);
}

LogicalExpression* Addition::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    vector<LogicalExpression*> newExpr;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new Addition(newExpr);
}

LogicalExpression* Subtraction::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    vector<LogicalExpression*> newExpr;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new Subtraction(newExpr);
}

LogicalExpression* Multiplication::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    vector<LogicalExpression*> newExpr;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new Multiplication(newExpr);
}

LogicalExpression* Division::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    vector<LogicalExpression*> newExpr;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new Division(newExpr);
}

/*****************************************************************
                          Unaries
*****************************************************************/

LogicalExpression* NegateExpression::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    LogicalExpression* newExpr = expr->instantiate(task, replacements);
    return new NegateExpression(newExpr);
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

LogicalExpression* KronDeltaDistribution::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    LogicalExpression* newExpr = expr->instantiate(task, replacements);
    return new KronDeltaDistribution(newExpr);
}

LogicalExpression* BernoulliDistribution::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    LogicalExpression* newExpr = expr->instantiate(task, replacements);
    return new BernoulliDistribution(newExpr);
}

LogicalExpression* DiscreteDistribution::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    vector<LogicalExpression*> newValues;
    vector<LogicalExpression*> newProbs;

    for(unsigned int i = 0; i < values.size(); ++i) {
        newValues.push_back(values[i]->instantiate(task, replacements));
        newProbs.push_back(probabilities[i]->instantiate(task, replacements));
    }
    return new DiscreteDistribution(newValues, newProbs);
}

/*****************************************************************
                         Conditionals
*****************************************************************/

LogicalExpression* IfThenElseExpression::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    LogicalExpression* newCondition = condition->instantiate(task, replacements);
    LogicalExpression* newValueIfTrue = valueIfTrue->instantiate(task, replacements);
    LogicalExpression* newValueIfFalse = valueIfFalse->instantiate(task, replacements);
    return new IfThenElseExpression(newCondition, newValueIfTrue, newValueIfFalse);
}

LogicalExpression* MultiConditionChecker::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    std::vector<LogicalExpression*> newConditions;
    std::vector<LogicalExpression*> newEffects;

    for(unsigned int i = 0; i < conditions.size(); ++i) {
        newConditions.push_back(conditions[i]->instantiate(task, replacements));
        newEffects.push_back(effects[i]->instantiate(task, replacements));
    }
    return new MultiConditionChecker(newConditions, newEffects);
}
