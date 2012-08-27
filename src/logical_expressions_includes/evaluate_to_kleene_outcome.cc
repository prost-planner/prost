void LogicalExpression::evaluateToKleeneOutcome(double& /*res*/, State const& /*current*/, State const& /*next*/, ActionState const& /*actions*/) {
    assert(false);
}

void StateFluent::evaluateToKleeneOutcome(double& res, State const& current, State const& /*next*/, ActionState const& /*actions*/) {
    res = current[index];
}

void ActionFluent::evaluateToKleeneOutcome(double& res, State const& /*current*/, State const& /*next*/, ActionState const& actions) {
    res = actions[index];
}

void NumericConstant::evaluateToKleeneOutcome(double& res, State const& /*current*/, State const& /*next*/, ActionState const& /*actions*/) {
    res = value;
}

void Conjunction::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    res = 1.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->evaluateToKleeneOutcome(exprRes, current, next, actions);
        if(MathUtils::doubleIsEqual(exprRes, 0.0)) {
            res = 0.0;
            return;
        } else if(MathUtils::doubleIsMinusInfinity(exprRes)) {
            res = -numeric_limits<double>::max();
        }
    }
}

void Disjunction::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    res = 0.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->evaluateToKleeneOutcome(exprRes, current, next, actions);
        if(MathUtils::doubleIsEqual(exprRes, 1.0)) {
            res = 1.0;
            return;
        } else if(MathUtils::doubleIsMinusInfinity(exprRes)) {
            res = -numeric_limits<double>::max();
        }
    }
}

void EqualsExpression::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    exprs[0]->evaluateToKleeneOutcome(res, current, next, actions);

    if(MathUtils::doubleIsMinusInfinity(res)) {
        //res is already -infty
        return;
    }

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        exprs[i]->evaluateToKleeneOutcome(exprRes, current, next, actions);
        if(MathUtils::doubleIsMinusInfinity(exprRes)) {
            res = -numeric_limits<double>::max();
            return;
        } else if(!MathUtils::doubleIsEqual(exprRes, res)) {
            res = 0.0;
            return;
        }
    }
    res = 1.0;
}

void GreaterExpression::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    assert(exprs.size() == 2);
    exprs[0]->evaluateToKleeneOutcome(res, current, next, actions);
    exprs[1]->evaluateToKleeneOutcome(exprRes, current, next, actions);

    if(MathUtils::doubleIsMinusInfinity(res) || MathUtils::doubleIsMinusInfinity(exprRes)) {
        res = -numeric_limits<double>::max();
    } else {
        res = MathUtils::doubleIsGreater(res,exprRes);
    }
}

void LowerExpression::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    assert(exprs.size() == 2);
    exprs[0]->evaluateToKleeneOutcome(res, current, next, actions);
    exprs[1]->evaluateToKleeneOutcome(exprRes, current, next, actions);

    if(MathUtils::doubleIsMinusInfinity(res) || MathUtils::doubleIsMinusInfinity(exprRes)) {
        res = -numeric_limits<double>::max();
    } else {
        res = MathUtils::doubleIsSmaller(res,exprRes);
    }
}

void GreaterEqualsExpression::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    assert(exprs.size() == 2);
    exprs[0]->evaluateToKleeneOutcome(res, current, next, actions);
    exprs[1]->evaluateToKleeneOutcome(exprRes, current, next, actions);

    if(MathUtils::doubleIsMinusInfinity(res) || MathUtils::doubleIsMinusInfinity(exprRes)) {
        res = -numeric_limits<double>::max();
    } else {
        res = MathUtils::doubleIsGreaterOrEqual(res,exprRes);
    }
}

void LowerEqualsExpression::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    assert(exprs.size() == 2);
    exprs[0]->evaluateToKleeneOutcome(res, current, next, actions);
    exprs[1]->evaluateToKleeneOutcome(exprRes, current, next, actions);

    if(MathUtils::doubleIsMinusInfinity(res) || MathUtils::doubleIsMinusInfinity(exprRes)) {
        res = -numeric_limits<double>::max();
    } else {
        res = MathUtils::doubleIsSmallerOrEqual(res,exprRes);
    }
}

void Addition::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    res = 0.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->evaluateToKleeneOutcome(exprRes, current, next, actions);
        if(MathUtils::doubleIsMinusInfinity(exprRes)) {
            res = -numeric_limits<double>::max();
            return;
        } else {
            res += exprRes;
        }
    }
}

void Subtraction::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    exprs[0]->evaluateToKleeneOutcome(res, current, next, actions);

    if(MathUtils::doubleIsMinusInfinity(res)) {
        //res is already -infty
        return;
    }

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        exprs[i]->evaluateToKleeneOutcome(exprRes, current, next, actions);

        if(MathUtils::doubleIsMinusInfinity(exprRes)) {
            res = -numeric_limits<double>::max();
            return;
        } else {
            res -= exprRes;
        }
    }
}

void Multiplication::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    res = 1.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->evaluateToKleeneOutcome(exprRes, current, next, actions);

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

void Division::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    exprs[0]->evaluateToKleeneOutcome(res, current, next, actions);

    if(MathUtils::doubleIsMinusInfinity(res)) {
        //res is already -infty
        return;
    } else if(MathUtils::doubleIsEqual(res, 0.0)) {
        //res is already 0.0
        return;
    }

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        exprs[i]->evaluateToKleeneOutcome(exprRes, current, next, actions);

        if(MathUtils::doubleIsMinusInfinity(exprRes)) {
            res = -numeric_limits<double>::max();
            return;
        } else {
            res /= exprRes;
        }
    }
}

void BernoulliDistribution::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    expr->evaluateToKleeneOutcome(res, current, next, actions);
    if(!MathUtils::doubleIsEqual(res,1.0) && !MathUtils::doubleIsEqual(res,0.0)) {
        res = -numeric_limits<double>::max();
    }
}

void IfThenElseExpression::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    condition->evaluateToKleeneOutcome(res, current, next, actions);

    if(MathUtils::doubleIsMinusInfinity(res)) {
        //the condition could be true or false -> we evaluate both, and check if they are equal
        valueIfFalse->evaluateToKleeneOutcome(res, current, next, actions);
        valueIfTrue->evaluateToKleeneOutcome(exprRes, current, next, actions);
        if(!MathUtils::doubleIsEqual(res,exprRes)) {
            res = -numeric_limits<double>::max();
        }
    } else {
        //otherwise we only evaluate the corresponding log expr
        if(MathUtils::doubleIsEqual(res,0.0)) {
            valueIfFalse->evaluateToKleeneOutcome(res, current, next, actions);
        } else {
            valueIfTrue->evaluateToKleeneOutcome(res, current, next, actions);
        }
    }
}

void MultiConditionChecker::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    //if we meet a condition that evaluates to unknown we must keep on checking conditions until we find one
    //that is always true, and compare all potentially true ones with each other
    hasUncertainCondition = false;

    for(unsigned int i = 0; i < conditions.size(); ++i) {
        conditions[i]->evaluateToKleeneOutcome(res, current, next, actions);
        if(MathUtils::doubleIsEqual(res,1.0)) {
            effects[i]->evaluateToKleeneOutcome(res, current, next, actions);
            if(hasUncertainCondition && !MathUtils::doubleIsEqual(res,exprRes)) {
                res = -numeric_limits<double>::max();
            }
            return;
        } else if(MathUtils::doubleIsMinusInfinity(res)) {
            effects[i]->evaluateToKleeneOutcome(res, current, next, actions);

            if(MathUtils::doubleIsMinusInfinity(res)) {
                return;
            } else if(hasUncertainCondition && !MathUtils::doubleIsEqual(res,exprRes)) {
                res = -numeric_limits<double>::max();
                return;
            }
            hasUncertainCondition = true;
            exprRes = res;
        }
    }
    assert(false);
}

void NegateExpression::evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
    expr->evaluateToKleeneOutcome(res, current, next, actions);
    if(MathUtils::doubleIsEqual(res, 0.0)) {
        res = 1.0;
    } else if(MathUtils::doubleIsEqual(res, 1.0)) {
        res = 0.0;
    }
}
