void LogicalExpression::evaluateToKleeneOutcome(double& /*res*/, State const& /*current*/, ActionState const& /*actions*/) {
    assert(false);
}

/*****************************************************************
                           Atomics
*****************************************************************/

void StateFluent::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& /*actions*/) {
    res = current[index];
}

void ActionFluent::evaluateToKleeneOutcome(double& res, State const& /*current*/, ActionState const& actions) {
    res = actions[index];
}

void NumericConstant::evaluateToKleeneOutcome(double& res, State const& /*current*/, ActionState const& /*actions*/) {
    res = value;
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Conjunction::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    res = 1.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        double exprRes = 0.0;
        exprs[i]->evaluateToKleeneOutcome(exprRes, current, actions);
        if(MathUtils::doubleIsEqual(exprRes, 0.0)) {
            res = 0.0;
            return;
        } else if(MathUtils::doubleIsMinusInfinity(exprRes)) {
            res = -numeric_limits<double>::max();
        }
    }
}

void Disjunction::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    res = 0.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        double exprRes = 0.0;
        exprs[i]->evaluateToKleeneOutcome(exprRes, current, actions);
        if(MathUtils::doubleIsEqual(exprRes, 1.0)) {
            res = 1.0;
            return;
        } else if(MathUtils::doubleIsMinusInfinity(exprRes)) {
            res = -numeric_limits<double>::max();
        }
    }
}

void EqualsExpression::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    assert(exprs.size() == 2);
    double lhs = 0.0;
    exprs[0]->evaluateToKleeneOutcome(lhs, current, actions);
    if(MathUtils::doubleIsMinusInfinity(lhs)) {
        res = -numeric_limits<double>::max();
        return;
    }
    double rhs = 0.0;
    exprs[1]->evaluateToKleeneOutcome(rhs, current, actions);
    if(MathUtils::doubleIsMinusInfinity(rhs)) {
        res = -numeric_limits<double>::max();
        return;
    }
    res = MathUtils::doubleIsEqual(lhs, rhs);
}

void GreaterExpression::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    assert(exprs.size() == 2);
    double lhs = 0.0;
    exprs[0]->evaluateToKleeneOutcome(lhs, current, actions);
    if(MathUtils::doubleIsMinusInfinity(lhs)) {
        res = -numeric_limits<double>::max();
        return;
    }
    double rhs = 0.0;
    exprs[1]->evaluateToKleeneOutcome(rhs, current, actions);
    if(MathUtils::doubleIsMinusInfinity(rhs)) {
        res = -numeric_limits<double>::max();
        return;
    }
    res = MathUtils::doubleIsGreater(lhs, rhs);
}

void LowerExpression::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    assert(exprs.size() == 2);
    double lhs = 0.0;
    exprs[0]->evaluateToKleeneOutcome(lhs, current, actions);
    if(MathUtils::doubleIsMinusInfinity(lhs)) {
        res = -numeric_limits<double>::max();
        return;
    }
    double rhs = 0.0;
    exprs[1]->evaluateToKleeneOutcome(rhs, current, actions);
    if(MathUtils::doubleIsMinusInfinity(rhs)) {
        res = -numeric_limits<double>::max();
        return;
    }
    res = MathUtils::doubleIsSmaller(lhs, rhs);
}

void GreaterEqualsExpression::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    assert(exprs.size() == 2);
    double lhs = 0.0;
    exprs[0]->evaluateToKleeneOutcome(lhs, current, actions);
    if(MathUtils::doubleIsMinusInfinity(lhs)) {
        res = -numeric_limits<double>::max();
        return;
    }
    double rhs = 0.0;
    exprs[1]->evaluateToKleeneOutcome(rhs, current, actions);
    if(MathUtils::doubleIsMinusInfinity(rhs)) {
        res = -numeric_limits<double>::max();
        return;
    }
    res = MathUtils::doubleIsGreaterOrEqual(lhs, rhs);
}

void LowerEqualsExpression::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    assert(exprs.size() == 2);
    double lhs = 0.0;
    exprs[0]->evaluateToKleeneOutcome(lhs, current, actions);
    if(MathUtils::doubleIsMinusInfinity(lhs)) {
        res = -numeric_limits<double>::max();
        return;
    }
    double rhs = 0.0;
    exprs[1]->evaluateToKleeneOutcome(rhs, current, actions);
    if(MathUtils::doubleIsMinusInfinity(rhs)) {
        res = -numeric_limits<double>::max();
        return;
    }
    res = MathUtils::doubleIsSmallerOrEqual(lhs, rhs);
}

void Addition::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    res = 0.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        double exprRes = 0.0;
        exprs[i]->evaluateToKleeneOutcome(exprRes, current, actions);
        if(MathUtils::doubleIsMinusInfinity(exprRes)) {
            res = -numeric_limits<double>::max();
            return;
        } else {
            res += exprRes;
        }
    }
}

void Subtraction::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    exprs[0]->evaluateToKleeneOutcome(res, current, actions);

    if(MathUtils::doubleIsMinusInfinity(res)) {
        //res is already -infty
        return;
    }

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        double exprRes = 0.0;
        exprs[i]->evaluateToKleeneOutcome(exprRes, current, actions);

        if(MathUtils::doubleIsMinusInfinity(exprRes)) {
            res = -numeric_limits<double>::max();
            return;
        } else {
            res -= exprRes;
        }
    }
}

void Multiplication::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    res = 1.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        double exprRes = 0.0;
        exprs[i]->evaluateToKleeneOutcome(exprRes, current, actions);

        if(MathUtils::doubleIsEqual(exprRes, 0.0)) {
            res = 0.0;
            return;
        } else if (MathUtils::doubleIsMinusInfinity(res) || MathUtils::doubleIsMinusInfinity(exprRes)) {
            res = -numeric_limits<double>::max();
        } else {
            res *= exprRes;
        }
    }
}

void Division::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    exprs[0]->evaluateToKleeneOutcome(res, current, actions);

    if(MathUtils::doubleIsMinusInfinity(res)) {
        //res is already -infty
        return;
    } else if(MathUtils::doubleIsEqual(res, 0.0)) {
        //res is already 0.0
        return;
    }

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        double exprRes = 0.0;
        exprs[i]->evaluateToKleeneOutcome(exprRes, current, actions);

        if(MathUtils::doubleIsMinusInfinity(exprRes)) {
            res = -numeric_limits<double>::max();
            return;
        } else {
            res /= exprRes;
        }
    }
}

/*****************************************************************
                          Unaries
*****************************************************************/

void NegateExpression::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    expr->evaluateToKleeneOutcome(res, current, actions);
    if(!MathUtils::doubleIsMinusInfinity(res)) {
        res = MathUtils::doubleIsEqual(res, 0.0);
    }
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

void BernoulliDistribution::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    expr->evaluateToKleeneOutcome(res, current, actions);
    if(!MathUtils::doubleIsEqual(res,1.0) && !MathUtils::doubleIsEqual(res,0.0)) {
        res = -numeric_limits<double>::max();
    }
}

void DiscreteDistribution::evaluateToKleeneOutcome(double& /*res*/, State const& /*current*/, ActionState const& /*actions*/) {
}

/*****************************************************************
                         Conditionals
*****************************************************************/

void IfThenElseExpression::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    condition->evaluateToKleeneOutcome(res, current, actions);

    if(MathUtils::doubleIsMinusInfinity(res)) {
        //the condition could be true or false -> we evaluate both, and check if they are equal
        double falseRes = 0.0;
        valueIfFalse->evaluateToKleeneOutcome(falseRes, current, actions);
        double trueRes = 0.0;
        valueIfTrue->evaluateToKleeneOutcome(trueRes, current, actions);
        if(!MathUtils::doubleIsEqual(falseRes, trueRes)) {
            res = -numeric_limits<double>::max();
        }
    } else if(MathUtils::doubleIsEqual(res, 0.0)) {
        valueIfFalse->evaluateToKleeneOutcome(res, current, actions);
    } else {
        valueIfTrue->evaluateToKleeneOutcome(res, current, actions);
    }
}

void MultiConditionChecker::evaluateToKleeneOutcome(double& res, State const& current, ActionState const& actions) {
    //if we meet a condition that evaluates to unknown we must keep on checking conditions until we find one
    //that is always true, and compare all potentially true ones with each other
    hasUncertainCondition = false;
    double uncertainRes = 0.0;

    for(unsigned int i = 0; i < conditions.size(); ++i) {
        conditions[i]->evaluateToKleeneOutcome(res, current, actions);
        if(MathUtils::doubleIsEqual(res, 1.0)) {
            effects[i]->evaluateToKleeneOutcome(res, current, actions);
            if(hasUncertainCondition && !MathUtils::doubleIsEqual(res, uncertainRes)) {
                res = -numeric_limits<double>::max();
            }
            return;
        } else if(MathUtils::doubleIsMinusInfinity(res)) {
            effects[i]->evaluateToKleeneOutcome(res, current, actions);

            if(MathUtils::doubleIsMinusInfinity(res)) {
                return;
            } else if(hasUncertainCondition && !MathUtils::doubleIsEqual(res, uncertainRes)) {
                res = -numeric_limits<double>::max();
                return;
            }
            hasUncertainCondition = true;
            uncertainRes = res;
        }
    }
    assert(false);
}


