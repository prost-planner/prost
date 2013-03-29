void Quantifier::getReplacements(UnprocessedPlanningTask* task, vector<string>& parameterNames, vector<vector<Object*> >& replacements, Instantiator* instantiator) {
    vector<ObjectType*> parameterTypes;
    for(unsigned int i = 0; i < parameterDefsSet->parameterDefs.size(); ++i) {
        parameterNames.push_back(parameterDefsSet->parameterDefs[i]->parameterName);
        parameterTypes.push_back(parameterDefsSet->parameterDefs[i]->parameterType);
    }
    instantiator->instantiateParams(task, parameterTypes, replacements);
}

//void replaceQuantifier(UnprocessedPlanningTask* task, map<string, string>& replacements)

LogicalExpression* LogicalExpression::replaceQuantifier(UnprocessedPlanningTask* /*task*/, map<string,string>& /*replacements*/, Instantiator* /*instantiator*/) {
    assert(false);
    return NULL;
}

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

LogicalExpression* NumericConstant::replaceQuantifier(UnprocessedPlanningTask* /*task*/, map<string,string>& /*replacements*/, Instantiator* /*instantiator*/) {
    return this;
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

LogicalExpression* BernoulliDistribution::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    LogicalExpression* newExpr = expr->replaceQuantifier(task, replacements, instantiator);
    return new BernoulliDistribution(newExpr);
}

LogicalExpression* KronDeltaDistribution::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    LogicalExpression* newExpr = expr->replaceQuantifier(task, replacements, instantiator);
    return new KronDeltaDistribution(newExpr);
}

LogicalExpression* IfThenElseExpression::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    LogicalExpression* newCondition = condition->replaceQuantifier(task, replacements, instantiator);
    LogicalExpression* newValueIfTrue = valueIfTrue->replaceQuantifier(task, replacements, instantiator);
    LogicalExpression* newValueIfFalse = valueIfFalse->replaceQuantifier(task, replacements, instantiator);
    return new IfThenElseExpression(newCondition, newValueIfTrue, newValueIfFalse);
}

LogicalExpression* NegateExpression::replaceQuantifier(UnprocessedPlanningTask* task, map<string,string>& replacements, Instantiator* instantiator) {
    LogicalExpression* newExpr = expr->replaceQuantifier(task, replacements, instantiator);
    return new NegateExpression(newExpr);
}

//LogicalExpression* ParameterDefinition::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements)

LogicalExpression* LogicalExpression::instantiate(UnprocessedPlanningTask* /*task*/, map<string, Object*>& /*replacements*/) {
    assert(false);
    return NULL;
}

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
        return task->getConstant(this);
        break;
    }
    assert(false);
    return NULL;
}

LogicalExpression* AtomicLogicalExpression::instantiate(UnprocessedPlanningTask* /*task*/, map<string, Object*>& /*replacements*/) {
    return this;
}

LogicalExpression* NumericConstant::instantiate(UnprocessedPlanningTask* /*task*/, map<string, Object*>& /*replacements*/) {
    return this;
}

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

LogicalExpression* BernoulliDistribution::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    LogicalExpression* newExpr = expr->instantiate(task, replacements);
    return new BernoulliDistribution(newExpr);
}

LogicalExpression* KronDeltaDistribution::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    LogicalExpression* newExpr = expr->instantiate(task, replacements);
    return new KronDeltaDistribution(newExpr);
}

LogicalExpression* IfThenElseExpression::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    LogicalExpression* newCondition = condition->instantiate(task, replacements);
    LogicalExpression* newValueIfTrue = valueIfTrue->instantiate(task, replacements);
    LogicalExpression* newValueIfFalse = valueIfFalse->instantiate(task, replacements);
    return new IfThenElseExpression(newCondition, newValueIfTrue, newValueIfFalse);
}

LogicalExpression* NegateExpression::instantiate(UnprocessedPlanningTask* task, map<string, Object*>& replacements) {
    LogicalExpression* newExpr = expr->instantiate(task, replacements);
    return new NegateExpression(newExpr);
}
