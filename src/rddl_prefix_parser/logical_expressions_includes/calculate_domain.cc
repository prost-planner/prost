void LogicalExpression::calculateDomain(Domains const& /*domains*/,
                                        ActionState const& /*action*/,
                                        set<double>& /*res*/) {
    assert(false);
}

/*****************************************************************
                           Atomics
*****************************************************************/

void StateFluent::calculateDomain(Domains const& domains,
                                  ActionState const& /*action*/,
                                  set<double>& res) {
    assert(res.empty());
    assert(index < domains.size());
    assert(!domains[index].empty());
    res.insert(domains[index].begin(), domains[index].end());
}

void ActionFluent::calculateDomain(Domains const& /*domains*/,
                                   ActionState const& action,
                                   set<double>& res) {
    assert(res.empty());
    assert(index < action.state.size());
    res.insert(action[index]);
}

void NumericConstant::calculateDomain(Domains const& /*domains*/,
                                      ActionState const& /*action*/,
                                      set<double>& res) {
    assert(res.empty());
    res.insert(value);
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Conjunction::calculateDomain(Domains const& domains,
                                  ActionState const& action, set<double>& res) {
    assert(res.empty());

    // This must be true if all variables are true
    bool mustBeTrue = true;

    for (unsigned int i = 0; i < exprs.size(); ++i) {
        res.clear();
        exprs[i]->calculateDomain(domains, action, res);
        if ((res.size() == 1) && MathUtils::doubleIsEqual(*res.begin(), 0.0)) {
            // This element and the whole conjunction must be false
            return;
        }

        // This element might be false
        if (res.find(0.0) != res.end()) {
            mustBeTrue = false;
        }
    }

    res.clear();
    res.insert(1.0);
    if (!mustBeTrue) {
        res.insert(0.0);
    }
}

void Disjunction::calculateDomain(Domains const& domains,
                                  ActionState const& action, set<double>& res) {
    assert(res.empty());

    // This must be false if all variables are false
    bool mustBeFalse = true;

    for (unsigned int i = 0; i < exprs.size(); ++i) {
        res.clear();
        exprs[i]->calculateDomain(domains, action, res);
        if ((res.size() == 1) && !MathUtils::doubleIsEqual(*res.begin(), 0.0)) {
            // This element and the whole conjunction must be true
            res.clear();
            res.insert(1.0);
            return;
        }

        // This element might be true
        if ((res.find(0.0) == res.end()) || res.size() > 1) {
            mustBeFalse = false;
        }
    }

    res.clear();
    res.insert(0.0);
    if (!mustBeFalse) {
        res.insert(1.0);
    }
}

void EqualsExpression::calculateDomain(Domains const& domains,
                                       ActionState const& action,
                                       set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> lhs;
    exprs[0]->calculateDomain(domains, action, lhs);

    set<double> rhs;
    exprs[1]->calculateDomain(domains, action, rhs);

    if (lhs.size() != rhs.size()) {
        res.insert(0.0);
    }

    for (set<double>::iterator it = lhs.begin(); it != lhs.end(); ++it) {
        if (rhs.find(*it) != rhs.end()) {
            res.insert(1.0);
        } else {
            res.insert(0.0);
        }

        if (res.size() == 2) {
            break;
        }
    }
}

void GreaterExpression::calculateDomain(Domains const& domains,
                                        ActionState const& action,
                                        set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> lhs;
    exprs[0]->calculateDomain(domains, action, lhs);

    set<double> rhs;
    exprs[1]->calculateDomain(domains, action, rhs);

    // If the largest lhs is bigger than the smallest rhs this can be true
    if (MathUtils::doubleIsGreater(*lhs.rbegin(), *rhs.begin())) {
        res.insert(1.0);
    }

    // If the smallest lhs is not bigger than the largest rhs this can be false
    if (!MathUtils::doubleIsGreater(*lhs.begin(), *rhs.rbegin())) {
        res.insert(0.0);
    }
}

void LowerExpression::calculateDomain(Domains const& domains,
                                      ActionState const& action,
                                      set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> lhs;
    exprs[0]->calculateDomain(domains, action, lhs);

    set<double> rhs;
    exprs[1]->calculateDomain(domains, action, rhs);

    if (MathUtils::doubleIsSmaller(*lhs.rbegin(), *rhs.begin())) {
        res.insert(1.0);
    }

    if (!MathUtils::doubleIsSmaller(*lhs.begin(), *rhs.rbegin())) {
        res.insert(0.0);
    }
}

void GreaterEqualsExpression::calculateDomain(Domains const& domains,
                                              ActionState const& action,
                                              set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> lhs;
    exprs[0]->calculateDomain(domains, action, lhs);

    set<double> rhs;
    exprs[1]->calculateDomain(domains, action, rhs);

    if (MathUtils::doubleIsGreaterOrEqual(*lhs.rbegin(), *rhs.begin())) {
        res.insert(1.0);
    }

    if (!MathUtils::doubleIsGreaterOrEqual(*lhs.begin(), *rhs.rbegin())) {
        res.insert(0.0);
    }
}

void LowerEqualsExpression::calculateDomain(Domains const& domains,
                                            ActionState const& action,
                                            set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> lhs;
    exprs[0]->calculateDomain(domains, action, lhs);

    set<double> rhs;
    exprs[1]->calculateDomain(domains, action, rhs);

    if (MathUtils::doubleIsSmallerOrEqual(*lhs.rbegin(), *rhs.begin())) {
        res.insert(1.0);
    }

    if (!MathUtils::doubleIsSmallerOrEqual(*lhs.begin(), *rhs.rbegin())) {
        res.insert(0.0);
    }
}

void Addition::calculateDomain(Domains const& domains,
                               ActionState const& action, set<double>& res) {
    assert(res.empty());
    set<double> sums;
    exprs[0]->calculateDomain(domains, action, sums);

    for (unsigned int i = 1; i < exprs.size(); ++i) {
        set<double> element;
        exprs[i]->calculateDomain(domains, action, element);
        res.clear();

        for (set<double>::iterator it = sums.begin(); it != sums.end(); ++it) {
            for (set<double>::iterator it2 = element.begin();
                 it2 != element.end(); ++it2) {
                res.insert(*it + *it2);
            }
        }
        sums.clear();
        sums = res;
    }
}

void Subtraction::calculateDomain(Domains const& domains,
                                  ActionState const& action, set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> lhs;
    exprs[0]->calculateDomain(domains, action, lhs);

    set<double> rhs;
    exprs[1]->calculateDomain(domains, action, rhs);
    for (set<double>::iterator it = lhs.begin(); it != lhs.end(); ++it) {
        for (set<double>::iterator it2 = rhs.begin(); it2 != rhs.end(); ++it2) {
            res.insert(*it - *it2);
        }
    }
}

void Multiplication::calculateDomain(Domains const& domains,
                                     ActionState const& action,
                                     set<double>& res) {
    assert(res.empty());
    set<double> prods;
    exprs[0]->calculateDomain(domains, action, prods);

    for (unsigned int i = 1; i < exprs.size(); ++i) {
        set<double> element;
        exprs[i]->calculateDomain(domains, action, element);
        res.clear();

        for (set<double>::iterator it = prods.begin(); it != prods.end();
             ++it) {
            for (set<double>::iterator it2 = element.begin();
                 it2 != element.end(); ++it2) {
                res.insert(*it * *it2);
            }
        }
        prods.clear();
        prods = res;
    }
}

void Division::calculateDomain(Domains const& domains,
                               ActionState const& action, set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> lhs;
    exprs[0]->calculateDomain(domains, action, lhs);

    set<double> rhs;
    exprs[1]->calculateDomain(domains, action, rhs);
    for (set<double>::iterator it = lhs.begin(); it != lhs.end(); ++it) {
        for (set<double>::iterator it2 = rhs.begin(); it2 != rhs.end(); ++it2) {
            res.insert(*it / *it2);
        }
    }
}

/*****************************************************************
                          Unaries
*****************************************************************/

void Negation::calculateDomain(Domains const& domains,
                               ActionState const& action, set<double>& res) {
    set<double> tmp;
    expr->calculateDomain(domains, action, tmp);

    if (tmp.find(0.0) != tmp.end()) {
        res.insert(1.0);
    }
    if ((tmp.size() > 1) || (tmp.find(0.0) == tmp.end())) {
        res.insert(0.0);
    }
}

void ExponentialFunction::calculateDomain(Domains const& domains,
                                          ActionState const& action,
                                          set<double>& res) {
    set<double> tmp;
    expr->calculateDomain(domains, action, tmp);

    for (set<double>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
        res.insert(std::exp(*it));
    }
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

void BernoulliDistribution::calculateDomain(Domains const& domains,
                                            ActionState const& action,
                                            set<double>& res) {
    set<double> tmp;
    expr->calculateDomain(domains, action, tmp);

    for (set<double>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
        if (!MathUtils::doubleIsEqual(*it, 0.0)) {
            res.insert(1.0);
        }

        if (MathUtils::doubleIsGreaterOrEqual(*it, 0.0) &&
            MathUtils::doubleIsSmaller(*it, 1.0)) {
            res.insert(0.0);
        }

        if (res.size() == 2) {
            break;
        }
    }
}

void DiscreteDistribution::calculateDomain(Domains const& domains,
                                           ActionState const& action,
                                           set<double>& res) {
    for (unsigned int i = 0; i < values.size(); ++i) {
        set<double> probs;
        probabilities[i]->calculateDomain(domains, action, probs);
        if ((probs.size() > 1) || (probs.find(0.0) == probs.end())) {
            set<double> vals;
            values[i]->calculateDomain(domains, action, vals);
            res.insert(vals.begin(), vals.end());
        }
    }
}

/*****************************************************************
                         Conditionals
*****************************************************************/

void IfThenElseExpression::calculateDomain(Domains const& domains,
                                           ActionState const& action,
                                           set<double>& res) {
    set<double> cond;
    condition->calculateDomain(domains, action, cond);
    if (cond.size() > 1) {
        // cond has more than one value, one of which must represent 'true'
        valueIfTrue->calculateDomain(domains, action, res);
        if (cond.find(0.0) != cond.end()) {
            // and false is also possible
            set<double> tmp;
            valueIfFalse->calculateDomain(domains, action, tmp);
            res.insert(tmp.begin(), tmp.end());
        }
    } else if (cond.find(0.0) != cond.end()) {
        // there is only one value in cond which is 'false'
        valueIfFalse->calculateDomain(domains, action, res);
    } else {
        valueIfTrue->calculateDomain(domains, action, res);
    }
}

void MultiConditionChecker::calculateDomain(Domains const& domains,
                                            ActionState const& action,
                                            set<double>& res) {
    for (unsigned int i = 0; i < conditions.size(); ++i) {
        set<double> cond;
        conditions[i]->calculateDomain(domains, action, cond);
        if (cond.size() > 1) {
            // cond has more than one value, one of which must represent 'true'
            set<double> tmp;
            effects[i]->calculateDomain(domains, action, tmp);
            res.insert(tmp.begin(), tmp.end());
            if (cond.find(0.0) == cond.end()) {
                // all values in cond represent 'true'
                return;
            }
        } else if (cond.find(0.0) == cond.end()) {
            // there is only one value in cond, and it is true
            set<double> tmp;
            effects[i]->calculateDomain(domains, action, tmp);
            res.insert(tmp.begin(), tmp.end());
            return;
        }
    }
}
