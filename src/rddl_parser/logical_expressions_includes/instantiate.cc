LogicalExpression* LogicalExpression::instantiate(PlanningTask* /*task*/,
                                                  Instantiations& /*replace*/) {
    assert(false);
    return nullptr;
}

/*****************************************************************
                         Schematics
*****************************************************************/

LogicalExpression* ParametrizedVariable::instantiate(PlanningTask* task,
                                                     Instantiations& replace) {
    vector<Object*> newParams;
    for (unsigned int i = 0; i < params.size(); ++i) {
        Object* param = dynamic_cast<Object*>(params[i]);
        if (!param) {
            assert(replace.find(params[i]->name) != replace.end());
            param = replace[params[i]->name];
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
        double& val = task->getNonFluent(instantiatedName)->initialValue;
        return new NumericConstant(val);
        break;
    }
    assert(false);
    return nullptr;
}

/*****************************************************************
                           Atomics
*****************************************************************/

LogicalExpression* NumericConstant::instantiate(PlanningTask* /*task*/,
                                                Instantiations& /*replace*/) {
    return this;
}

LogicalExpression* Parameter::instantiate(PlanningTask* task,
                                          Instantiations& replace) {
    // This is only called if this parameter is not a fluent's parameter but to
    // compare parameters and/or objects
    assert(replace.find(name) != replace.end());
    return replace[name]->instantiate(task, replace);
}

LogicalExpression* Object::instantiate(PlanningTask* /*task*/,
                                       Instantiations& /*replace*/) {
    return new NumericConstant(value);
}

/*****************************************************************
                           Connectives
*****************************************************************/

LogicalExpression* Conjunction::instantiate(PlanningTask* task,
                                            Instantiations& replace) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replace));
    }
    return new Conjunction(newExpr);
}
LogicalExpression* Disjunction::instantiate(PlanningTask* task,
                                            Instantiations& replace) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replace));
    }
    return new Disjunction(newExpr);
}

LogicalExpression* EqualsExpression::instantiate(PlanningTask* task,
                                                 Instantiations& replace) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replace));
    }
    return new EqualsExpression(newExpr);
}

LogicalExpression* GreaterExpression::instantiate(PlanningTask* task,
                                                  Instantiations& replace) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replace));
    }
    return new GreaterExpression(newExpr);
}

LogicalExpression* LowerExpression::instantiate(PlanningTask* task,
                                                Instantiations& replace) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replace));
    }
    return new LowerExpression(newExpr);
}

LogicalExpression* GreaterEqualsExpression::instantiate(
    PlanningTask* task, Instantiations& replace) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replace));
    }
    return new GreaterEqualsExpression(newExpr);
}

LogicalExpression* LowerEqualsExpression::instantiate(PlanningTask* task,
                                                      Instantiations& replace) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replace));
    }
    return new LowerEqualsExpression(newExpr);
}

LogicalExpression* Addition::instantiate(PlanningTask* task,
                                         Instantiations& replace) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replace));
    }
    return new Addition(newExpr);
}

LogicalExpression* Subtraction::instantiate(PlanningTask* task,
                                            Instantiations& replace) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replace));
    }
    return new Subtraction(newExpr);
}

LogicalExpression* Multiplication::instantiate(PlanningTask* task,
                                               Instantiations& replace) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replace));
    }
    return new Multiplication(newExpr);
}

LogicalExpression* Division::instantiate(PlanningTask* task,
                                         Instantiations& replace) {
    vector<LogicalExpression*> newExpr;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExpr.push_back(exprs[i]->instantiate(task, replace));
    }
    return new Division(newExpr);
}

/*****************************************************************
                          Unaries
*****************************************************************/

LogicalExpression* Negation::instantiate(PlanningTask* task,
                                         Instantiations& replace) {
    LogicalExpression* newExpr = expr->instantiate(task, replace);
    return new Negation(newExpr);
}

LogicalExpression* ExponentialFunction::instantiate(PlanningTask* task,
                                                    Instantiations& replace) {
    LogicalExpression* newExpr = expr->instantiate(task, replace);
    return new ExponentialFunction(newExpr);
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

LogicalExpression* KronDeltaDistribution::instantiate(PlanningTask* task,
                                                      Instantiations& replace) {
    LogicalExpression* newExpr = expr->instantiate(task, replace);
    return new KronDeltaDistribution(newExpr);
}

LogicalExpression* BernoulliDistribution::instantiate(PlanningTask* task,
                                                      Instantiations& replace) {
    LogicalExpression* newExpr = expr->instantiate(task, replace);
    return new BernoulliDistribution(newExpr);
}

LogicalExpression* DiscreteDistribution::instantiate(PlanningTask* task,
                                                     Instantiations& replace) {
    vector<LogicalExpression*> newValues;
    vector<LogicalExpression*> newProbs;

    for (unsigned int i = 0; i < values.size(); ++i) {
        newValues.push_back(values[i]->instantiate(task, replace));
        newProbs.push_back(probabilities[i]->instantiate(task, replace));
    }
    return new DiscreteDistribution(newValues, newProbs);
}

/*****************************************************************
                         Conditionals
*****************************************************************/

LogicalExpression* IfThenElseExpression::instantiate(PlanningTask* task,
                                                     Instantiations& replace) {
    LogicalExpression* newCondition = condition->instantiate(task, replace);
    LogicalExpression* newValueIfTrue = valueIfTrue->instantiate(task, replace);
    LogicalExpression* newValueIfFalse =
        valueIfFalse->instantiate(task, replace);
    return new IfThenElseExpression(newCondition, newValueIfTrue,
                                    newValueIfFalse);
}

LogicalExpression* MultiConditionChecker::instantiate(PlanningTask* task,
                                                      Instantiations& replace) {
    std::vector<LogicalExpression*> newConditions;
    std::vector<LogicalExpression*> newEffects;

    for (unsigned int i = 0; i < conditions.size(); ++i) {
        newConditions.push_back(conditions[i]->instantiate(task, replace));
        newEffects.push_back(effects[i]->instantiate(task, replace));
    }
    return new MultiConditionChecker(newConditions, newEffects);
}
