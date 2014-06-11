LogicalExpression* LogicalExpression::instantiate(PlanningTask* /*task*/,
        map<string,
            Object*>& /*replacements*/) {
    assert(false);
    return NULL;
}

/*****************************************************************
                         Schematics
*****************************************************************/

LogicalExpression* ParametrizedVariable::instantiate(PlanningTask* task,
        map<string,
            Object*>& replacements) {
    vector<Object*> newParams;
    for (unsigned int i = 0; i < params.size(); ++i) {
        Object* param = dynamic_cast<Object*>(params[i]);
        if (!param) {
            assert(replacements.find(params[i]->name) != replacements.end());
            param = replacements[params[i]->name];
        }
        newParams.push_back(param);
    }

    string instantiatedName(variableName);
    if (!newParams.empty()) {
        instantiatedName += "(";
        for (unsigned int i = 0; i < newParams.size(); ++i) {
            instantiatedName += newParams[i]->name;
            if (i != newParams.size() - 1) {
                instantiatedName += ", ";
            }
        }
        instantiatedName += ")";
    }

    switch (variableType) {
    case ParametrizedVariable::STATE_FLUENT:
        return task->getStateFluent(instantiatedName);
        break;
    case ParametrizedVariable::ACTION_FLUENT:
        return task->getActionFluent(instantiatedName);
        break;
    case ParametrizedVariable::NON_FLUENT:
        return new NumericConstant(task->getNonFluent(
                        instantiatedName)->initialValue);
        break;
    }
    assert(false);
    return NULL;
}

/*****************************************************************
                           Atomics
*****************************************************************/

LogicalExpression* NumericConstant::instantiate(PlanningTask* /*task*/,
        map<string,
            Object*>& /*replacements*/) {
    return this;
}

LogicalExpression* Parameter::instantiate(PlanningTask* task, map<string,
                                                                  Object*>&
        replacements) {
    // This is only called if this is not used as parameter but as part of an
    // EqualsExpression.
    assert(replacements.find(name) != replacements.end());
    return replacements[name]->instantiate(task, replacements);
}

LogicalExpression* Object::instantiate(PlanningTask* /*task*/, map<string,
                                                                   Object*>& /*replacements*/)
{
    // This is only called if this is not used as parameter but as part of an
    // EqualsExpression.
    if (values.size() != 1) {
        SystemUtils::abort("Error: Implement object fluents in instantiate.cc.");
    }
    return new NumericConstant(values[0]);
}

/*****************************************************************
                           Connectives
*****************************************************************/

LogicalExpression* Conjunction::instantiate(PlanningTask* task, map<string,
                                                                    Object*>&
        replacements) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new Conjunction(newExpr);
}
LogicalExpression* Disjunction::instantiate(PlanningTask* task, map<string,
                                                                    Object*>&
        replacements) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new Disjunction(newExpr);
}

LogicalExpression* EqualsExpression::instantiate(PlanningTask* task, map<string,
                                                                         Object
                                                                         *>&
        replacements) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new EqualsExpression(newExpr);
}

LogicalExpression* GreaterExpression::instantiate(PlanningTask* task,
        map<string,
            Object*>& replacements) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new GreaterExpression(newExpr);
}

LogicalExpression* LowerExpression::instantiate(PlanningTask* task, map<string,
                                                                        Object*>
        & replacements) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new LowerExpression(newExpr);
}

LogicalExpression* GreaterEqualsExpression::instantiate(PlanningTask* task,
        map<string,
            Object*>& replacements) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new GreaterEqualsExpression(newExpr);
}

LogicalExpression* LowerEqualsExpression::instantiate(PlanningTask* task,
        map<string,
            Object*>& replacements) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new LowerEqualsExpression(newExpr);
}

LogicalExpression* Addition::instantiate(PlanningTask* task, map<string,
                                                                 Object*>&
        replacements) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new Addition(newExpr);
}

LogicalExpression* Subtraction::instantiate(PlanningTask* task, map<string,
                                                                    Object*>&
        replacements) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new Subtraction(newExpr);
}

LogicalExpression* Multiplication::instantiate(PlanningTask* task, map<string,
                                                                       Object*>
        & replacements) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new Multiplication(newExpr);
}

LogicalExpression* Division::instantiate(PlanningTask* task, map<string,
                                                                 Object*>&
        replacements) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replacements));
    }
    return new Division(newExpr);
}

/*****************************************************************
                          Unaries
*****************************************************************/

LogicalExpression* Negation::instantiate(PlanningTask* task, map<string,
                                                                 Object*>&
        replacements) {
    LogicalExpression* newExpr = expr->instantiate(task, replacements);
    return new Negation(newExpr);
}

LogicalExpression* ExponentialFunction::instantiate(
        PlanningTask* task,
        map<string, Object*>& replacements) {
    LogicalExpression* newExpr = expr->instantiate(task, replacements);
    return new ExponentialFunction(newExpr);
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

LogicalExpression* KronDeltaDistribution::instantiate(PlanningTask* task,
        map<string,
            Object*>& replacements) {
    LogicalExpression* newExpr = expr->instantiate(task, replacements);
    return new KronDeltaDistribution(newExpr);
}

LogicalExpression* BernoulliDistribution::instantiate(PlanningTask* task,
        map<string,
            Object*>& replacements) {
    LogicalExpression* newExpr = expr->instantiate(task, replacements);
    return new BernoulliDistribution(newExpr);
}

LogicalExpression* DiscreteDistribution::instantiate(PlanningTask* task,
        map<string,
            Object*>& replacements) {
    vector<LogicalExpression*> newValues;
    vector<LogicalExpression*> newProbs;

    for (unsigned int i = 0; i < values.size(); ++i) {
        newValues.push_back(values[i]->instantiate(task, replacements));
        newProbs.push_back(probabilities[i]->instantiate(task, replacements));
    }
    return new DiscreteDistribution(newValues, newProbs);
}

/*****************************************************************
                         Conditionals
*****************************************************************/

LogicalExpression* IfThenElseExpression::instantiate(PlanningTask* task,
        map<string,
            Object*>& replacements) {
    LogicalExpression* newCondition = condition->instantiate(task, replacements);
    LogicalExpression* newValueIfTrue = valueIfTrue->instantiate(task,
            replacements);
    LogicalExpression* newValueIfFalse = valueIfFalse->instantiate(task,
            replacements);
    return new IfThenElseExpression(newCondition, newValueIfTrue,
            newValueIfFalse);
}

LogicalExpression* MultiConditionChecker::instantiate(PlanningTask* task,
        map<string,
            Object*>& replacements) {
    std::vector<LogicalExpression*> newConditions;
    std::vector<LogicalExpression*> newEffects;

    for (unsigned int i = 0; i < conditions.size(); ++i) {
        newConditions.push_back(conditions[i]->instantiate(task, replacements));
        newEffects.push_back(effects[i]->instantiate(task, replacements));
    }
    return new MultiConditionChecker(newConditions, newEffects);
}
