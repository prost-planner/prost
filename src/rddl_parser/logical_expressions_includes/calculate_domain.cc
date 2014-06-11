void LogicalExpression::calculateDomain(vector<set<double> > const& /*domains*/,
        ActionState const& /*actions*/,
        set<double>& /*res*/) {
    assert(false);
}

/*****************************************************************
                           Atomics
*****************************************************************/

void StateFluent::calculateDomain(vector<set<double> > const& domains,
        ActionState const& /*actions*/,
        set<double>& res) {
    assert(res.empty());
    assert(index < domains.size());
    assert(!domains[index].empty());
    res.insert(domains[index].begin(), domains[index].end());
}

void ActionFluent::calculateDomain(vector<set<double> > const& /*domains*/,
        ActionState const& actions,
        set<double>& res) {
    assert(res.empty());
    assert(index < actions.state.size());
    res.insert(actions[index]);
}

void NumericConstant::calculateDomain(vector<set<double> > const& /*domains*/,
        ActionState const& /*actions*/,
        set<double>& res) {
    assert(res.empty());
    res.insert(value);
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Conjunction::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions, set<double>& res) {
    assert(res.empty());

    // This must be true if all variables are true
    bool mustBeTrue = true;

    for (unsigned int i = 0; i < exprs.size(); ++i) {
        res.clear();
        exprs[i]->calculateDomain(domains, actions, res);
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

void Disjunction::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        set<double>& res) {
    assert(res.empty());

    // This must be false if all variables are false
    bool mustBeFalse = true;

    for (unsigned int i = 0; i < exprs.size(); ++i) {
        res.clear();
        exprs[i]->calculateDomain(domains, actions, res);
        if ((res.size() == 1) &&
            !MathUtils::doubleIsEqual(*res.begin(), 0.0)) {
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

void EqualsExpression::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> lhs;
    exprs[0]->calculateDomain(domains, actions, lhs);

    set<double> rhs;
    exprs[1]->calculateDomain(domains, actions, rhs);

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

void GreaterExpression::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> lhs;
    exprs[0]->calculateDomain(domains, actions, lhs);

    set<double> rhs;
    exprs[1]->calculateDomain(domains, actions, rhs);

    // If the largest lhs is bigger than the smallest rhs this can be true
    if (MathUtils::doubleIsGreater(*lhs.rbegin(), *rhs.begin())) {
        res.insert(1.0);
    }

    // If the smallest lhs is not bigger than the largest rhs this can be false
    if (!MathUtils::doubleIsGreater(*lhs.begin(), *rhs.rbegin())) {
        res.insert(0.0);
    }
}

void LowerExpression::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> lhs;
    exprs[0]->calculateDomain(domains, actions, lhs);

    set<double> rhs;
    exprs[1]->calculateDomain(domains, actions, rhs);

    if (MathUtils::doubleIsSmaller(*lhs.rbegin(), *rhs.begin())) {
        res.insert(1.0);
    }

    if (!MathUtils::doubleIsSmaller(*lhs.begin(), *rhs.rbegin())) {
        res.insert(0.0);
    }
}

void GreaterEqualsExpression::calculateDomain(
        vector<set<double> > const& domains, ActionState const& actions,
        set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> lhs;
    exprs[0]->calculateDomain(domains, actions, lhs);

    set<double> rhs;
    exprs[1]->calculateDomain(domains, actions, rhs);

    if (MathUtils::doubleIsGreaterOrEqual(*lhs.rbegin(), *rhs.begin())) {
        res.insert(1.0);
    }

    if (!MathUtils::doubleIsGreaterOrEqual(*lhs.begin(), *rhs.rbegin())) {
        res.insert(0.0);
    }
}

void LowerEqualsExpression::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> lhs;
    exprs[0]->calculateDomain(domains, actions, lhs);

    set<double> rhs;
    exprs[1]->calculateDomain(domains, actions, rhs);

    if (MathUtils::doubleIsSmallerOrEqual(*lhs.rbegin(), *rhs.begin())) {
        res.insert(1.0);
    }

    if (!MathUtils::doubleIsSmallerOrEqual(*lhs.begin(), *rhs.rbegin())) {
        res.insert(0.0);
    }
}

void Addition::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        set<double>& res) {
    assert(res.empty());
    set<double> sums;
    exprs[0]->calculateDomain(domains, actions, sums);

    for (unsigned int i = 1; i < exprs.size(); ++i) {
        set<double> element;
        exprs[i]->calculateDomain(domains, actions, element);
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

void Subtraction::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> lhs;
    exprs[0]->calculateDomain(domains, actions, lhs);

    set<double> rhs;
    exprs[1]->calculateDomain(domains, actions, rhs);
    for (set<double>::iterator it = lhs.begin(); it != lhs.end(); ++it) {
        for (set<double>::iterator it2 = rhs.begin(); it2 != rhs.end();
             ++it2) {
            res.insert(*it - *it2);
        }
    }
}

void Multiplication::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        set<double>& res) {
    assert(res.empty());
    set<double> prods;
    exprs[0]->calculateDomain(domains, actions, prods);

    for (unsigned int i = 1; i < exprs.size(); ++i) {
        set<double> element;
        exprs[i]->calculateDomain(domains, actions, element);
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

void Division::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> lhs;
    exprs[0]->calculateDomain(domains, actions, lhs);

    set<double> rhs;
    exprs[1]->calculateDomain(domains, actions, rhs);
    for (set<double>::iterator it = lhs.begin(); it != lhs.end(); ++it) {
        for (set<double>::iterator it2 = rhs.begin(); it2 != rhs.end();
             ++it2) {
            res.insert(*it / *it2);
        }
    }
}

/*****************************************************************
                          Unaries
*****************************************************************/

void Negation::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        set<double>& res) {
    set<double> tmp;
    expr->calculateDomain(domains, actions, tmp);

    if (tmp.find(0.0) != tmp.end()) {
        res.insert(1.0);
    }
    if ((tmp.size() > 1) || (tmp.find(0.0) == tmp.end())) {
        res.insert(0.0);
    }
}

void ExponentialFunction::calculateDomain(vector<set<double> > const& domains,
                                          ActionState const& actions,
                                          set<double>& res) {
    set<double> tmp;
    expr->calculateDomain(domains, actions, tmp);

    for (set<double>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
        res.insert(std::exp(*it));
    }
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

void BernoulliDistribution::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        set<double>& res) {
    set<double> tmp;
    expr->calculateDomain(domains, actions, tmp);

    for (set<double>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
        if (!MathUtils::doubleIsEqual(*it, 0.0)) {
            res.insert(1.0);
        }

        if (MathUtils::doubleIsGreaterOrEqual(*it,
                    0.0) && MathUtils::doubleIsSmaller(*it, 1.0)) {
            res.insert(0.0);
        }

        if (res.size() == 2) {
            break;
        }
    }
}

void DiscreteDistribution::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        set<double>& res) {
    for (unsigned int i = 0; i < values.size(); ++i) {
        set<double> probs;
        probabilities[i]->calculateDomain(domains, actions, probs);
        if ((probs.size() > 1) || (probs.find(0.0) == probs.end())) {
            set<double> vals;
            values[i]->calculateDomain(domains, actions, vals);
            res.insert(vals.begin(), vals.end());
        }
    }
}

/*****************************************************************
                         Conditionals
*****************************************************************/

void IfThenElseExpression::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        set<double>& res) {
    set<double> cond;
    condition->calculateDomain(domains, actions, cond);
    if (cond.size() > 1) {
        // cond has more than one value, one of which must represent 'true'
        valueIfTrue->calculateDomain(domains, actions, res);
        if (cond.find(0.0) != cond.end()) {
            // and false is also possible
            set<double> tmp;
            valueIfFalse->calculateDomain(domains, actions, tmp);
            res.insert(tmp.begin(), tmp.end());
        }
    } else if (cond.find(0.0) != cond.end()) {
        // there is only one value in cond which is 'false'
        valueIfFalse->calculateDomain(domains, actions, res);
    } else {
        valueIfTrue->calculateDomain(domains, actions, res);
    }
}

void MultiConditionChecker::calculateDomain(vector<set<double> > const& domains,
        ActionState const& actions,
        set<double>& res) {
    for (unsigned int i = 0; i < conditions.size(); ++i) {
        set<double> cond;
        conditions[i]->calculateDomain(domains, actions, cond);
        if (cond.size() > 1) {
            // cond has more than one value, one of which must represent 'true'
            set<double> tmp;
            effects[i]->calculateDomain(domains, actions, tmp);
            res.insert(tmp.begin(), tmp.end());
            if (cond.find(0.0) == cond.end()) {
                // all values in cond represent 'true'
                return;
            }
        } else if (cond.find(0.0) == cond.end()) {
            // there is only one value in cond, and it is true
            set<double> tmp;
            effects[i]->calculateDomain(domains, actions, tmp);
            res.insert(tmp.begin(), tmp.end());
            return;
        }
    }
}
