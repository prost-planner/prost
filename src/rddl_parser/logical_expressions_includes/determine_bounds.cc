void LogicalExpression::determineBounds(vector<ActionState> const& actionStates,
                                        double& minRes, double& maxRes) const {
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());

    for (ActionState const& actionState : actionStates) {
        double min = numeric_limits<double>::max();
        double max = -numeric_limits<double>::max();
        determineBounds(actionState, min, max);
        minRes = std::min(minRes, min);
        maxRes = std::max(maxRes, max);
    }
}

void LogicalExpression::determineBounds(ActionState const& /*action*/,
                                        double& /*minRes*/,
                                        double& /*maxRes*/) const {
    assert(false);
}

/*****************************************************************
                           Atomics
*****************************************************************/

void StateFluent::determineBounds(ActionState const& /*action*/, double& minRes,
                                  double& maxRes) const {
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());
    minRes = 0.0;
    maxRes = domainSize - 1.0;
}

void ActionFluent::determineBounds(ActionState const& action, double& minRes,
                                   double& maxRes) const {
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());
    assert(index < action.state.size());
    minRes = action[index];
    maxRes = action[index];
}

void NumericConstant::determineBounds(ActionState const& /*action*/,
                                      double& minRes, double& maxRes) const {
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());
    minRes = value;
    maxRes = value;
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Conjunction::determineBounds(ActionState const& action, double& minRes,
                                  double& maxRes) const {
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());

    // This must be false if all variables are true
    bool minIsFalse = false;

    for (unsigned int i = 0; i < exprs.size(); ++i) {
        minRes = numeric_limits<double>::max();
        maxRes = -numeric_limits<double>::max();

        exprs[i]->determineBounds(action, minRes, maxRes);

        // If one max value is false every max and min value is false
        if (MathUtils::doubleIsEqual(maxRes, 0.0)) {
            return;
        }

        // If one min value is false, every min value is false
        if (!minIsFalse && MathUtils::doubleIsEqual(minRes, 0.0)) {
            minIsFalse = true;
        }
    }
    maxRes = 1.0;
    if (minIsFalse) {
        minRes = 0.0;
    } else {
        minRes = 1.0;
    }
}

void Disjunction::determineBounds(ActionState const& action, double& minRes,
                                  double& maxRes) const {
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());

    // This must be false if all variables are false
    bool maxIsTrue = false;

    for (unsigned int i = 0; i < exprs.size(); ++i) {
        minRes = numeric_limits<double>::max();
        maxRes = -numeric_limits<double>::max();
        exprs[i]->determineBounds(action, minRes, maxRes);

        // If one min value is true every max and min value is true
        if (MathUtils::doubleIsEqual(minRes, 1.0)) {
            return;
        }

        // If one max value is true every max value is true
        if (!maxIsTrue && MathUtils::doubleIsEqual(maxRes, 1.0)) {
            maxIsTrue = true;
        }
    }

    minRes = 0.0;
    if (maxIsTrue) {
        maxRes = 1.0;
    } else {
        maxRes = 0.0;
    }
}

void EqualsExpression::determineBounds(ActionState const& action,
                                       double& minRes, double& maxRes) const {
    assert(exprs.size() == 2);
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());

    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->determineBounds(action, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->determineBounds(action, rhsMin, rhsMax);

    // If both intervals dont overlap they won't ever be equal
    if (MathUtils::doubleIsGreater(lhsMin, rhsMax) ||
        MathUtils::doubleIsGreater(rhsMin, lhsMax)) {
        minRes = 0.0;
        maxRes = 0.0;
        return;
    }

    // If both intervals represent the same value, they are always equal
    if (MathUtils::doubleIsEqual(lhsMin, rhsMin) &&
        MathUtils::doubleIsEqual(lhsMin, lhsMax) &&
        MathUtils::doubleIsEqual(rhsMin, rhsMax)) {
        minRes = 1.0;
        maxRes = 1.0;
        return;
    }
    minRes = 0.0;
    maxRes = 1.0;
}

void GreaterExpression::determineBounds(ActionState const& action,
                                        double& minRes, double& maxRes) const {
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());

    assert(exprs.size() == 2);

    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->determineBounds(action, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->determineBounds(action, rhsMin, rhsMax);

    // If left min is greater than right max everything is greater
    if (MathUtils::doubleIsGreater(lhsMin, rhsMax)) {
        minRes = 1.0;
        maxRes = 1.0;
        return;
    }

    // The converse statement
    if (MathUtils::doubleIsGreaterOrEqual(rhsMin, lhsMax)) {
        minRes = 0.0;
        maxRes = 0.0;
        return;
    }

    // Otherwise the intervals overlap in at least one value
    minRes = 0.0;
    maxRes = 1.0;
}

void LowerExpression::determineBounds(ActionState const& action, double& minRes,
                                      double& maxRes) const {
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());

    assert(exprs.size() == 2);

    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->determineBounds(action, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->determineBounds(action, rhsMin, rhsMax);

    // If left max is lesser than right min everything is lesser
    if (MathUtils::doubleIsSmaller(lhsMax, rhsMin)) {
        minRes = 1.0;
        maxRes = 1.0;
        return;
    }

    // The converse statement
    if (MathUtils::doubleIsSmallerOrEqual(rhsMax, lhsMin)) {
        minRes = 0.0;
        maxRes = 0.0;
        return;
    }

    // Otherwise the intervals overlap in at least one value
    minRes = 0.0;
    maxRes = 1.0;
}

void GreaterEqualsExpression::determineBounds(ActionState const& action,
                                              double& minRes,
                                              double& maxRes) const {
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());

    assert(exprs.size() == 2);

    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->determineBounds(action, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->determineBounds(action, rhsMin, rhsMax);

    // If left min is greater or equal than right max everything is greater or
    // equal
    if (MathUtils::doubleIsGreaterOrEqual(lhsMin, rhsMax)) {
        minRes = 1.0;
        maxRes = 1.0;
        return;
    }

    // The converse statement
    if (MathUtils::doubleIsGreater(rhsMin, lhsMax)) {
        minRes = 0.0;
        maxRes = 0.0;
        return;
    }

    // Otherwise the intervals overlap
    minRes = 0.0;
    maxRes = 1.0;
}

void LowerEqualsExpression::determineBounds(ActionState const& action,
                                            double& minRes,
                                            double& maxRes) const {
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());

    assert(exprs.size() == 2);

    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->determineBounds(action, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->determineBounds(action, rhsMin, rhsMax);

    if (MathUtils::doubleIsSmallerOrEqual(lhsMax, rhsMin)) {
        minRes = 1.0;
        maxRes = 1.0;
        return;
    }

    // The converse statement
    if (MathUtils::doubleIsSmaller(rhsMax, lhsMin)) {
        minRes = 0.0;
        maxRes = 0.0;
        return;
    }

    // Otherwise the intervals overlap in at least one value
    minRes = 0.0;
    maxRes = 1.0;
}

void Addition::determineBounds(ActionState const& action, double& minRes,
                               double& maxRes) const {
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());

    // Addition on Intervals:
    // [x1,x2] + [y1,y2] = [x1+y1, x2+y2]

    exprs[0]->determineBounds(action, minRes, maxRes);

    for (unsigned int i = 1; i < exprs.size(); ++i) {
        double min = numeric_limits<double>::max();
        double max = -numeric_limits<double>::max();

        exprs[i]->determineBounds(action, min, max);
        minRes += min;
        maxRes += max;
    }
}

void Subtraction::determineBounds(ActionState const& action, double& minRes,
                                  double& maxRes) const {
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());

    // Subtraction on Intervals:
    // [x1,x2] - [y1,y2] = [x1-y2, x2-y1]

    exprs[0]->determineBounds(action, minRes, maxRes);

    for (unsigned int i = 1; i < exprs.size(); ++i) {
        double min = numeric_limits<double>::max();
        double max = -numeric_limits<double>::max();

        exprs[i]->determineBounds(action, min, max);
        minRes -= max;
        maxRes -= min;
    }
}

void Multiplication::determineBounds(ActionState const& action, double& minRes,
                                     double& maxRes) const {
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());

    // Multiplication on Intervals:
    // [x1,x2] * [y1,y2] = [min(x1*y1, x1*y2, x2*y1, x2*y2), max(x1*y1, x1*y2,
    // x2*y1, x2*y2)]

    exprs[0]->determineBounds(action, minRes, maxRes);

    for (unsigned int i = 1; i < exprs.size(); ++i) {
        double min = numeric_limits<double>::max();
        double max = -numeric_limits<double>::max();
        exprs[i]->determineBounds(action, min, max);
        // We could alternatively evaluate the signs of the individual values
        double helper = std::min(std::min(minRes * min, minRes * max),
                                 std::min(maxRes * min, maxRes * max));
        maxRes = std::max(std::max(minRes * min, minRes * max),
                          std::max(maxRes * min, maxRes * max));
        minRes = helper;
    }
}

void Division::determineBounds(ActionState const& action, double& minRes,
                               double& maxRes) const {
    assert(exprs.size() == 2);
    assert(minRes == numeric_limits<double>::max());
    assert(maxRes == -numeric_limits<double>::max());

    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->determineBounds(action, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->determineBounds(action, rhsMin, rhsMax);

    // Check for division by zero. Division is defined by
    // [x_1, x_2] / [y_1, y_2] = [x_1, x_2] * (1/[y_1,y_2], where
    // 1/[y_1,0] = [-infty, 1/y_1] and
    // 1/[0,y_2] = [1/y_2, infty]
    if (MathUtils::doubleIsEqual(rhsMax, 0.0) &&
        MathUtils::doubleIsEqual(rhsMin, 0.0)) {
        // Domain is ill-defined
        std::cout << "WARNING: DOMAIN ILL-DEFINED" << std::endl;
        minRes = -numeric_limits<double>::max();
        maxRes = numeric_limits<double>::max();
        return;
    }
    if (MathUtils::doubleIsEqual(rhsMax, 0.0)) {
        rhsMax = 1.0 / rhsMin;
        rhsMin = -numeric_limits<double>::max();
    }
    if (MathUtils::doubleIsEqual(rhsMin, 0.0)) {
        rhsMin = 1.0 / rhsMax;
        rhsMax = numeric_limits<double>::max();
    } else {
        rhsMin = 1.0 / rhsMin;
        rhsMax = 1.0 / rhsMax;
    }
    // Now that division by zero is handled we can just do interval
    // multiplication
    minRes = std::min(std::min(lhsMin * rhsMin, lhsMin * rhsMax),
                      std::min(lhsMax * rhsMin, lhsMax * rhsMax));
    maxRes = std::max(std::max(lhsMin * rhsMin, lhsMin * rhsMax),
                      std::max(lhsMax * rhsMin, rhsMax * rhsMax));
}

/*****************************************************************
  Unaries
 *****************************************************************/

void Negation::determineBounds(ActionState const& action, double& minRes,
                               double& maxRes) const {
    double tmpMin = numeric_limits<double>::max();
    double tmpMax = -numeric_limits<double>::max();
    expr->determineBounds(action, tmpMin, tmpMax);
    // Both max and min are either 0 or 1
    if (MathUtils::doubleIsEqual(tmpMin, tmpMax)) {
        if (MathUtils::doubleIsEqual(tmpMin, 0.0)) {
            minRes = 1.0;
            maxRes = 1.0;
            return;
        } else {
            minRes = 0.0;
            maxRes = 0.0;
            return;
        }
    }
    // Otherwise 0 and 1 are possible
    minRes = 0.0;
    maxRes = 1.0;
}

void ExponentialFunction::determineBounds(ActionState const& action,
                                          double& minRes,
                                          double& maxRes) const {
    expr->determineBounds(action, minRes, maxRes);

    minRes = std::exp(minRes);
    maxRes = std::exp(maxRes);
}

/*****************************************************************
  Probability Distributions
 *****************************************************************/

void BernoulliDistribution::determineBounds(ActionState const& action,
                                            double& minRes,
                                            double& maxRes) const {
    double tmpMin = numeric_limits<double>::max();
    double tmpMax = -numeric_limits<double>::max();
    expr->determineBounds(action, tmpMin, tmpMax);

    if (MathUtils::doubleIsGreater(tmpMax, 0.0)) {
        maxRes = 1.0;
    } else {
        maxRes = 0.0;
    }
    if (MathUtils::doubleIsSmaller(tmpMin, 1.0)) {
        minRes = 0.0;
    } else {
        minRes = 1.0;
    }
}

void DiscreteDistribution::determineBounds(ActionState const& action,
                                           double& minRes,
                                           double& maxRes) const {
    for (unsigned int i = 0; i < values.size(); ++i) {
        double probMin = numeric_limits<double>::max();
        double probMax = -numeric_limits<double>::max();

        probabilities[i]->determineBounds(action, probMin, probMax);
        if (!MathUtils::doubleIsEqual(probMin, probMax) ||
            !MathUtils::doubleIsEqual(0.0, probMax)) {
            double valMin = numeric_limits<double>::max();
            double valMax = -numeric_limits<double>::max();

            values[i]->determineBounds(action, valMin, valMax);
            if (MathUtils::doubleIsSmaller(valMin, minRes)) {
                minRes = valMin;
            }
            if (MathUtils::doubleIsGreater(valMax, maxRes)) {
                maxRes = valMax;
            }
        }
    }
}

/*****************************************************************
  Conditionals
 *****************************************************************/

void IfThenElseExpression::determineBounds(ActionState const& action,
                                           double& minRes,
                                           double& maxRes) const {
    double condMin = numeric_limits<double>::max();
    double condMax = -numeric_limits<double>::max();

    condition->determineBounds(action, condMin, condMax);

    if (MathUtils::doubleIsEqual(condMin, 0.0)) {
        // Condition may be false, valueIfFalse can fire
        double effMin = numeric_limits<double>::max();
        double effMax = -numeric_limits<double>::max();
        valueIfFalse->determineBounds(action, effMin, effMax);
        if (MathUtils::doubleIsSmaller(effMin, minRes)) {
            minRes = effMin;
        }
        if (MathUtils::doubleIsGreater(effMax, maxRes)) {
            maxRes = effMax;
        }
    }
    if (MathUtils::doubleIsGreater(condMax, 0.0)) {
        // Condition may be true, valueIfTrue can fire
        double effMin = numeric_limits<double>::max();
        double effMax = -numeric_limits<double>::max();
        // Condition may be false, valueIfFalse can fire
        valueIfTrue->determineBounds(action, effMin, effMax);
        if (MathUtils::doubleIsSmaller(effMin, minRes)) {
            minRes = effMin;
        }
        if (MathUtils::doubleIsGreater(effMax, maxRes)) {
            maxRes = effMax;
        }
    }
}

void MultiConditionChecker::determineBounds(ActionState const& action,
                                            double& minRes,
                                            double& maxRes) const {
    for (unsigned int i = 0; i < conditions.size(); ++i) {
        double condMin = numeric_limits<double>::max();
        double condMax = -numeric_limits<double>::max();

        conditions[i]->determineBounds(action, condMin, condMax);
        if (MathUtils::doubleIsGreater(condMax, 0.0)) {
            // condition fires in at least one case
            double tmpMin = numeric_limits<double>::max();
            double tmpMax = -numeric_limits<double>::max();

            effects[i]->determineBounds(action, tmpMin, tmpMax);
            if (MathUtils::doubleIsSmaller(tmpMin, minRes)) {
                minRes = tmpMin;
            }
            if (MathUtils::doubleIsGreater(tmpMax, maxRes)) {
                maxRes = tmpMax;
            }
            if (MathUtils::doubleIsGreater(condMin, 0.0)) {
                // condition always fires, other conditions won't be evaluated
                return;
            }
        }
    }
}
