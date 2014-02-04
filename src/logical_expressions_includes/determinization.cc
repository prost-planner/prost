LogicalExpression* LogicalExpression::determinizeMostLikely(NumericConstant* /*randomNumberReplacement*/) {
    assert(false);
    return NULL;
}

/*****************************************************************
                           Atomics
*****************************************************************/

LogicalExpression* AtomicLogicalExpression::determinizeMostLikely(NumericConstant* /*randomNumberReplacement*/) {
    return this;
}

LogicalExpression* NumericConstant::determinizeMostLikely(NumericConstant* /*randomNumberReplacement*/) {
    return this;
}

/*****************************************************************
                           Connectives
*****************************************************************/

LogicalExpression* Conjunction::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely(randomNumberReplacement);
        newExprs.push_back(newExpr);
    }
    return new Conjunction(newExprs);
}

LogicalExpression* Disjunction::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely(randomNumberReplacement);
        newExprs.push_back(newExpr);
    }
    return new Disjunction(newExprs);
}

LogicalExpression* EqualsExpression::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely(randomNumberReplacement);
        newExprs.push_back(newExpr);
    }
    return new EqualsExpression(newExprs);
}

LogicalExpression* GreaterExpression::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely(randomNumberReplacement);
        newExprs.push_back(newExpr);
    }
    return new GreaterExpression(newExprs);
}

LogicalExpression* LowerExpression::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely(randomNumberReplacement);
        newExprs.push_back(newExpr);
    }
    return new LowerExpression(newExprs);
}

LogicalExpression* GreaterEqualsExpression::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely(randomNumberReplacement);
        newExprs.push_back(newExpr);
    }
    return new GreaterEqualsExpression(newExprs);
}

LogicalExpression* LowerEqualsExpression::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely(randomNumberReplacement);
        newExprs.push_back(newExpr);
    }
    return new LowerEqualsExpression(newExprs);
}

LogicalExpression* Addition::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely(randomNumberReplacement);
        newExprs.push_back(newExpr);
    }
    return new Addition(newExprs);
}

LogicalExpression* Subtraction::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely(randomNumberReplacement);
        newExprs.push_back(newExpr);
    }
    return new Subtraction(newExprs);
}

LogicalExpression* Multiplication::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely(randomNumberReplacement);
        newExprs.push_back(newExpr);
    }
    return new Multiplication(newExprs);
}

LogicalExpression* Division::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    vector<LogicalExpression*> newExprs;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        LogicalExpression* newExpr = exprs[i]->determinizeMostLikely(randomNumberReplacement);
        newExprs.push_back(newExpr);
    }
    return new Division(newExprs);
}

/*****************************************************************
                          Unaries
*****************************************************************/

LogicalExpression* NegateExpression::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    LogicalExpression* newExpr = expr->determinizeMostLikely(randomNumberReplacement);
    return new NegateExpression(newExpr);
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

LogicalExpression* BernoulliDistribution::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    LogicalExpression* newExpr = expr->determinizeMostLikely(randomNumberReplacement);

    vector<LogicalExpression*> newExprs;
    newExprs.push_back(randomNumberReplacement);
    newExprs.push_back(newExpr);

    return new LowerEqualsExpression(newExprs);
}

LogicalExpression* DiscreteDistribution::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    int highestIndex = 0;
    double highestProb = 0.0;

    for(unsigned int i = 0; i < probabilities.size(); ++i) {
        LogicalExpression* prob = probabilities[i]->determinizeMostLikely(randomNumberReplacement);
        NumericConstant* nc = dynamic_cast<NumericConstant*>(prob);
        // Determinization with conditional probabilities is a lot harder so we
        // exclude it for now. (TODO!!!)
        if(!nc) {
            SystemUtils::abort("NOT SUPPORTED: Discrete statement with conditional probability detected.");
        }

        if(MathUtils::doubleIsGreater(nc->value, highestProb)) {
            highestProb = nc->value;
            highestIndex = i;
        }
    }
    return values[highestIndex];
}

/*****************************************************************
                         Conditionals
*****************************************************************/

LogicalExpression* IfThenElseExpression::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    LogicalExpression* newCondition = condition->determinizeMostLikely(randomNumberReplacement);
    LogicalExpression* newValueIfTrue = valueIfTrue->determinizeMostLikely(randomNumberReplacement);
    LogicalExpression* newValueIfFalse = valueIfFalse->determinizeMostLikely(randomNumberReplacement);
    return new IfThenElseExpression(newCondition, newValueIfTrue, newValueIfFalse);
}

LogicalExpression* MultiConditionChecker::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    vector<LogicalExpression*> newConds;
    vector<LogicalExpression*> newEffs;
    for(unsigned int i = 0; i < conditions.size(); ++i) {
        LogicalExpression* newCond = conditions[i]->determinizeMostLikely(randomNumberReplacement);
        LogicalExpression* newEff = effects[i]->determinizeMostLikely(randomNumberReplacement);
        newConds.push_back(newCond);
        newEffs.push_back(newEff);
    }
    return new MultiConditionChecker(newConds, newEffs);
}



