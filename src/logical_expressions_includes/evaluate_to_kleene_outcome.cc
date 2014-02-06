void LogicalExpression::evaluateToKleeneOutcome(set<double>& /*res*/, KleeneState const& /*current*/, ActionState const& /*actions*/) {
    assert(false);
}

/*****************************************************************
                           Atomics
*****************************************************************/

void StateFluent::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& /*actions*/) {
    assert(res.empty());
    res.insert(current[index].begin(), current[index].end());
}

void ActionFluent::evaluateToKleeneOutcome(set<double>& res, KleeneState const& /*current*/, ActionState const& actions) {
    assert(res.empty());
    res.insert(actions[index]);
}

void NumericConstant::evaluateToKleeneOutcome(set<double>& res, KleeneState const& /*current*/, ActionState const& /*actions*/) {
    assert(res.empty());
    res.insert(value);
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Conjunction::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    assert(res.empty());
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        set<double> tmp;
        exprs[i]->evaluateToKleeneOutcome(tmp, current, actions);
        if(tmp.size() == 1) {
            if(MathUtils::doubleIsEqual(*tmp.begin(), 0.0)) {
                res.clear();
                res.insert(0.0);
                return;
            } else {
                res.insert(1.0);
            }
        } else {
            if(tmp.find(0.0) != tmp.end()) {
                res.insert(0.0);
            }
            res.insert(1.0);
        }
    }
    assert(res.size() == 1 || res.size() == 2);
}

void Disjunction::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    assert(res.empty());
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        set<double> tmp;
        exprs[i]->evaluateToKleeneOutcome(tmp, current, actions);
        if(tmp.size() == 1) {
            if(!MathUtils::doubleIsEqual(*tmp.begin(), 0.0)) {
                res.clear();
                res.insert(1.0);
                return;
            } else {
                res.insert(0.0);
            }
        } else {
            if(tmp.find(0.0) != tmp.end()) {
                res.insert(0.0);
            }
            res.insert(1.0);
        }
    }
    assert(res.size() == 1 || res.size() == 2);
}

void EqualsExpression::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    assert(res.empty());
    assert(exprs.size() == 2);

    set<double> lhs;
    exprs[0]->evaluateToKleeneOutcome(lhs, current, actions);
    set<double> rhs;
    exprs[1]->evaluateToKleeneOutcome(rhs, current, actions);

    if(lhs.size() == 1 && rhs.size() == 1) {
        // If both evaluate to a single value we compare those values
        if(MathUtils::doubleIsEqual(*lhs.begin(), *rhs.begin())) {
            res.insert(1.0);
        } else {
            res.insert(0.0);
        }
    } else {
        // Otherwise, they can be different, and we still have to determine if
        // they can be equal as well.
        res.insert(0.0);
        for(set<double>::iterator it = lhs.begin(); it != lhs.end(); ++it) {
            if(rhs.find(*it) != rhs.end()) {
                res.insert(1.0);
                break;
            }
        }
    }
    // print(cout);
    // cout << ": " << *res.begin() << " " << *res.rbegin() << endl;
    assert(res.size() == 1 || res.size() == 2);
}

void GreaterExpression::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    assert(res.empty());
    assert(exprs.size() == 2);

    set<double> lhs;
    exprs[0]->evaluateToKleeneOutcome(lhs, current, actions);
    set<double> rhs;
    exprs[1]->evaluateToKleeneOutcome(rhs, current, actions);

    // x can be greater than y if the biggest possible x is greater
    // than the smallest possible y
    if(MathUtils::doubleIsGreater(*lhs.rbegin(), *rhs.begin())) {
        res.insert(1.0);
    }

    // x can be "not greater" than y if the smallest possible x is not
    // greater than  the biggest possible y
    if(!MathUtils::doubleIsGreater(*lhs.begin(), *rhs.rbegin())) {
        res.insert(0.0);
    }

    assert(res.size() == 1 || res.size() == 2);
}

void LowerExpression::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    assert(res.empty());
    assert(exprs.size() == 2);

    set<double> lhs;
    exprs[0]->evaluateToKleeneOutcome(lhs, current, actions);
    set<double> rhs;
    exprs[1]->evaluateToKleeneOutcome(rhs, current, actions);

    if(MathUtils::doubleIsSmaller(*lhs.begin(), *rhs.rbegin())) {
        res.insert(1.0);
    }

    if(!MathUtils::doubleIsSmaller(*lhs.rbegin(), *rhs.begin())) {
        res.insert(0.0);
    }

    assert(res.size() == 1 || res.size() == 2);
}

void GreaterEqualsExpression::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    assert(res.empty());
    assert(exprs.size() == 2);

    set<double> lhs;
    exprs[0]->evaluateToKleeneOutcome(lhs, current, actions);
    set<double> rhs;
    exprs[1]->evaluateToKleeneOutcome(rhs, current, actions);

    if(MathUtils::doubleIsGreaterOrEqual(*lhs.rbegin(), *rhs.begin())) {
        res.insert(1.0);
    }

    if(!MathUtils::doubleIsGreaterOrEqual(*lhs.begin(), *rhs.rbegin())) {
        res.insert(0.0);
    }

    assert(res.size() == 1 || res.size() == 2);
}

void LowerEqualsExpression::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    assert(res.empty());
    assert(exprs.size() == 2);

    set<double> lhs;
    exprs[0]->evaluateToKleeneOutcome(lhs, current, actions);
    set<double> rhs;
    exprs[1]->evaluateToKleeneOutcome(rhs, current, actions);

    if(MathUtils::doubleIsSmallerOrEqual(*lhs.begin(), *rhs.rbegin())) {
        res.insert(1.0);
    }

    if(!MathUtils::doubleIsSmallerOrEqual(*lhs.rbegin(), *rhs.begin())) {
        res.insert(0.0);
    }

    assert(res.size() == 1 || res.size() == 2);
}

void Addition::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    assert(res.empty());

    set<double> lhs;
    exprs[0]->evaluateToKleeneOutcome(lhs, current, actions);

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        res.clear();
        set<double> rhs;
        exprs[i]->evaluateToKleeneOutcome(rhs, current, actions);

        for(set<double>::iterator it = lhs.begin(); it != lhs.end(); ++it) {
            for(set<double>::iterator it2 = rhs.begin(); it2 != rhs.end(); ++it2) {
                res.insert(*it + *it2);
            }
        }
        lhs.clear();
        lhs = res;
    }
}

void Subtraction::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    assert(res.empty());
    assert(exprs.size() == 2);

    set<double> lhs;
    exprs[0]->evaluateToKleeneOutcome(lhs, current, actions);
    set<double> rhs;
    exprs[1]->evaluateToKleeneOutcome(rhs, current, actions);

    for(set<double>::iterator it = lhs.begin(); it != lhs.end(); ++it) {
        for(set<double>::iterator it2 = rhs.begin(); it2 != rhs.end(); ++it2) {
            res.insert(*it - *it2);
        }
    }
}

void Multiplication::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    assert(res.empty());
    assert(exprs.size() == 2);

    set<double> lhs;
    exprs[0]->evaluateToKleeneOutcome(lhs, current, actions);
    set<double> rhs;
    exprs[1]->evaluateToKleeneOutcome(rhs, current, actions);

    for(set<double>::iterator it = lhs.begin(); it != lhs.end(); ++it) {
        for(set<double>::iterator it2 = rhs.begin(); it2 != rhs.end(); ++it2) {
            res.insert(*it / *it2);
        }
    }
}

void Division::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    assert(res.empty());
    assert(exprs.size() == 2);

    set<double> lhs;
    exprs[0]->evaluateToKleeneOutcome(lhs, current, actions);
    set<double> rhs;
    exprs[1]->evaluateToKleeneOutcome(rhs, current, actions);

    for(set<double>::iterator it = lhs.begin(); it != lhs.end(); ++it) {
        for(set<double>::iterator it2 = rhs.begin(); it2 != rhs.end(); ++it2) {
            res.insert(*it * *it2);
        }
    }
}

/*****************************************************************
                          Unaries
*****************************************************************/

void NegateExpression::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    assert(res.empty());

    expr->evaluateToKleeneOutcome(res, current, actions);

    if(res.find(0.0) == res.end()) {
        // There are only numbers != 0 -> the negation is false with
        // certainty
        res.clear();
        res.insert(0.0);
    } else if(res.size() > 1) {
        // There is a 0.0 in res, and at least one other number ->
        // both values are possible
        res.clear();
        res.insert(0.0);
        res.insert(1.0);        
    } else {
        // There is only the 0.0 in res -> the negation is true with
        // certainty
        res.clear();
        res.insert(1.0);
    }
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

void BernoulliDistribution::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    assert(res.empty());

    set<double> tmp;
    expr->evaluateToKleeneOutcome(tmp, current, actions);

    for(set<double>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
        if(MathUtils::doubleIsGreater(*it, 0.0) && MathUtils::doubleIsSmaller(*it, 1.0)) {
            res.insert(0.0);
            res.insert(1.0);
            return;
        } else if(MathUtils::doubleIsEqual(*it, 0.0)) {
            res.insert(0.0);
        } else {
            res.insert(1.0);
        }
    }
}

void DiscreteDistribution::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    for(unsigned int index = 0; index < probabilities.size(); ++index) {
        set<double> tmp;
        probabilities[index]->evaluateToKleeneOutcome(tmp, current, actions);
        assert(!tmp.empty());

        // If there is at least on value in tmp that is different from 0.0 it is
        // possible that the according value is assigned (this is due to the
        // fact that we cosider probabilities smaller than 0 and greater than 1
        // as 1)
        if((tmp.size() > 1) || (tmp.find(0.0) == tmp.end())) {
            tmp.clear();
            values[index]->evaluateToKleeneOutcome(tmp, current, actions);
            res.insert(tmp.begin(), tmp.end());
        }
    }
}

/*****************************************************************
                         Conditionals
*****************************************************************/

void IfThenElseExpression::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    assert(res.empty());

    set<double> tmp;
    condition->evaluateToKleeneOutcome(tmp, current, actions);

    if(tmp.find(0.0) != tmp.end()) {
        valueIfFalse->evaluateToKleeneOutcome(res, current, actions);
        if(tmp.size() > 1) {
            tmp.clear();
            valueIfTrue->evaluateToKleeneOutcome(tmp, current, actions);
            res.insert(tmp.begin(), tmp.end());
        }
    } else {
        valueIfTrue->evaluateToKleeneOutcome(res, current, actions);
    }
}

void MultiConditionChecker::evaluateToKleeneOutcome(set<double>& res, KleeneState const& current, ActionState const& actions) {
    //if we meet a condition that evaluates to unknown we must keep on
    //checking conditions until we find one that is always true, and
    //compare all potentially true ones with each other
    assert(res.empty());

    for(unsigned int i = 0; i < conditions.size(); ++i) {
        set<double> tmp;
        conditions[i]->evaluateToKleeneOutcome(tmp, current, actions);

        if(tmp.size() == 1) {
            if(!MathUtils::doubleIsEqual(*tmp.begin(), 0.0)) {
                // This condition must fire, so all following cases
                // are ignored
                tmp.clear();
                effects[i]->evaluateToKleeneOutcome(tmp, current, actions);
                res.insert(tmp.begin(), tmp.end());
                return;
            } else {
                continue;
            }
        }

        // This condition can fire
        tmp.clear();
        effects[i]->evaluateToKleeneOutcome(tmp, current, actions);
        res.insert(tmp.begin(), tmp.end());
    }
    assert(false);
}


