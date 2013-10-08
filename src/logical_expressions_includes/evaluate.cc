void LogicalExpression::evaluate(double& /*res*/, State const& /*current*/, ActionState const& /*actions*/) {
    assert(false);
}

void StateFluent::evaluate(double& res, State const& current, ActionState const& /*actions*/) {
    res = current[index];
}

void ActionFluent::evaluate(double& res, State const& /*current*/, ActionState const& actions) {
    res = actions[index];
}

void NumericConstant::evaluate(double& res, State const& /*current*/, ActionState const& /*actions*/) {
    res = value;
}

void Conjunction::evaluate(double& res, State const& current, ActionState const& actions) {
    res = 1.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        double exprRes = 0.0;
        exprs[i]->evaluate(exprRes, current, actions);
        if(MathUtils::doubleIsEqual(exprRes, 0.0)) {
            res = 0.0;
            return;
        } else {
            res *= exprRes;
        }
    }
}

void Disjunction::evaluate(double& res, State const& current, ActionState const& actions) {
    res = 1.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        double exprRes = 0.0;
        exprs[i]->evaluate(exprRes, current, actions);
        if(MathUtils::doubleIsEqual(exprRes, 1.0)) {
            res = 1.0;
            return;
        } else {
            res *= (1.0 - exprRes);
        }
    }
    res = (1.0 - res);
}

void EqualsExpression::evaluate(double& res, State const& current, ActionState const& actions) {
    assert(exprs.size() == 2);
    double lhs = 0.0;
    exprs[0]->evaluate(lhs, current, actions);
    double rhs = 0.0;
    exprs[1]->evaluate(rhs, current, actions);

    res = MathUtils::doubleIsEqual(lhs, rhs);
}

void GreaterExpression::evaluate(double& res, State const& current, ActionState const& actions) {
    assert(exprs.size() == 2);
    double lhs = 0.0;
    exprs[0]->evaluate(lhs, current, actions);
    double rhs = 0.0;
    exprs[1]->evaluate(rhs, current, actions);

    res = MathUtils::doubleIsGreater(lhs, rhs);
}

void LowerExpression::evaluate(double& res, State const& current, ActionState const& actions) {
    assert(exprs.size() == 2);
    double lhs = 0.0;
    exprs[0]->evaluate(lhs, current, actions);
    double rhs = 0.0;
    exprs[1]->evaluate(rhs, current, actions);

    res = MathUtils::doubleIsSmaller(lhs, rhs);
}

void GreaterEqualsExpression::evaluate(double& res, State const& current, ActionState const& actions) {
    assert(exprs.size() == 2);
    double lhs = 0.0;
    exprs[0]->evaluate(lhs, current, actions);
    double rhs = 0.0;
    exprs[1]->evaluate(rhs, current, actions);

    res = MathUtils::doubleIsGreaterOrEqual(lhs, rhs);
}

void LowerEqualsExpression::evaluate(double& res, State const& current, ActionState const& actions) {
    assert(exprs.size() == 2);
    double lhs = 0.0;
    exprs[0]->evaluate(lhs, current, actions);
    double rhs = 0.0;
    exprs[1]->evaluate(rhs, current, actions);

    res = MathUtils::doubleIsSmallerOrEqual(lhs, rhs);
}

void Addition::evaluate(double& res, State const& current, ActionState const& actions) {
    res = 0.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        double exprRes = 0.0;
        exprs[i]->evaluate(exprRes, current, actions);
        res += exprRes;
    }
}

void Subtraction::evaluate(double& res, State const& current, ActionState const& actions) {
    exprs[0]->evaluate(res, current, actions);

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        double exprRes = 0.0;
        exprs[i]->evaluate(exprRes, current, actions);
        res -= exprRes;
    }
}

void Multiplication::evaluate(double& res, State const& current, ActionState const& actions) {
    res = 1.0;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        double exprRes = 0.0;
        exprs[i]->evaluate(exprRes, current, actions);
        res *= exprRes;

        if(MathUtils::doubleIsEqual(res, 0.0)) {
            return;
        }
    }
}

void Division::evaluate(double& res, State const& current, ActionState const& actions) {
    exprs[0]->evaluate(res, current, actions);

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        if(MathUtils::doubleIsEqual(res, 0.0)) {
            return;
        }

        double exprRes = 0.0;
        exprs[i]->evaluate(exprRes, current, actions);
        res /= exprRes;
    }
}

void BernoulliDistribution::evaluate(double& res, State const& current, ActionState const& actions) {
    expr->evaluate(res, current, actions);
}

void IfThenElseExpression::evaluate(double& res, State const& current, ActionState const& actions) {
    condition->evaluate(res, current, actions);
    if(MathUtils::doubleIsEqual(res, 0.0)) {
        valueIfFalse->evaluate(res, current, actions);
    } else if(MathUtils::doubleIsEqual(res, 1.0)) {
        valueIfTrue->evaluate(res, current, actions);
    } else {
        double trueRes = 0.0;
        valueIfTrue->evaluate(trueRes, current, actions);
        double falseRes = 0.0;
        valueIfFalse->evaluate(falseRes, current, actions);
        res = (res * trueRes) + ((1.0 - res) * falseRes);
    }
}

void MultiConditionChecker::evaluate(double& res, State const& current, ActionState const& actions) {
    res = 0.0;
    double remainingProb = 1.0;

    for(unsigned int i = 0; i < conditions.size(); ++i) {
        double prob = 0.0;
        conditions[i]->evaluate(prob, current, actions);

        assert(MathUtils::doubleIsGreaterOrEqual(prob, 0.0) && MathUtils::doubleIsSmallerOrEqual(prob, 1.0));

        if(!MathUtils::doubleIsEqual(prob, 0.0)) {
            double effRes = 0.0;
            effects[i]->evaluate(effRes, current, actions);
            res += (prob * remainingProb * effRes);
        }

        remainingProb *= (1.0 - prob);
        if(MathUtils::doubleIsEqual(remainingProb, 0.0)) {
            return;
        }
    }
    assert(false);
}

void NegateExpression::evaluate(double& res, State const& current, ActionState const& actions) {
    expr->evaluate(res, current, actions);
    res = MathUtils::doubleIsEqual(res, 0.0);
}
