LogicalExpression* LogicalExpression::replaceQuantifier(UnprocessedPlanningTask* /*task*/, map<string,string>& /*replacements*/, Instantiator* /*instantiator*/) {
    assert(false);
    return NULL;
}

/*****************************************************************
                         Schematics
*****************************************************************/

LogicalExpression* UninstantiatedVariable::replaceQuantifier(UnprocessedPlanningTask* /*task*/, map<string, string>& replacements, Instantiator* /*instantiator*/) {
    vector<string> newParams;
    for(unsigned int i = 0; i < params.size(); ++i) {
        if(replacements.find(params[i]) != replacements.end()) {
            newParams.push_back(replacements[params[i]]);
        } else {
            newParams.push_back(params[i]);
        }
    }
    return new UninstantiatedVariable(parent, newParams);
}

/*****************************************************************
                           Atomics
*****************************************************************/

LogicalExpression* NumericConstant::replaceQuantifier(UnprocessedPlanningTask* /*task*/, map<string,string>& /*replacements*/, Instantiator* /*instantiator*/) {
    return this;
}

/*****************************************************************
                          Quantifier
*****************************************************************/

void Quantifier::getReplacements(UnprocessedPlanningTask* task, vector<string>& parameterNames, vector<vector<Object*> >& replacements, Instantiator* instantiator) {
    vector<ObjectType*> parameterTypes;
    for(unsigned int i = 0; i < parameterDefsSet->parameterDefs.size(); ++i) {
        parameterNames.push_back(parameterDefsSet->parameterDefs[i]->parameterName);
        parameterTypes.push_back(parameterDefsSet->parameterDefs[i]->parameterType);
    }
    instantiator->instantiateParams(task, parameterTypes, replacements);
}

LogicalExpression* Sumation::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    vector<string> parameterNames;
    vector<vector<Object*> > newParams;
    getReplacements(task, parameterNames, newParams, instantiator);
    for(unsigned int i = 0; i < newParams.size(); ++i) {
        map<string,string> replacementsCopy = replacements;
        assert(newParams[i].size() == parameterNames.size());
        for(unsigned int j = 0; j < newParams[i].size(); ++j) {
            assert(replacementsCopy.find(parameterNames[j]) == replacementsCopy.end());
            replacementsCopy[parameterNames[j]] = newParams[i][j]->name;
        }
        LogicalExpression* e = expr->replaceQuantifier(task, replacementsCopy, instantiator);
        newExprs.push_back(e);
    }
    return new Addition(newExprs);
}

LogicalExpression* Product::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    vector<string> parameterNames;
    vector<vector<Object*> > newParams;
    getReplacements(task, parameterNames, newParams, instantiator);
    for(unsigned int i = 0; i < newParams.size(); ++i) {
        map<string,string> replacementsCopy = replacements;
        assert(newParams[i].size() == parameterNames.size());
        for(unsigned int j = 0; j < newParams[i].size(); ++j) {
            assert(replacementsCopy.find(parameterNames[j]) == replacementsCopy.end());
            replacementsCopy[parameterNames[j]] = newParams[i][j]->name;
        }
        LogicalExpression* e = expr->replaceQuantifier(task, replacementsCopy, instantiator);
        newExprs.push_back(e);
    }
    return new Multiplication(newExprs);
}

LogicalExpression* UniversalQuantification::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    vector<string> parameterNames;
    vector<vector<Object*> > newParams;
    getReplacements(task, parameterNames, newParams, instantiator);
    for(unsigned int i = 0; i < newParams.size(); ++i) {
        map<string,string> replacementsCopy(replacements);
        assert(newParams[i].size() == parameterNames.size());
        for(unsigned int j = 0; j < newParams[i].size(); ++j) {
            assert(replacementsCopy.find(parameterNames[j]) == replacementsCopy.end());
            replacementsCopy[parameterNames[j]] = newParams[i][j]->name;
        }
        LogicalExpression* e = expr->replaceQuantifier(task, replacementsCopy, instantiator);
        newExprs.push_back(e);
    }
    return new Conjunction(newExprs);
}

LogicalExpression* ExistentialQuantification::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    vector<string> parameterNames;
    vector<vector<Object*> > newParams;
    getReplacements(task, parameterNames, newParams, instantiator);

    for(unsigned int i = 0; i < newParams.size(); ++i) {
        map<string,string> replacementsCopy = replacements;
        assert(newParams[i].size() == parameterNames.size());
        for(unsigned int j = 0; j < newParams[i].size(); ++j) {
            assert(replacementsCopy.find(parameterNames[j]) == replacementsCopy.end());
            replacementsCopy[parameterNames[j]] = newParams[i][j]->name;
        }
        LogicalExpression* e = expr->replaceQuantifier(task, replacementsCopy, instantiator);
        newExprs.push_back(e);
    }
    return new Disjunction(newExprs);
}

/*****************************************************************
                           Connectives
*****************************************************************/

LogicalExpression* Conjunction::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(task, replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new Conjunction(newExprs);
}

LogicalExpression* Disjunction::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(task, replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new Disjunction(newExprs);
}

LogicalExpression* EqualsExpression::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(task, replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new EqualsExpression(newExprs);
}

LogicalExpression* GreaterExpression::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(task, replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new GreaterExpression(newExprs);
}

LogicalExpression* LowerExpression::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(task, replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new LowerExpression(newExprs);
}

LogicalExpression* GreaterEqualsExpression::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(task, replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new GreaterEqualsExpression(newExprs);
}

LogicalExpression* LowerEqualsExpression::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(task, replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new LowerEqualsExpression(newExprs);
}

LogicalExpression* Addition::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(task, replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new Addition(newExprs);
}

LogicalExpression* Subtraction::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(task, replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new Subtraction(newExprs);
}

LogicalExpression* Multiplication::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(task, replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new Multiplication(newExprs);
}

LogicalExpression* Division::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(task, replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new Division(newExprs);
}

/*****************************************************************
                          Unaries
*****************************************************************/

LogicalExpression* NegateExpression::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    LogicalExpression* newExpr = expr->replaceQuantifier(task, replacements, instantiator);
    return new NegateExpression(newExpr);
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

LogicalExpression* KronDeltaDistribution::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    LogicalExpression* newExpr = expr->replaceQuantifier(task, replacements, instantiator);
    return new KronDeltaDistribution(newExpr);
}

LogicalExpression* BernoulliDistribution::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    LogicalExpression* newExpr = expr->replaceQuantifier(task, replacements, instantiator);
    return new BernoulliDistribution(newExpr);
}

LogicalExpression* DiscreteDistribution::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newValues;
    vector<LogicalExpression*> newProbs;

    for(unsigned int i = 0; i < values.size(); ++i) {
        newValues.push_back(values[i]->replaceQuantifier(task, replacements, instantiator));
        newProbs.push_back(probabilities[i]->replaceQuantifier(task, replacements, instantiator));
    }
    return new DiscreteDistribution(newValues, newProbs);
}

/*****************************************************************
                         Conditionals
*****************************************************************/

LogicalExpression* IfThenElseExpression::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    LogicalExpression* newCondition = condition->replaceQuantifier(task, replacements, instantiator);
    LogicalExpression* newValueIfTrue = valueIfTrue->replaceQuantifier(task, replacements, instantiator);
    LogicalExpression* newValueIfFalse = valueIfFalse->replaceQuantifier(task, replacements, instantiator);
    return new IfThenElseExpression(newCondition, newValueIfTrue, newValueIfFalse);
}

LogicalExpression* MultiConditionChecker::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    std::vector<LogicalExpression*> newConditions;
    std::vector<LogicalExpression*> newEffects;

    for(unsigned int i = 0; i < conditions.size(); ++i) {
        newConditions.push_back(conditions[i]->replaceQuantifier(task, replacements, instantiator));
        newEffects.push_back(effects[i]->replaceQuantifier(task, replacements, instantiator));
    }
    return new MultiConditionChecker(newConditions, newEffects);
}
