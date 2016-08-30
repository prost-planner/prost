void LogicalExpression::evaluateToPD(DiscretePD& /*res*/,
                                     State const& /*current*/,
                                     ActionState const& /*actions*/) const {
    assert(false);
}

/*****************************************************************
                           Atomics
*****************************************************************/

void DeterministicStateFluent::evaluateToPD(
    DiscretePD& res, State const& current,
    ActionState const& /*actions*/) const {
    res.assignDiracDelta(current.deterministicStateFluent(index));
}

void ProbabilisticStateFluent::evaluateToPD(
    DiscretePD& res, State const& current,
    ActionState const& /*actions*/) const {
    res.assignDiracDelta(current.probabilisticStateFluent(index));
}

void ActionFluent::evaluateToPD(DiscretePD& res, State const& /*current*/,
                                ActionState const& actions) const {
    res.assignDiracDelta(actions[index]);
}

void NumericConstant::evaluateToPD(DiscretePD& res, State const& /*current*/,
                                   ActionState const& /*actions*/) const {
    res.assignDiracDelta(value);
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Conjunction::evaluateToPD(DiscretePD& res, State const& current,
                               ActionState const& actions) const {
    double truthProb = 1.0;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        DiscretePD exprRes;
        exprs[i]->evaluateToPD(exprRes, current, actions);
        assert(exprRes.isWellDefined());

        if (exprRes.isFalsity()) {
            res.assignDiracDelta(0.0);
            return;
        } else {
            truthProb *= exprRes.truthProbability();
        }
    }

    res.assignBernoulli(truthProb);
}

void Disjunction::evaluateToPD(DiscretePD& res, State const& current,
                               ActionState const& actions) const {
    double falsityProb = 1.0;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        DiscretePD exprRes;
        exprs[i]->evaluateToPD(exprRes, current, actions);
        assert(exprRes.isWellDefined());

        if (exprRes.isTruth()) {
            res.assignDiracDelta(1.0);
            return;
        } else {
            falsityProb *= exprRes.falsityProbability();
        }
    }

    res.assignBernoulli(1.0 - falsityProb);
}

void EqualsExpression::evaluateToPD(DiscretePD& res, State const& current,
                                    ActionState const& actions) const {
    assert(exprs.size() == 2);

    DiscretePD lhs;
    exprs[0]->evaluateToPD(lhs, current, actions);
    assert(lhs.isWellDefined());

    DiscretePD rhs;
    exprs[1]->evaluateToPD(rhs, current, actions);
    assert(rhs.isWellDefined());

    // We don't check if the probability distributions are identical but we
    // compute the probability that they evaluateToPD to the same value!
    double equalityProb = 0.0;
    for (unsigned int i = 0; i < lhs.values.size(); ++i) {
        equalityProb +=
            (lhs.probabilities[i] * rhs.probabilityOf(lhs.values[i]));
    }

    res.assignBernoulli(equalityProb);
}

void GreaterExpression::evaluateToPD(DiscretePD& res, State const& current,
                                     ActionState const& actions) const {
    assert(exprs.size() == 2);

    DiscretePD lhs;
    exprs[0]->evaluateToPD(lhs, current, actions);
    assert(lhs.isWellDefined());

    DiscretePD rhs;
    exprs[1]->evaluateToPD(rhs, current, actions);
    assert(rhs.isWellDefined());

    double greaterProb = 0.0;
    for (unsigned int i = 0; i < lhs.values.size(); ++i) {
        for (unsigned int j = 0; j < rhs.values.size(); ++j) {
            if (MathUtils::doubleIsGreater(lhs.values[i], rhs.values[j])) {
                greaterProb += (rhs.probabilities[j] * lhs.probabilities[i]);
            } else {
                break;
            }
        }
    }

    res.assignBernoulli(greaterProb);
}

void LowerExpression::evaluateToPD(DiscretePD& res, State const& current,
                                   ActionState const& actions) const {
    assert(exprs.size() == 2);

    DiscretePD lhs;
    exprs[0]->evaluateToPD(lhs, current, actions);
    assert(lhs.isWellDefined());

    DiscretePD rhs;
    exprs[1]->evaluateToPD(rhs, current, actions);
    assert(rhs.isWellDefined());

    double lowerProb = 0.0;
    for (unsigned int i = 0; i < lhs.values.size(); ++i) {
        for (int j = rhs.values.size() - 1; j >= 0; --j) {
            if (MathUtils::doubleIsSmaller(lhs.values[i], rhs.values[j])) {
                lowerProb += (rhs.probabilities[j] * lhs.probabilities[i]);
            } else {
                break;
            }
        }
    }

    res.assignBernoulli(lowerProb);
}

void GreaterEqualsExpression::evaluateToPD(DiscretePD& res,
                                           State const& current,
                                           ActionState const& actions) const {
    assert(exprs.size() == 2);

    DiscretePD lhs;
    exprs[0]->evaluateToPD(lhs, current, actions);
    assert(lhs.isWellDefined());

    DiscretePD rhs;
    exprs[1]->evaluateToPD(rhs, current, actions);
    assert(rhs.isWellDefined());

    double greaterEqualProb = 0.0;
    for (unsigned int i = 0; i < lhs.values.size(); ++i) {
        for (unsigned int j = 0; j < rhs.values.size(); ++j) {
            if (MathUtils::doubleIsGreaterOrEqual(lhs.values[i],
                                                  rhs.values[j])) {
                greaterEqualProb +=
                    (rhs.probabilities[j] * lhs.probabilities[i]);
            } else {
                break;
            }
        }
    }

    res.assignBernoulli(greaterEqualProb);
}

void LowerEqualsExpression::evaluateToPD(DiscretePD& res, State const& current,
                                         ActionState const& actions) const {
    assert(exprs.size() == 2);

    DiscretePD lhs;
    exprs[0]->evaluateToPD(lhs, current, actions);
    assert(lhs.isWellDefined());

    DiscretePD rhs;
    exprs[1]->evaluateToPD(rhs, current, actions);
    assert(rhs.isWellDefined());

    double lowerEqualProb = 0.0;
    for (unsigned int i = 0; i < lhs.values.size(); ++i) {
        for (int j = rhs.values.size() - 1; j >= 0; --j) {
            if (MathUtils::doubleIsSmallerOrEqual(lhs.values[i],
                                                  rhs.values[j])) {
                lowerEqualProb += (rhs.probabilities[j] * lhs.probabilities[i]);
            } else {
                break;
            }
        }
    }

    res.assignBernoulli(lowerEqualProb);
}

void Addition::evaluateToPD(DiscretePD& res, State const& current,
                            ActionState const& actions) const {
    exprs[0]->evaluateToPD(res, current, actions);
    assert(res.isWellDefined());

    for (unsigned int index = 1; index < exprs.size(); ++index) {
        DiscretePD exprRes;
        exprs[index]->evaluateToPD(exprRes, current, actions);
        assert(exprRes.isWellDefined());

        // Merge the results
        std::map<double, double> valProbPairs;
        for (unsigned int i = 0; i < res.values.size(); ++i) {
            for (unsigned int j = 0; j < exprRes.values.size(); ++j) {
                double val = res.values[i] + exprRes.values[j];
                if (valProbPairs.find(val) == valProbPairs.end()) {
                    valProbPairs[val] = 0.0;
                }
                valProbPairs[val] +=
                    (res.probabilities[i] * exprRes.probabilities[j]);
            }
        }
        res.assignDiscrete(valProbPairs);
    }
    assert(res.isWellDefined());
}

void Subtraction::evaluateToPD(DiscretePD& res, State const& current,
                               ActionState const& actions) const {
    exprs[0]->evaluateToPD(res, current, actions);
    assert(res.isWellDefined());

    for (unsigned int index = 1; index < exprs.size(); ++index) {
        DiscretePD exprRes;
        exprs[index]->evaluateToPD(exprRes, current, actions);
        assert(exprRes.isWellDefined());

        // Merge the results
        std::map<double, double> valProbPairs;
        for (unsigned int i = 0; i < res.values.size(); ++i) {
            for (unsigned int j = 0; j < exprRes.values.size(); ++j) {
                double val = res.values[i] - exprRes.values[j];
                if (valProbPairs.find(val) == valProbPairs.end()) {
                    valProbPairs[val] = 0.0;
                }
                valProbPairs[val] +=
                    (res.probabilities[i] * exprRes.probabilities[j]);
            }
        }
        res.assignDiscrete(valProbPairs);
    }
    assert(res.isWellDefined());
}

void Multiplication::evaluateToPD(DiscretePD& res, State const& current,
                                  ActionState const& actions) const {
    exprs[0]->evaluateToPD(res, current, actions);
    assert(res.isWellDefined());

    for (unsigned int index = 1; index < exprs.size(); ++index) {
        DiscretePD exprRes;
        exprs[index]->evaluateToPD(exprRes, current, actions);
        assert(exprRes.isWellDefined());

        // Merge the results
        std::map<double, double> valProbPairs;
        for (unsigned int i = 0; i < res.values.size(); ++i) {
            for (unsigned int j = 0; j < exprRes.values.size(); ++j) {
                double val = res.values[i] * exprRes.values[j];
                if (valProbPairs.find(val) == valProbPairs.end()) {
                    valProbPairs[val] = 0.0;
                }
                valProbPairs[val] +=
                    (res.probabilities[i] * exprRes.probabilities[j]);
            }
        }
        res.assignDiscrete(valProbPairs);
    }
    assert(res.isWellDefined());
}

void Division::evaluateToPD(DiscretePD& res, State const& current,
                            ActionState const& actions) const {
    exprs[0]->evaluateToPD(res, current, actions);
    assert(res.isWellDefined());

    for (unsigned int index = 1; index < exprs.size(); ++index) {
        DiscretePD exprRes;
        exprs[index]->evaluateToPD(exprRes, current, actions);
        assert(exprRes.isWellDefined());

        // Merge the results
        std::map<double, double> valProbPairs;
        for (unsigned int i = 0; i < res.values.size(); ++i) {
            for (unsigned int j = 0; j < exprRes.values.size(); ++j) {
                assert(!MathUtils::doubleIsEqual(exprRes.values[j], 0.0));
                double val = res.values[i] / exprRes.values[j];
                if (valProbPairs.find(val) == valProbPairs.end()) {
                    valProbPairs[val] = 0.0;
                }
                valProbPairs[val] +=
                    (res.probabilities[i] * exprRes.probabilities[j]);
            }
        }
        res.assignDiscrete(valProbPairs);
    }
    assert(res.isWellDefined());
}

/*****************************************************************
                          Unaries
*****************************************************************/

void Negation::evaluateToPD(DiscretePD& res, State const& current,
                            ActionState const& actions) const {
    DiscretePD exprRes;
    expr->evaluateToPD(exprRes, current, actions);
    assert(exprRes.isWellDefined());
    res.assignBernoulli(exprRes.falsityProbability());
}

void ExponentialFunction::evaluateToPD(DiscretePD& res, State const& current,
                                       ActionState const& actions) const {
    expr->evaluateToPD(res, current, actions);

    for (unsigned int i = 0; i < res.values.size(); ++i) {
        res.values[i] = std::exp(res.values[i]);
    }
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

void BernoulliDistribution::evaluateToPD(DiscretePD& res, State const& current,
                                         ActionState const& actions) const {
    DiscretePD exprRes;
    expr->evaluateToPD(exprRes, current, actions);
    assert(exprRes.isWellDefined());

    // the expression must evaluateToPD to a real number which is converted to
    // the
    // probability that this is true
    assert(exprRes.isDeterministic());
    res.assignBernoulli(exprRes.values[0]);
}

void DiscreteDistribution::evaluateToPD(DiscretePD& res, State const& current,
                                        ActionState const& actions) const {
    std::map<double, double> valProbPairs;
    res.reset();
    for (unsigned int i = 0; i < values.size(); ++i) {
        DiscretePD val;
        values[i]->evaluateToPD(val, current, actions);

        DiscretePD prob;
        probabilities[i]->evaluateToPD(prob, current, actions);

        // Both value and prob must be determinstic
        assert(val.isDeterministic());
        assert(prob.isDeterministic());

        if (MathUtils::doubleIsGreater(prob.values[0], 0.0)) {
            if (valProbPairs.find(val.values[0]) == valProbPairs.end()) {
                valProbPairs[val.values[0]] = 0.0;
            }
            valProbPairs[val.values[0]] += prob.values[0];
        }
    }

    res.assignDiscrete(valProbPairs);
    assert(res.isWellDefined());
}

/*****************************************************************
                         Conditionals
*****************************************************************/

void MultiConditionChecker::evaluateToPD(DiscretePD& res, State const& current,
                                         ActionState const& actions) const {
    std::map<double, double> valProbPairs;
    double remainingProb = 1.0;

    for (unsigned int index = 0; index < conditions.size(); ++index) {
        DiscretePD prob;
        conditions[index]->evaluateToPD(prob, current, actions);
        assert(prob.isWellDefined());

        if (!prob.isFalsity()) {
            DiscretePD exprRes;
            effects[index]->evaluateToPD(exprRes, current, actions);
            assert(exprRes.isWellDefined());

            for (unsigned int i = 0; i < exprRes.values.size(); ++i) {
                if (valProbPairs.find(exprRes.values[i]) ==
                    valProbPairs.end()) {
                    valProbPairs[exprRes.values[i]] = 0.0;
                }
                valProbPairs[exprRes.values[i]] +=
                    (prob.truthProbability() * remainingProb *
                     exprRes.probabilities[i]);
            }
        }

        remainingProb *= prob.falsityProbability();
        if (MathUtils::doubleIsEqual(remainingProb, 0.0)) {
            break;
        }
    }

    res.assignDiscrete(valProbPairs);
    assert(res.isWellDefined());
}
