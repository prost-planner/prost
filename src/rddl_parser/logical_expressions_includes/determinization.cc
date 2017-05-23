LogicalExpression* LogicalExpression::determinizeMostLikely(
    std::vector<ActionState> const& /*actionStates*/) {
    assert(false);
    return nullptr;
}

/*****************************************************************
                           Atomics
*****************************************************************/

LogicalExpression* ParametrizedVariable::determinizeMostLikely(
    std::vector<ActionState> const& /*actionStates*/) {
    return this;
}

LogicalExpression* NumericConstant::determinizeMostLikely(
    std::vector<ActionState> const& /*actionStates*/) {
    return this;
}

/*****************************************************************
                           Connectives
*****************************************************************/

LogicalExpression* Conjunction::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr =
            exprs[i]->determinizeMostLikely(actionStates);
        newExprs.push_back(newExpr);
    }
    return new Conjunction(newExprs);
}

LogicalExpression* Disjunction::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr =
            exprs[i]->determinizeMostLikely(actionStates);
        newExprs.push_back(newExpr);
    }
    return new Disjunction(newExprs);
}

LogicalExpression* EqualsExpression::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr =
            exprs[i]->determinizeMostLikely(actionStates);
        newExprs.push_back(newExpr);
    }
    return new EqualsExpression(newExprs);
}

LogicalExpression* GreaterExpression::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr =
            exprs[i]->determinizeMostLikely(actionStates);
        newExprs.push_back(newExpr);
    }
    return new GreaterExpression(newExprs);
}

LogicalExpression* LowerExpression::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr =
            exprs[i]->determinizeMostLikely(actionStates);
        newExprs.push_back(newExpr);
    }
    return new LowerExpression(newExprs);
}

LogicalExpression* GreaterEqualsExpression::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr =
            exprs[i]->determinizeMostLikely(actionStates);
        newExprs.push_back(newExpr);
    }
    return new GreaterEqualsExpression(newExprs);
}

LogicalExpression* LowerEqualsExpression::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr =
            exprs[i]->determinizeMostLikely(actionStates);
        newExprs.push_back(newExpr);
    }
    return new LowerEqualsExpression(newExprs);
}

LogicalExpression* Addition::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr =
            exprs[i]->determinizeMostLikely(actionStates);
        newExprs.push_back(newExpr);
    }
    return new Addition(newExprs);
}

LogicalExpression* Subtraction::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr =
            exprs[i]->determinizeMostLikely(actionStates);
        newExprs.push_back(newExpr);
    }
    return new Subtraction(newExprs);
}

LogicalExpression* Multiplication::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr =
            exprs[i]->determinizeMostLikely(actionStates);
        newExprs.push_back(newExpr);
    }
    return new Multiplication(newExprs);
}

LogicalExpression* Division::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    vector<LogicalExpression*> newExprs;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr =
            exprs[i]->determinizeMostLikely(actionStates);
        newExprs.push_back(newExpr);
    }
    return new Division(newExprs);
}

/*****************************************************************
                          Unaries
*****************************************************************/

LogicalExpression* Negation::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    LogicalExpression* newExpr = expr->determinizeMostLikely(actionStates);
    return new Negation(newExpr);
}

LogicalExpression* ExponentialFunction::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    LogicalExpression* newExpr = expr->determinizeMostLikely(actionStates);
    return new ExponentialFunction(newExpr);
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

LogicalExpression* BernoulliDistribution::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    LogicalExpression* newExpr = expr->determinizeMostLikely(actionStates);

    vector<LogicalExpression*> newExprs;
    newExprs.push_back(new NumericConstant(0.5));
    newExprs.push_back(newExpr);

    return new LowerEqualsExpression(newExprs);
}

LogicalExpression* DiscreteDistribution::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    // print(std::cout);
    // std::cout << std::endl;
    // Determinize each discrete probability only once:
    vector<LogicalExpression*> determinizedProbs;
    vector<double> upperBounds;
    double maxLowerBound = -numeric_limits<double>::max();
    for (LogicalExpression* expr : probabilities) {
        // Determinize all probabilities
        determinizedProbs.push_back(expr->determinizeMostLikely(actionStates));
        // Compute the bounds for all determinized probabilities
        double lower = numeric_limits<double>::max();
        double upper = -numeric_limits<double>::max();
        determinizedProbs.back()->determineBounds(actionStates, lower, upper);
        maxLowerBound = std::max(maxLowerBound, lower);
        upperBounds.push_back(upper);
    }

    // Determinization of a discrete distribution returns the value which
    // corresponds to the highest probability. Therefore this returns a multi
    // condition checker, where for each value a condition checks if
    // the probability is higher than ALL probabilities of other values
    vector<LogicalExpression*> conditions;
    vector<LogicalExpression*> effects;
    Simplifications dummy;
    for (size_t i = 0; i < probabilities.size(); ++i) {
        // Do not consider conditions which maxRes is smaller than another
        // conditions minRes (their conjunction can never evaluate to true)
        if (upperBounds[i] < maxLowerBound) {
            continue;
        }
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

LogicalExpression* IfThenElseExpression::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    LogicalExpression* newCondition =
        condition->determinizeMostLikely(actionStates);
    LogicalExpression* newValueIfTrue =
        valueIfTrue->determinizeMostLikely(actionStates);
    LogicalExpression* newValueIfFalse =
        valueIfFalse->determinizeMostLikely(actionStates);
    return new IfThenElseExpression(newCondition, newValueIfTrue,
                                    newValueIfFalse);
}

LogicalExpression* MultiConditionChecker::determinizeMostLikely(
    std::vector<ActionState> const& actionStates) {
    vector<LogicalExpression*> newConds;
    vector<LogicalExpression*> newEffs;
    for (unsigned int i = 0; i < conditions.size(); ++i) {
        LogicalExpression* newCond =
            conditions[i]->determinizeMostLikely(actionStates);
        LogicalExpression* newEff =
            effects[i]->determinizeMostLikely(actionStates);
        newConds.push_back(newCond);
        newEffs.push_back(newEff);
    }
    return new MultiConditionChecker(newConds, newEffs);
}
