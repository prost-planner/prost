LogicalExpression* LogicalExpression::determinizeMostLikely() {
    assert(false);
    return nullptr;
}

/*****************************************************************
                           Atomics
*****************************************************************/

LogicalExpression* ParametrizedVariable::determinizeMostLikely() {
    return this;
}

LogicalExpression* NumericConstant::determinizeMostLikely() {
    return this;
}

/*****************************************************************
                           Connectives
*****************************************************************/

LogicalExpression* Conjunction::determinizeMostLikely() {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely();
        newExprs.push_back(newExpr);
    }
    return new Conjunction(newExprs);
}

LogicalExpression* Disjunction::determinizeMostLikely() {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely();
        newExprs.push_back(newExpr);
    }
    return new Disjunction(newExprs);
}

LogicalExpression* EqualsExpression::determinizeMostLikely() {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely();
        newExprs.push_back(newExpr);
    }
    return new EqualsExpression(newExprs);
}

LogicalExpression* GreaterExpression::determinizeMostLikely() {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely();
        newExprs.push_back(newExpr);
    }
    return new GreaterExpression(newExprs);
}

LogicalExpression* LowerExpression::determinizeMostLikely() {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely();
        newExprs.push_back(newExpr);
    }
    return new LowerExpression(newExprs);
}

LogicalExpression* GreaterEqualsExpression::determinizeMostLikely() {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely();
        newExprs.push_back(newExpr);
    }
    return new GreaterEqualsExpression(newExprs);
}

LogicalExpression* LowerEqualsExpression::determinizeMostLikely() {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely();
        newExprs.push_back(newExpr);
    }
    return new LowerEqualsExpression(newExprs);
}

LogicalExpression* Addition::determinizeMostLikely() {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely();
        newExprs.push_back(newExpr);
    }
    return new Addition(newExprs);
}

LogicalExpression* Subtraction::determinizeMostLikely() {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely();
        newExprs.push_back(newExpr);
    }
    return new Subtraction(newExprs);
}

LogicalExpression* Multiplication::determinizeMostLikely() {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely();
        newExprs.push_back(newExpr);
    }
    return new Multiplication(newExprs);
}

LogicalExpression* Division::determinizeMostLikely() {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely();
        newExprs.push_back(newExpr);
    }
    return new Division(newExprs);
}

/*****************************************************************
                          Unaries
*****************************************************************/

LogicalExpression* Negation::determinizeMostLikely() {
    LogicalExpression* newExpr = expr->determinizeMostLikely();
    return new Negation(newExpr);
}

LogicalExpression* ExponentialFunction::determinizeMostLikely() {
    LogicalExpression* newExpr = expr->determinizeMostLikely();
    return new ExponentialFunction(newExpr);
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

LogicalExpression* BernoulliDistribution::determinizeMostLikely() {
    LogicalExpression* newExpr = expr->determinizeMostLikely();

    vector<LogicalExpression*> newExprs;
    newExprs.push_back(new NumericConstant(0.5));
    newExprs.push_back(newExpr);

    return new LowerEqualsExpression(newExprs);
}

LogicalExpression* DiscreteDistribution::determinizeMostLikely() {
    // print(std::cout);
    // std::cout << std::endl;
    // Determinize each discrete probability only once:
    vector<LogicalExpression*> determinizedProbs;
    for (LogicalExpression* expr : probabilities) {
        determinizedProbs.push_back(expr->determinizeMostLikely());
    }

    // Determinization of a discrete distribution returns the value which
    // corresponds to the highest probability. Therefore this returns a multi
    // condition checker, where for each value a condition checks if
    // the probability is higher than ALL probabilities of other values
    vector<LogicalExpression*> conditions;
    vector<LogicalExpression*> effects;
    Simplifications dummy;
    for (size_t i = 0; i < probabilities.size(); ++i) {
        vector<LogicalExpression*> comparisons;
        for (size_t j = 0; j < probabilities.size(); ++j) {
            if (i == j) {
                continue;
            }
            vector<LogicalExpression*> compareParts{determinizedProbs[i],
                                                    determinizedProbs[j]};
            auto geq = new GreaterEqualsExpression(compareParts);
            comparisons.push_back(geq->simplify(dummy));
        }
        auto conjunction = new Conjunction(comparisons);
        conditions.push_back(conjunction->simplify(dummy));
        effects.push_back(values[i]);
    }
    LogicalExpression* mcc = new MultiConditionChecker(conditions, effects);
    mcc = mcc->simplify(dummy);
    // mcc->print(std::cout);
    // std::cout << std::endl;
    return mcc;
}

/*****************************************************************
                         Conditionals
*****************************************************************/

LogicalExpression* IfThenElseExpression::determinizeMostLikely() {
    LogicalExpression* newCondition = condition->determinizeMostLikely();
    LogicalExpression* newValueIfTrue = valueIfTrue->determinizeMostLikely();
    LogicalExpression* newValueIfFalse = valueIfFalse->determinizeMostLikely();
    return new IfThenElseExpression(newCondition, newValueIfTrue,
                                    newValueIfFalse);
}

LogicalExpression* MultiConditionChecker::determinizeMostLikely() {
    vector<LogicalExpression*> newConds;
    vector<LogicalExpression*> newEffs;
    for (unsigned int i = 0; i < conditions.size(); ++i) {
        LogicalExpression* newCond = conditions[i]->determinizeMostLikely();
        LogicalExpression* newEff = effects[i]->determinizeMostLikely();
        newConds.push_back(newCond);
        newEffs.push_back(newEff);
    }
    return new MultiConditionChecker(newConds, newEffs);
}
