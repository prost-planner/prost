void LogicalExpression::evaluate(double& /*res*/, State const& /*current*/, State const& /*next*/, ActionState const& /*actions*/) {
    assert(false);
}

void StateFluent::evaluate(double& res, State const& current, State const& /*next*/, ActionState const& /*actions*/) {
    res = current[index];
}

void ActionFluent::evaluate(double& res, State const& /*current*/, State const& /*next*/, ActionState const& actions) {
    res = actions[index];
}

void NumericConstant::evaluate(double& res, State const& /*current*/, State const& /*next*/, ActionState const& /*actions*/) {
    res = value;
}

void Conjunction::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    res = 1.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->evaluate(exprRes, current, next, actions);
        if(MathUtils::doubleIsEqual(exprRes, 0.0)) {
            res = 0.0;
            return;
        } else {
            res *= exprRes;
        }
    }
}

void Disjunction::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    res = 1.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->evaluate(exprRes, current, next, actions);
        if(MathUtils::doubleIsEqual(exprRes, 1.0)) {
            res = 1.0;
            return;
        } else {
            res *= (1.0 - exprRes);
        }
    }
    res = (1.0 - res);
}

//TODO: If these are probabilistic, compare them on a prob level (i.e. for ==, res = (res*exprRes) + ((1-res)*(1-exprRes)))
void EqualsExpression::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    exprs[0]->evaluate(res, current, next, actions);

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        exprs[i]->evaluate(exprRes, current, next, actions);
        if(!MathUtils::doubleIsEqual(exprRes, res)) {
            res = 0.0;
            return;
        }
    }
    res = 1.0;
}

void GreaterExpression::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    assert(exprs.size() == 2);
    exprs[0]->evaluate(res, current, next, actions);
    exprs[1]->evaluate(exprRes, current, next, actions);
    res = MathUtils::doubleIsGreater(res,exprRes);
}

void LowerExpression::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    assert(exprs.size() == 2);
    exprs[0]->evaluate(res, current, next, actions);
    exprs[1]->evaluate(exprRes, current, next, actions);
    res = MathUtils::doubleIsSmaller(res,exprRes);
}

void GreaterEqualsExpression::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    assert(exprs.size() == 2);
    exprs[0]->evaluate(res, current, next, actions);
    exprs[1]->evaluate(exprRes, current, next, actions);
    res = MathUtils::doubleIsGreaterOrEqual(res,exprRes);
}

void LowerEqualsExpression::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    assert(exprs.size() == 2);
    exprs[0]->evaluate(res, current, next, actions);
    exprs[1]->evaluate(exprRes, current, next, actions);
    res = MathUtils::doubleIsSmallerOrEqual(res,exprRes);
}

void Addition::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    res = 0.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->evaluate(exprRes, current, next, actions);
        res += exprRes;
    }
}

void Subtraction::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    exprs[0]->evaluate(res, current, next, actions);

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        exprs[i]->evaluate(exprRes, current, next, actions);
        res -= exprRes;
    }
}

void Multiplication::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    res = 1.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->evaluate(exprRes, current, next, actions);
        res *= exprRes;

        if(MathUtils::doubleIsEqual(res, 0.0)) {
            return;
        }
    }
}

void Division::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    exprs[0]->evaluate(res, current, next, actions);

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        if(MathUtils::doubleIsEqual(res, 0.0)) {
            return;
        }

        exprs[i]->evaluate(exprRes, current, next, actions);
        res /= exprRes;
    }
}

void BernoulliDistribution::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    expr->evaluate(res, current, next, actions);
}

void IfThenElseExpression::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    condition->evaluate(res, current, next, actions);
    if(MathUtils::doubleIsEqual(res,0.0)) {
        valueIfFalse->evaluate(res, current, next, actions);
    } else {
        valueIfTrue->evaluate(res, current, next, actions);
    }
}

void MultiConditionChecker::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    for(unsigned int i = 0; i < conditions.size(); ++i) {
        res = 0.0;
        conditions[i]->evaluate(res, current, next, actions);

        //TODO: In general, it is not correct that the condition evaluates to 0 or 1, but it works so far...
        assert(MathUtils::doubleIsEqual(res,0.0) || MathUtils::doubleIsEqual(res,1.0));

        if(!MathUtils::doubleIsEqual(res,0.0)) {
            effects[i]->evaluate(res, current, next, actions);
            return;
        }
    }
    assert(false);
}

void NegateExpression::evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
    expr->evaluate(res, current, next, actions);
    if(MathUtils::doubleIsGreater(res,1.0)) {
        res = 0.0;
    }
    res = 1.0 - res;
}
