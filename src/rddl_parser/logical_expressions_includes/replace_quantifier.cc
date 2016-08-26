LogicalExpression* LogicalExpression::replaceQuantifier(
    Instantiations& /*replace*/, Instantiator* /*inst*/) {
    print(cout);
    assert(false);
    return nullptr;
}

/*****************************************************************
                         Schematics
*****************************************************************/

LogicalExpression* ParametrizedVariable::replaceQuantifier(
    Instantiations& replace, Instantiator* inst) {
    vector<Parameter*> newParams;
    for (unsigned int i = 0; i < params.size(); ++i) {
        Parameter* param = dynamic_cast<Parameter*>(
            params[i]->replaceQuantifier(replace, inst));
        assert(param);
        newParams.push_back(param);
    }
    return new ParametrizedVariable(*this, newParams);
}

/*****************************************************************
                           Atomics
*****************************************************************/

LogicalExpression* NumericConstant::replaceQuantifier(
    Instantiations& /*replace*/, Instantiator* /*inst*/) {
    return this;
}

LogicalExpression* Parameter::replaceQuantifier(Instantiations& replace,
                                                Instantiator* /*inst*/) {
    if (replace.find(name) != replace.end()) {
        return replace[name];
    }
    return this;
}

LogicalExpression* Object::replaceQuantifier(Instantiations& /*replace*/,
                                             Instantiator* /*inst*/) {
    return this;
}

/*****************************************************************
                          Quantifier
*****************************************************************/

void Quantifier::getReplacements(vector<string>& parameterNames,
                                 vector<vector<Parameter*>>& replace,
                                 Instantiator* inst) {
    vector<Parameter*> parameterTypes;
    for (unsigned int i = 0; i < paramList->params.size(); ++i) {
        parameterNames.push_back(paramList->params[i]->name);
        Parameter* param =
            new Parameter(paramList->params[i]->name, paramList->types[i]);
        parameterTypes.push_back(param);
    }
    inst->instantiateParams(parameterTypes, replace);
}

LogicalExpression* Sumation::replaceQuantifier(Instantiations& replace,
                                               Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    vector<string> parameterNames;
    vector<vector<Parameter*>> newParams;
    getReplacements(parameterNames, newParams, inst);

    for (unsigned int i = 0; i < newParams.size(); ++i) {
        assert(newParams[i].size() == parameterNames.size());

        Instantiations copy = replace;
        for (unsigned int j = 0; j < newParams[i].size(); ++j) {
            assert(copy.find(parameterNames[j]) == copy.end());
            Object* obj = dynamic_cast<Object*>(newParams[i][j]);
            assert(obj);
            copy[parameterNames[j]] = obj;
        }
        LogicalExpression* e = expr->replaceQuantifier(copy, inst);
        newExprs.push_back(e);
    }
    return new Addition(newExprs);
}

LogicalExpression* Product::replaceQuantifier(Instantiations& replace,
                                              Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    vector<string> parameterNames;
    vector<vector<Parameter*>> newParams;
    getReplacements(parameterNames, newParams, inst);

    for (unsigned int i = 0; i < newParams.size(); ++i) {
        assert(newParams[i].size() == parameterNames.size());

        Instantiations copy = replace;
        for (unsigned int j = 0; j < newParams[i].size(); ++j) {
            assert(copy.find(parameterNames[j]) == copy.end());
            Object* obj = dynamic_cast<Object*>(newParams[i][j]);
            assert(obj);
            copy[parameterNames[j]] = obj;
        }
        LogicalExpression* e = expr->replaceQuantifier(copy, inst);
        newExprs.push_back(e);
    }
    return new Multiplication(newExprs);
}

LogicalExpression* UniversalQuantification::replaceQuantifier(
    Instantiations& replace, Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    vector<string> parameterNames;
    vector<vector<Parameter*>> newParams;
    getReplacements(parameterNames, newParams, inst);

    for (unsigned int i = 0; i < newParams.size(); ++i) {
        assert(newParams[i].size() == parameterNames.size());

        Instantiations copy(replace);
        for (unsigned int j = 0; j < newParams[i].size(); ++j) {
            assert(copy.find(parameterNames[j]) == copy.end());
            Object* obj = dynamic_cast<Object*>(newParams[i][j]);
            assert(obj);
            copy[parameterNames[j]] = obj;
        }
        LogicalExpression* e = expr->replaceQuantifier(copy, inst);
        newExprs.push_back(e);
    }
    return new Conjunction(newExprs);
}

LogicalExpression* ExistentialQuantification::replaceQuantifier(
    Instantiations& replace, Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    vector<string> parameterNames;
    vector<vector<Parameter*>> newParams;
    getReplacements(parameterNames, newParams, inst);

    for (unsigned int i = 0; i < newParams.size(); ++i) {
        assert(newParams[i].size() == parameterNames.size());

        Instantiations copy = replace;
        for (unsigned int j = 0; j < newParams[i].size(); ++j) {
            assert(copy.find(parameterNames[j]) == copy.end());
            Object* obj = dynamic_cast<Object*>(newParams[i][j]);
            assert(obj);
            copy[parameterNames[j]] = obj;
        }
        LogicalExpression* e = expr->replaceQuantifier(copy, inst);
        newExprs.push_back(e);
    }
    return new Disjunction(newExprs);
}

/*****************************************************************
                           Connectives
*****************************************************************/

LogicalExpression* Conjunction::replaceQuantifier(Instantiations& replace,
                                                  Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExprs.push_back(exprs[i]->replaceQuantifier(replace, inst));
    }
    return new Conjunction(newExprs);
}

LogicalExpression* Disjunction::replaceQuantifier(Instantiations& replace,
                                                  Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExprs.push_back(exprs[i]->replaceQuantifier(replace, inst));
    }
    return new Disjunction(newExprs);
}

LogicalExpression* EqualsExpression::replaceQuantifier(Instantiations& replace,
                                                       Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExprs.push_back(exprs[i]->replaceQuantifier(replace, inst));
    }
    return new EqualsExpression(newExprs);
}

LogicalExpression* GreaterExpression::replaceQuantifier(Instantiations& replace,
                                                        Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExprs.push_back(exprs[i]->replaceQuantifier(replace, inst));
    }
    return new GreaterExpression(newExprs);
}

LogicalExpression* LowerExpression::replaceQuantifier(Instantiations& replace,
                                                      Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExprs.push_back(exprs[i]->replaceQuantifier(replace, inst));
    }
    return new LowerExpression(newExprs);
}

LogicalExpression* GreaterEqualsExpression::replaceQuantifier(
    Instantiations& replace, Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExprs.push_back(exprs[i]->replaceQuantifier(replace, inst));
    }
    return new GreaterEqualsExpression(newExprs);
}

LogicalExpression* LowerEqualsExpression::replaceQuantifier(
    Instantiations& replace, Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExprs.push_back(exprs[i]->replaceQuantifier(replace, inst));
    }
    return new LowerEqualsExpression(newExprs);
}

LogicalExpression* Addition::replaceQuantifier(Instantiations& replace,
                                               Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExprs.push_back(exprs[i]->replaceQuantifier(replace, inst));
    }
    return new Addition(newExprs);
}

LogicalExpression* Subtraction::replaceQuantifier(Instantiations& replace,
                                                  Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExprs.push_back(exprs[i]->replaceQuantifier(replace, inst));
    }
    return new Subtraction(newExprs);
}

LogicalExpression* Multiplication::replaceQuantifier(Instantiations& replace,
                                                     Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExprs.push_back(exprs[i]->replaceQuantifier(replace, inst));
    }
    return new Multiplication(newExprs);
}

LogicalExpression* Division::replaceQuantifier(Instantiations& replace,
                                               Instantiator* inst) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        newExprs.push_back(exprs[i]->replaceQuantifier(replace, inst));
    }
    return new Division(newExprs);
}

/*****************************************************************
                          Unaries
*****************************************************************/

LogicalExpression* Negation::replaceQuantifier(Instantiations& replace,
                                               Instantiator* inst) {
    return new Negation(expr->replaceQuantifier(replace, inst));
}

LogicalExpression* ExponentialFunction::replaceQuantifier(
    Instantiations& replace, Instantiator* inst) {
    return new ExponentialFunction(expr->replaceQuantifier(replace, inst));
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

LogicalExpression* KronDeltaDistribution::replaceQuantifier(
    Instantiations& replace, Instantiator* inst) {
    return new KronDeltaDistribution(expr->replaceQuantifier(replace, inst));
}

LogicalExpression* BernoulliDistribution::replaceQuantifier(
    Instantiations& replace, Instantiator* inst) {
    return new BernoulliDistribution(expr->replaceQuantifier(replace, inst));
}

LogicalExpression* DiscreteDistribution::replaceQuantifier(
    Instantiations& replace, Instantiator* inst) {
    vector<LogicalExpression*> newValues;
    vector<LogicalExpression*> newProbs;

    for (unsigned int i = 0; i < values.size(); ++i) {
        newValues.push_back(values[i]->replaceQuantifier(replace, inst));
        newProbs.push_back(probabilities[i]->replaceQuantifier(replace, inst));
    }
    return new DiscreteDistribution(newValues, newProbs);
}

/*****************************************************************
                         Conditionals
*****************************************************************/

LogicalExpression* IfThenElseExpression::replaceQuantifier(
    Instantiations& replace, Instantiator* inst) {
    LogicalExpression* newCondition =
        condition->replaceQuantifier(replace, inst);
    LogicalExpression* newValueIfTrue =
        valueIfTrue->replaceQuantifier(replace, inst);
    LogicalExpression* newValueIfFalse =
        valueIfFalse->replaceQuantifier(replace, inst);
    return new IfThenElseExpression(newCondition, newValueIfTrue,
                                    newValueIfFalse);
}

LogicalExpression* MultiConditionChecker::replaceQuantifier(
    Instantiations& replace, Instantiator* inst) {
    std::vector<LogicalExpression*> newConditions;
    std::vector<LogicalExpression*> newEffects;

    for (unsigned int i = 0; i < conditions.size(); ++i) {
        newConditions.push_back(
            conditions[i]->replaceQuantifier(replace, inst));
        newEffects.push_back(effects[i]->replaceQuantifier(replace, inst));
    }
    return new MultiConditionChecker(newConditions, newEffects);
}
