void LogicalExpression::calculateDomainAsInterval(Domains const& /*domains*/,
                                                  ActionState const& /*action*/,
                                                  double& /*minRes*/,
                                                  double& /*maxRes*/) {
    assert(false);
}

/*****************************************************************
                           Atomics
*****************************************************************/

void StateFluent::calculateDomainAsInterval(Domains const& domains,
                                            ActionState const& /*action*/,
                                            double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());
    assert(index < domains.size());
    assert(!domains[index].empty());
    minRes = *(domains[index].begin());
    maxRes = *(domains[index].rbegin());
}

void ActionFluent::calculateDomainAsInterval(Domains const& /*domains*/,
                                             ActionState const& action,
                                             double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());
    assert(index < action.state.size());
    minRes = action[index];
    maxRes = action[index];
}

void NumericConstant::calculateDomainAsInterval(Domains const& /*domains*/,
                                                ActionState const& /*action*/,
                                                double& minRes,
                                                double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());
    minRes = value;
    maxRes = value;
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Conjunction::calculateDomainAsInterval(Domains const& domains,
                                            ActionState const& action,
                                            double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    // This must be false if all variables are true
    bool minIsFalse = false;

    for (unsigned int i = 0; i < exprs.size(); ++i) {
        minRes = numeric_limits<double>::max();
        maxRes = -numeric_limits<double>::max();

        exprs[i]->calculateDomainAsInterval(domains, action, minRes, maxRes);

        // If one max value is false every max and min value is false
        if (utils::doubleIsEqual(maxRes, 0.0)) {
            return;
        }

        // If one min value is false, every min value is false
        if (!minIsFalse && utils::doubleIsEqual(minRes, 0.0)) {
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

void Disjunction::calculateDomainAsInterval(Domains const& domains,
                                            ActionState const& action,
                                            double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    // This must be false if all variables are false
    bool maxIsTrue = false;

    for (unsigned int i = 0; i < exprs.size(); ++i) {
        minRes = numeric_limits<double>::max();
        maxRes = -numeric_limits<double>::max();
        exprs[i]->calculateDomainAsInterval(domains, action, minRes, maxRes);

        // If one min value is true every max and min value is true
        if (utils::doubleIsEqual(minRes, 1.0)) {
            return;
        }

        // If one max value is true every max value is true
        if (!maxIsTrue && utils::doubleIsEqual(maxRes, 1.0)) {
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

void EqualsExpression::calculateDomainAsInterval(Domains const& domains,
                                                 ActionState const& action,
                                                 double& minRes,
                                                 double& maxRes) {
    assert(exprs.size() == 2);
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->calculateDomainAsInterval(domains, action, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->calculateDomainAsInterval(domains, action, rhsMin, rhsMax);

    // If both intervals dont overlap they won't ever be equal
    if (utils::doubleIsGreater(lhsMin, rhsMax) ||
        utils::doubleIsGreater(rhsMin, lhsMax)) {
        minRes = 0.0;
        maxRes = 0.0;
        return;
    }

    // If both intervals represent the same value, they are always equal
    if (utils::doubleIsEqual(lhsMin, rhsMin) &&
        utils::doubleIsEqual(lhsMin, lhsMax) &&
        utils::doubleIsEqual(rhsMin, rhsMax)) {
        minRes = 1.0;
        maxRes = 1.0;
        return;
    }
    minRes = 0.0;
    maxRes = 1.0;
}

void GreaterExpression::calculateDomainAsInterval(Domains const& domains,
                                                  ActionState const& action,
                                                  double& minRes,
                                                  double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    assert(exprs.size() == 2);

    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->calculateDomainAsInterval(domains, action, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->calculateDomainAsInterval(domains, action, rhsMin, rhsMax);

    // If left min is greater than right max everything is greater
    if (utils::doubleIsGreater(lhsMin, rhsMax)) {
        minRes = 1.0;
        maxRes = 1.0;
        return;
    }

    // The converse statement
    if (utils::doubleIsGreaterOrEqual(rhsMin, lhsMax)) {
        minRes = 0.0;
        maxRes = 0.0;
        return;
    }

    // Otherwise the intervals overlap in at least one value
    minRes = 0.0;
    maxRes = 1.0;
}

void LowerExpression::calculateDomainAsInterval(Domains const& domains,
                                                ActionState const& action,
                                                double& minRes,
                                                double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    assert(exprs.size() == 2);

    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->calculateDomainAsInterval(domains, action, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->calculateDomainAsInterval(domains, action, rhsMin, rhsMax);

    // If left max is lesser than right min everything is lesser
    if (utils::doubleIsSmaller(lhsMax, rhsMin)) {
        minRes = 1.0;
        maxRes = 1.0;
        return;
    }

    // The converse statement
    if (utils::doubleIsSmallerOrEqual(rhsMax, lhsMin)) {
        minRes = 0.0;
        maxRes = 0.0;
        return;
    }

    // Otherwise the intervals overlap in at least one value
    minRes = 0.0;
    maxRes = 1.0;
}

void GreaterEqualsExpression::calculateDomainAsInterval(
    Domains const& domains, ActionState const& action, double& minRes,
    double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    assert(exprs.size() == 2);

    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->calculateDomainAsInterval(domains, action, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->calculateDomainAsInterval(domains, action, rhsMin, rhsMax);

    // If left min is greater or equal than right max everything is greater or
    // equal
    if (utils::doubleIsGreaterOrEqual(lhsMin, rhsMax)) {
        minRes = 1.0;
        maxRes = 1.0;
        return;
    }

    // The converse statement
    if (utils::doubleIsGreater(rhsMin, lhsMax)) {
        minRes = 0.0;
        maxRes = 0.0;
        return;
    }

    // Otherwise the intervals overlap
    minRes = 0.0;
    maxRes = 1.0;
}

void LowerEqualsExpression::calculateDomainAsInterval(Domains const& domains,
                                                      ActionState const& action,
                                                      double& minRes,
                                                      double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    assert(exprs.size() == 2);

    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->calculateDomainAsInterval(domains, action, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->calculateDomainAsInterval(domains, action, rhsMin, rhsMax);

    if (utils::doubleIsSmallerOrEqual(lhsMax, rhsMin)) {
        minRes = 1.0;
        maxRes = 1.0;
        return;
    }

    // The converse statement
    if (utils::doubleIsSmaller(rhsMax, lhsMin)) {
        minRes = 0.0;
        maxRes = 0.0;
        return;
    }

    // Otherwise the intervals overlap in at least one value
    minRes = 0.0;
    maxRes = 1.0;
}

void Addition::calculateDomainAsInterval(Domains const& domains,
                                         ActionState const& action,
                                         double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    // Addition on Intervals:
    // [x1,x2] + [y1,y2] = [x1+y1, x2+y2]

    exprs[0]->calculateDomainAsInterval(domains, action, minRes, maxRes);

    for (unsigned int i = 1; i < exprs.size(); ++i) {
        double min = numeric_limits<double>::max();
        double max = -numeric_limits<double>::max();

        exprs[i]->calculateDomainAsInterval(domains, action, min, max);
        minRes += min;
        maxRes += max;
    }
}

void Subtraction::calculateDomainAsInterval(Domains const& domains,
                                            ActionState const& action,
                                            double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    // Subtraction on Intervals:
    // [x1,x2] - [y1,y2] = [x1-y2, x2-y1]

    exprs[0]->calculateDomainAsInterval(domains, action, minRes, maxRes);

    for (unsigned int i = 1; i < exprs.size(); ++i) {
        double min = numeric_limits<double>::max();
        double max = -numeric_limits<double>::max();

        exprs[i]->calculateDomainAsInterval(domains, action, min, max);
        minRes -= max;
        maxRes -= min;
    }
}

void Multiplication::calculateDomainAsInterval(Domains const& domains,
                                               ActionState const& action,
                                               double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    // Multiplication on Intervals:
    // [x1,x2] * [y1,y2] = [min(x1*y1, x1*y2, x2*y1, x2*y2), max(x1*y1, x1*y2,
    // x2*y1, x2*y2)]

    exprs[0]->calculateDomainAsInterval(domains, action, minRes, maxRes);

    for (unsigned int i = 1; i < exprs.size(); ++i) {
        double min = numeric_limits<double>::max();
        double max = -numeric_limits<double>::max();
        exprs[i]->calculateDomainAsInterval(domains, action, min, max);
        // We could alternatively evaluate the signs of the individual values
        double helper = std::min(std::min(minRes * min, minRes * max),
                                 std::min(maxRes * min, maxRes * max));
        maxRes = std::max(std::max(minRes * min, minRes * max),
                          std::max(maxRes * min, maxRes * max));
        minRes = helper;
    }
}

void Division::calculateDomainAsInterval(Domains const& domains,
                                         ActionState const& action,
                                         double& minRes, double& maxRes) {
    assert(exprs.size() == 2);
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->calculateDomainAsInterval(domains, action, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->calculateDomainAsInterval(domains, action, rhsMin, rhsMax);

    // Check for division by zero. Division is defined by
    // [x_1, x_2] / [y_1, y_2] = [x_1, x_2] * (1/[y_1,y_2], where
    // 1/[y_1,0] = [-infty, 1/y_1] and
    // 1/[0,y_2] = [1/y_2, infty]
    if (utils::doubleIsEqual(rhsMax, 0.0) &&
        utils::doubleIsEqual(rhsMin, 0.0)) {
        // Domain is ill-defined
        std::cout << "WARNING: DOMAIN ILL-DEFINED" << std::endl;
        minRes = -numeric_limits<double>::max();
        maxRes = numeric_limits<double>::max();
        return;
    }
    if (utils::doubleIsEqual(rhsMax, 0.0)) {
        rhsMax = 1.0 / rhsMin;
        rhsMin = -numeric_limits<double>::max();
    }
    if (utils::doubleIsEqual(rhsMin, 0.0)) {
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

void Negation::calculateDomainAsInterval(Domains const& domains,
                                         ActionState const& action,
                                         double& minRes, double& maxRes) {
    double tmpMin = numeric_limits<double>::max();
    double tmpMax = -numeric_limits<double>::max();
    expr->calculateDomainAsInterval(domains, action, tmpMin, tmpMax);
    // Both max and min are either 0 or 1
    if (utils::doubleIsEqual(tmpMin, tmpMax)) {
        if (utils::doubleIsEqual(tmpMin, 0.0)) {
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

void ExponentialFunction::calculateDomainAsInterval(Domains const& domains,
                                                    ActionState const& action,
                                                    double& minRes,
                                                    double& maxRes) {
    expr->calculateDomainAsInterval(domains, action, minRes, maxRes);

    minRes = std::exp(minRes);
    maxRes = std::exp(maxRes);
}

/*****************************************************************
  Probability Distributions
 *****************************************************************/

void BernoulliDistribution::calculateDomainAsInterval(Domains const& domains,
                                                      ActionState const& action,
                                                      double& minRes,
                                                      double& maxRes) {
    double tmpMin = numeric_limits<double>::max();
    double tmpMax = -numeric_limits<double>::max();
    expr->calculateDomainAsInterval(domains, action, tmpMin, tmpMax);

    if (utils::doubleIsGreater(tmpMax, 0.0)) {
        maxRes = 1.0;
    } else {
        maxRes = 0.0;
    }
    if (utils::doubleIsSmaller(tmpMin, 1.0)) {
        minRes = 0.0;
    } else {
        minRes = 1.0;
    }
}

void DiscreteDistribution::calculateDomainAsInterval(Domains const& domains,
                                                     ActionState const& action,
                                                     double& minRes,
                                                     double& maxRes) {
    for (unsigned int i = 0; i < values.size(); ++i) {
        double probMin = numeric_limits<double>::max();
        double probMax = -numeric_limits<double>::max();

        probabilities[i]->calculateDomainAsInterval(domains, action, probMin,
                                                    probMax);
        if (!utils::doubleIsEqual(probMin, probMax) ||
            !utils::doubleIsEqual(0.0, probMax)) {
            double valMin = numeric_limits<double>::max();
            double valMax = -numeric_limits<double>::max();

            values[i]->calculateDomainAsInterval(domains, action, valMin,
                                                 valMax);
            if (utils::doubleIsSmaller(valMin, minRes)) {
                minRes = valMin;
            }
            if (utils::doubleIsGreater(valMax, maxRes)) {
                maxRes = valMax;
            }
        }
    }
}

/*****************************************************************
  Conditionals
 *****************************************************************/

void IfThenElseExpression::calculateDomainAsInterval(Domains const& domains,
                                                     ActionState const& action,
                                                     double& minRes,
                                                     double& maxRes) {
    double condMin = numeric_limits<double>::max();
    double condMax = -numeric_limits<double>::max();

    condition->calculateDomainAsInterval(domains, action, condMin, condMax);
    if (!utils::doubleIsEqual(condMin, condMax)) {
        // cond has more than one value, one of which must represent 'true'
        valueIfTrue->calculateDomainAsInterval(domains, action, minRes, maxRes);
        if (utils::doubleIsEqual(0.0, minRes)) {
            // and false is also possible
            double tmpMin = numeric_limits<double>::max();
            double tmpMax = -numeric_limits<double>::max();

            valueIfFalse->calculateDomainAsInterval(domains, action, tmpMin,
                                                    tmpMax);
            if (utils::doubleIsSmaller(tmpMin, minRes)) {
                minRes = tmpMin;
            }
            if (utils::doubleIsGreater(tmpMax, maxRes)) {
                maxRes = tmpMax;
            }
        }
    } else if (utils::doubleIsEqual(minRes, 0.0)) {
        // there is only one value in cond which is 'false'
        valueIfFalse->calculateDomainAsInterval(domains, action, minRes,
                                                maxRes);
    } else {
        valueIfTrue->calculateDomainAsInterval(domains, action, minRes, maxRes);
    }
}

void MultiConditionChecker::calculateDomainAsInterval(Domains const& domains,
                                                      ActionState const& action,
                                                      double& minRes,
                                                      double& maxRes) {
    for (unsigned int i = 0; i < conditions.size(); ++i) {
        double condMin = numeric_limits<double>::max();
        double condMax = -numeric_limits<double>::max();

        conditions[i]->calculateDomainAsInterval(domains, action, condMin,
                                                 condMax);
        if (!utils::doubleIsEqual(condMin, condMax)) {
            // cond has more than one value, one of which must represent 'true'
            double tmpMin = numeric_limits<double>::max();
            double tmpMax = -numeric_limits<double>::max();

            effects[i]->calculateDomainAsInterval(domains, action, tmpMin,
                                                  tmpMax);
            if (utils::doubleIsSmaller(tmpMin, minRes)) {
                minRes = tmpMin;
            }
            if (utils::doubleIsGreater(tmpMax, maxRes)) {
                maxRes = tmpMax;
            }
            if (!utils::doubleIsEqual(condMin, 0.0)) {
                // all values in cond represent 'true'
                return;
            }
        } else if (!utils::doubleIsEqual(condMin, 0.0)) {
            // there is only one value in cond, and it is true
            double tmpMin = numeric_limits<double>::max();
            double tmpMax = -numeric_limits<double>::max();
            effects[i]->calculateDomainAsInterval(domains, action, tmpMin,
                                                  tmpMax);
            if (utils::doubleIsSmaller(tmpMin, minRes)) {
                minRes = tmpMin;
            }
            if (utils::doubleIsGreater(tmpMax, maxRes)) {
                maxRes = tmpMax;
            }
            return;
        }
    }
}
