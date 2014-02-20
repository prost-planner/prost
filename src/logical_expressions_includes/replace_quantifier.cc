LogicalExpression* LogicalExpression::replaceQuantifier(map<string, Object*>& /*replacements*/, Instantiator* /*instantiator*/) {
    print(cout);
    assert(false);
    return NULL;
}

/*****************************************************************
                         Schematics
*****************************************************************/

LogicalExpression* UninstantiatedVariable::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<Parameter*> newParams;
    for(unsigned int i = 0; i < params.size(); ++i) {
        Parameter* param = dynamic_cast<Parameter*>(params[i]->replaceQuantifier(replacements, instantiator));
        assert(param);
        newParams.push_back(param);
    }
    return new UninstantiatedVariable(parent, newParams);
}

/*****************************************************************
                           Atomics
*****************************************************************/

LogicalExpression* NumericConstant::replaceQuantifier(map<string, Object*>& /*replacements*/, Instantiator* /*instantiator*/) {
    return this;
}

LogicalExpression* Parameter::replaceQuantifier(map<string, Object*>& replacements, Instantiator* /*instantiator*/) {
    if(replacements.find(name) != replacements.end()) {
        return replacements[name];
    }
    return this;
}

LogicalExpression* Object::replaceQuantifier(map<string, Object*>& /*replacements*/, Instantiator* /*instantiator*/) {
    return this;
}

/*****************************************************************
                          Quantifier
*****************************************************************/

void Quantifier::getReplacements(vector<string>& parameterNames, vector<vector<Object*> >& replacements, Instantiator* instantiator) {
    vector<ObjectType*> parameterTypes;
    for(unsigned int i = 0; i < paramList->params.size(); ++i) {
        parameterNames.push_back(paramList->params[i]->name);
        parameterTypes.push_back(paramList->types[i]);
    }
    instantiator->instantiateParams(parameterTypes, replacements);
}

LogicalExpression* Sumation::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    vector<string> parameterNames;
    vector<vector<Object*> > newParams;
    getReplacements(parameterNames, newParams, instantiator);
    for(unsigned int i = 0; i < newParams.size(); ++i) {
        map<string, Object*> replacementsCopy = replacements;
        assert(newParams[i].size() == parameterNames.size());
        for(unsigned int j = 0; j < newParams[i].size(); ++j) {
            assert(replacementsCopy.find(parameterNames[j]) == replacementsCopy.end());
            replacementsCopy[parameterNames[j]] = newParams[i][j];
        }
        LogicalExpression* e = expr->replaceQuantifier(replacementsCopy, instantiator);
        newExprs.push_back(e);
    }
    return new Addition(newExprs);
}

LogicalExpression* Product::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    vector<string> parameterNames;
    vector<vector<Object*> > newParams;
    getReplacements(parameterNames, newParams, instantiator);
    for(unsigned int i = 0; i < newParams.size(); ++i) {
        map<string, Object*> replacementsCopy = replacements;
        assert(newParams[i].size() == parameterNames.size());
        for(unsigned int j = 0; j < newParams[i].size(); ++j) {
            assert(replacementsCopy.find(parameterNames[j]) == replacementsCopy.end());
            replacementsCopy[parameterNames[j]] = newParams[i][j];
        }
        LogicalExpression* e = expr->replaceQuantifier(replacementsCopy, instantiator);
        newExprs.push_back(e);
    }
    return new Multiplication(newExprs);
}

LogicalExpression* UniversalQuantification::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    vector<string> parameterNames;
    vector<vector<Object*> > newParams;
    getReplacements(parameterNames, newParams, instantiator);
    for(unsigned int i = 0; i < newParams.size(); ++i) {
        map<string, Object*> replacementsCopy(replacements);
        assert(newParams[i].size() == parameterNames.size());
        for(unsigned int j = 0; j < newParams[i].size(); ++j) {
            assert(replacementsCopy.find(parameterNames[j]) == replacementsCopy.end());
            replacementsCopy[parameterNames[j]] = newParams[i][j];
        }
        LogicalExpression* e = expr->replaceQuantifier(replacementsCopy, instantiator);
        newExprs.push_back(e);
    }
    return new Conjunction(newExprs);
}

LogicalExpression* ExistentialQuantification::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    vector<string> parameterNames;
    vector<vector<Object*> > newParams;
    getReplacements(parameterNames, newParams, instantiator);

    for(unsigned int i = 0; i < newParams.size(); ++i) {
        map<string, Object*> replacementsCopy = replacements;
        assert(newParams[i].size() == parameterNames.size());
        for(unsigned int j = 0; j < newParams[i].size(); ++j) {
            assert(replacementsCopy.find(parameterNames[j]) == replacementsCopy.end());
            replacementsCopy[parameterNames[j]] = newParams[i][j];
        }
        LogicalExpression* e = expr->replaceQuantifier(replacementsCopy, instantiator);
        newExprs.push_back(e);
    }
    return new Disjunction(newExprs);
}

/*****************************************************************
                           Connectives
*****************************************************************/

LogicalExpression* Conjunction::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new Conjunction(newExprs);
}

LogicalExpression* Disjunction::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new Disjunction(newExprs);
}

LogicalExpression* EqualsExpression::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new EqualsExpression(newExprs);
}

LogicalExpression* GreaterExpression::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new GreaterExpression(newExprs);
}

LogicalExpression* LowerExpression::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new LowerExpression(newExprs);
}

LogicalExpression* GreaterEqualsExpression::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new GreaterEqualsExpression(newExprs);
}

LogicalExpression* LowerEqualsExpression::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new LowerEqualsExpression(newExprs);
}

LogicalExpression* Addition::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new Addition(newExprs);
}

LogicalExpression* Subtraction::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new Subtraction(newExprs);
}

LogicalExpression* Multiplication::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new Multiplication(newExprs);
}

LogicalExpression* Division::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->replaceQuantifier(replacements, instantiator);
        newExprs.push_back(newExpr);
    }
    return new Division(newExprs);
}

/*****************************************************************
                          Unaries
*****************************************************************/

LogicalExpression* Negation::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    LogicalExpression* newExpr = expr->replaceQuantifier(replacements, instantiator);
    return new Negation(newExpr);
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

LogicalExpression* KronDeltaDistribution::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    LogicalExpression* newExpr = expr->replaceQuantifier(replacements, instantiator);
    return new KronDeltaDistribution(newExpr);
}

LogicalExpression* BernoulliDistribution::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    LogicalExpression* newExpr = expr->replaceQuantifier(replacements, instantiator);
    return new BernoulliDistribution(newExpr);
}

LogicalExpression* DiscreteDistribution::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    vector<LogicalExpression*> newValues;
    vector<LogicalExpression*> newProbs;

    for(unsigned int i = 0; i < values.size(); ++i) {
        newValues.push_back(values[i]->replaceQuantifier(replacements, instantiator));
        newProbs.push_back(probabilities[i]->replaceQuantifier(replacements, instantiator));
    }
    return new DiscreteDistribution(newValues, newProbs);
}

/*****************************************************************
                         Conditionals
*****************************************************************/

LogicalExpression* IfThenElseExpression::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    LogicalExpression* newCondition = condition->replaceQuantifier(replacements, instantiator);
    LogicalExpression* newValueIfTrue = valueIfTrue->replaceQuantifier(replacements, instantiator);
    LogicalExpression* newValueIfFalse = valueIfFalse->replaceQuantifier(replacements, instantiator);
    return new IfThenElseExpression(newCondition, newValueIfTrue, newValueIfFalse);
}

LogicalExpression* MultiConditionChecker::replaceQuantifier(map<string, Object*>& replacements, Instantiator* instantiator) {
    std::vector<LogicalExpression*> newConditions;
    std::vector<LogicalExpression*> newEffects;

    for(unsigned int i = 0; i < conditions.size(); ++i) {
        newConditions.push_back(conditions[i]->replaceQuantifier(replacements, instantiator));
        newEffects.push_back(effects[i]->replaceQuantifier(replacements, instantiator));
    }
    return new MultiConditionChecker(newConditions, newEffects);
}
