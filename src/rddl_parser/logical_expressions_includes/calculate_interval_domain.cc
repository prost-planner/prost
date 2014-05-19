using std::numeric_limits;
using std::set;
using std::vector;

void LogicalExpression::calculateIntervalDomain(vector<set<double> > const& /*domains*/,
        ActionState const& /*actions*/,
        double& /*minRes*/, double& /*maxRes*/) {
    assert(false);
}

/*****************************************************************
                           Atomics
*****************************************************************/

void StateFluent::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& /*actions*/,
        double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());
    assert(index < domains.size());
    assert(!domains[index].empty());
    minRes = *(domains[index].begin());
    maxRes = *(domains[index].rbegin());
}

void ActionFluent::calculateIntervalDomain(vector<set<double> > const& /*domains*/,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());
    assert(index < actions.state.size());
    minRes = actions[index];
    maxRes = actions[index];
}

void NumericConstant::calculateIntervalDomain(vector<set<double> > const& /*domains*/,
        ActionState const& /*actions*/,
        double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());
    minRes = value;
    maxRes = value;  
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Conjunction::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions, 
        double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    // This must be false if all variables are true
    bool minIsFalse = false;

    for (unsigned int i = 0; i < exprs.size(); ++i) {
        minRes = numeric_limits<double>::max();
        maxRes = -numeric_limits<double>::max();

        exprs[i]->calculateIntervalDomain(domains, actions, minRes, maxRes);

        // If one max value is false every max and min value is false
        if (MathUtils::doubleIsEqual(maxRes,0.0)) {
            return;
        }

        // If one min value is false, every min value is false
        if (!minIsFalse && MathUtils::doubleIsEqual(minRes,0.0)) {
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

void Disjunction::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    // This must be false if all variables are false
    bool maxIsTrue = false;

    for (unsigned int i = 0; i < exprs.size(); ++i) {
        minRes = numeric_limits<double>::max();
        maxRes = -numeric_limits<double>::max();
        exprs[i]->calculateIntervalDomain(domains, actions, minRes, maxRes);

        // If one min value is true every max and min value is true 
        if (MathUtils::doubleIsEqual(minRes,1.0)) {
            return;
        }

        // If one max value is true every max value is true
        if (!maxIsTrue && MathUtils::doubleIsEqual(maxRes,1.0)) {
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

void EqualsExpression::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    assert(exprs.size() == 2);
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->calculateIntervalDomain(domains, actions, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->calculateIntervalDomain(domains, actions, rhsMin, rhsMax);

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

void GreaterExpression::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    assert(exprs.size() == 2);
    
    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->calculateIntervalDomain(domains, actions, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->calculateIntervalDomain(domains, actions, rhsMin, rhsMax);
    
    // If left min is greater than right max everything is greater
    if (MathUtils::doubleIsGreater(lhsMin, rhsMax)) {
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

    // Otherwise the intervals overlap in at least one value
    minRes = 0.0;
    maxRes = 1.0; 
}

void LowerExpression::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    assert(exprs.size() == 2);
    
    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->calculateIntervalDomain(domains, actions, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->calculateIntervalDomain(domains, actions, rhsMin, rhsMax);
    
    // If left max is lesser than right min everything is lesser
    if (MathUtils::doubleIsSmaller(lhsMax, rhsMin)) {
        minRes = 0.0;
        maxRes = 0.0;
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

void GreaterEqualsExpression::calculateIntervalDomain(
        vector<set<double> > const& domains, ActionState const& actions,
        double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    assert(exprs.size() == 2);
    
    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->calculateIntervalDomain(domains, actions, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->calculateIntervalDomain(domains, actions, rhsMin, rhsMax);
    
    // If left min is greater or equal than right max everything is greater or
    // equal
    if (MathUtils::doubleIsGreaterOrEqual(lhsMin, rhsMax)) {
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

    // Otherwise the intervals overlap 
    minRes = 0.0;
    maxRes = 1.0; 
}

void LowerEqualsExpression::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    assert(exprs.size() == 2);
    
    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->calculateIntervalDomain(domains, actions, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->calculateIntervalDomain(domains, actions, rhsMin, rhsMax);
    
    if (MathUtils::doubleIsSmallerOrEqual(lhsMax, rhsMin)) {
        minRes = 0.0;
        maxRes = 0.0;
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

void Addition::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    // Addition on Intervals:
    // [x1,x2] + [y1,y2] = [x1+y1, x2+y2]

    exprs[0]->calculateIntervalDomain(domains, actions, minRes, maxRes);

    for (unsigned int i = 1; i < exprs.size(); ++i) {
        double min = numeric_limits<double>::max();
        double max = -numeric_limits<double>::max();

        exprs[i]->calculateIntervalDomain(domains, actions, min, max);
        minRes += min;
        maxRes += max;
    }
}

void Subtraction::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    // Subtraction on Intervals:
    // [x1,x2] - [y1,y2] = [x1-y2, x2-y1]

    exprs[0]->calculateIntervalDomain(domains, actions, minRes, maxRes);

    for (unsigned int i = 1; i < exprs.size(); ++i) {
        double min = numeric_limits<double>::max();
        double max = -numeric_limits<double>::max();

        exprs[i]->calculateIntervalDomain(domains, actions, min, max);
        minRes -= max;
        maxRes -= min;
    }
}

void Multiplication::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    // Multiplication on Intervals:
    // [x1,x2] * [y1,y2] = [min(x1*y1, x1*y2, x2*y1, x2*y2), max(x1*y1, x1*y2, x2*y1, x2*y2)]

    exprs[0]->calculateIntervalDomain(domains, actions, minRes, maxRes);

    for (unsigned int i = 1; i < exprs.size(); ++i) {
        double min = numeric_limits<double>::max();
        double max = -numeric_limits<double>::max();
        exprs[i]->calculateIntervalDomain(domains, actions, min, max);
        // We could alternatively evaluate the signs of the individual values
        minRes = std::min(std::min(minRes*min, minRes*max),
                           std::min(maxRes*min, maxRes*max));
        maxRes = std::max(std::max(minRes*min, minRes*max),
                           std::max(maxRes*min, maxRes*max));
    }
}

void Division::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    assert(exprs.size() == 2);
    assert(minRes = numeric_limits<double>::max());
    assert(maxRes = -numeric_limits<double>::max());

    double lhsMin = numeric_limits<double>::max();
    double lhsMax = -numeric_limits<double>::max();
    exprs[0]->calculateIntervalDomain(domains, actions, lhsMin, lhsMax);

    double rhsMin = numeric_limits<double>::max();
    double rhsMax = -numeric_limits<double>::max();
    exprs[1]->calculateIntervalDomain(domains, actions, rhsMin, rhsMax);
    
    //TODO
    minRes = lhsMin / rhsMin;
    maxRes = lhsMax / rhsMax;
}

/*****************************************************************
  Unaries
 *****************************************************************/

void Negation::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    double tmpMin = numeric_limits<double>::max();
    double tmpMax = -numeric_limits<double>::max();
    expr->calculateIntervalDomain(domains, actions, tmpMin, tmpMax);
    if (MathUtils::doubleIsEqual(tmpMin, 0.0)) {
        minRes = 0.0;
    } else {
        minRes = 1.0;
    }
    if (MathUtils::doubleIsEqual(tmpMax, 1.0)) {
        maxRes = 1.0;
    } else {
        maxRes = 0.0;
    }
}

/*****************************************************************
  Probability Distributions
 *****************************************************************/

void BernoulliDistribution::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    double tmpMin = numeric_limits<double>::max();
    double tmpMax = -numeric_limits<double>::max();
    expr->calculateIntervalDomain(domains, actions, tmpMin, tmpMax);

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

void DiscreteDistribution::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    for (unsigned int i = 0; i < values.size(); ++i) {
        double probMin = numeric_limits<double>::max();
        double probMax = -numeric_limits<double>::max();

        probabilities[i]->calculateIntervalDomain(domains, actions, probMin, probMax);
        if (!MathUtils::doubleIsEqual(probMin, probMax) || 
            !MathUtils::doubleIsEqual(0.0, probMax)) {
            double valMin = numeric_limits<double>::max();
            double valMax = -numeric_limits<double>::max();

            values[i]->calculateIntervalDomain(domains, actions, valMin, valMax);
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

void IfThenElseExpression::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    double condMin =  numeric_limits<double>::max();
    double condMax =  -numeric_limits<double>::max();

    condition->calculateIntervalDomain(domains, actions, condMin, condMax);
    if (!MathUtils::doubleIsEqual(condMin, condMax)) {
        // cond has more than one value, one of which must represent 'true'
        valueIfTrue->calculateIntervalDomain(domains, actions, minRes, maxRes);
        if (MathUtils::doubleIsEqual(0.0, minRes)) {
            // and false is also possible
            double tmpMin = numeric_limits<double>::max();
            double tmpMax = -numeric_limits<double>::max();

            valueIfFalse->calculateIntervalDomain(domains, actions, tmpMin, tmpMax);
            if (MathUtils::doubleIsSmaller(tmpMin, minRes)) {
                minRes = tmpMin;
            }
            if (MathUtils::doubleIsGreater(tmpMax, maxRes)) {
                maxRes = tmpMax;
            }
        }
    } else if (MathUtils::doubleIsEqual(minRes, 0.0)) {
        // there is only one value in cond which is 'false'
        valueIfFalse->calculateIntervalDomain(domains, actions, minRes, maxRes);
    } else {
        valueIfTrue->calculateIntervalDomain(domains, actions, minRes, maxRes);
    }
}

void MultiConditionChecker::calculateIntervalDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        double& minRes, double& maxRes) {
    for (unsigned int i = 0; i < conditions.size(); ++i) {
        double condMin =  numeric_limits<double>::max();
        double condMax =  -numeric_limits<double>::max();

        conditions[i]->calculateIntervalDomain(domains, actions, condMin, condMax);
        if (!MathUtils::doubleIsEqual(condMin, condMax)) {
            // cond has more than one value, one of which must represent 'true'
            double tmpMin = numeric_limits<double>::max();
            double tmpMax = -numeric_limits<double>::max();

            effects[i]->calculateIntervalDomain(domains, actions, tmpMin, tmpMax);
            if (MathUtils::doubleIsSmaller(tmpMin, minRes)) {
                minRes = tmpMin;
            }
            if (MathUtils::doubleIsGreater(tmpMax, maxRes)) {
                maxRes = tmpMax;
            }
            if (!MathUtils::doubleIsEqual(condMin, 0.0)) {
                // all values in cond represent 'true'
                return;
            }
        } else if (!MathUtils::doubleIsEqual(condMin, 0.0)) {
            // there is only one value in cond, and it is true
            double tmpMin = numeric_limits<double>::max();
            double tmpMax = -numeric_limits<double>::max();
            effects[i]->calculateIntervalDomain(domains, actions, tmpMin, tmpMax);
            if (MathUtils::doubleIsSmaller(tmpMin, minRes)) {
                minRes = tmpMin;
            }
            if (MathUtils::doubleIsGreater(tmpMax, maxRes)) {
                maxRes = tmpMax;
            }
            return;
        }
    }
}
