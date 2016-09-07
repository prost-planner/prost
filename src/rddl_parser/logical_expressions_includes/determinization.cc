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
    int highestIndex = 0;
    double highestProb = 0.0;

    for (unsigned int i = 0; i < probabilities.size(); ++i) {
        LogicalExpression* prob = probabilities[i]->determinizeMostLikely();
        NumericConstant* nc = dynamic_cast<NumericConstant*>(prob);
        // Determinization with conditional probabilities is a lot harder so we
        // exclude it for now. (TODO!!!)
        if (!nc) {
            SystemUtils::abort(
                "NOT SUPPORTED: Discrete statement with conditional "
                "probability detected.");
        }

        if (MathUtils::doubleIsGreater(nc->value, highestProb)) {
            highestProb = nc->value;
            highestIndex = i;
        }
    }
    return values[highestIndex];
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
