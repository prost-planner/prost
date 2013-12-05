void LogicalExpression::calculateDomain(ActionState const& /*actions*/, set<double>& /*res*/) {
    assert(false);
}

void StateFluent::calculateDomain(ActionState const& /*actions*/, set<double>& res) {
    res.insert(0.0);
    res.insert(1.0);
}

void ActionFluent::calculateDomain(ActionState const& actions, set<double>& res) {
    assert(index < actions.state.size());
    res.insert(actions[index]);
}

void NumericConstant::calculateDomain(ActionState const& /*actions*/, set<double>& res) {
    res.insert(value);
}

void Conjunction::calculateDomain(ActionState const& actions, set<double>& res) {
    assert(res.empty());
    set<double> tmp1;
    tmp1.insert(1.0);
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        res.clear();
        set<double> tmp;
        exprs[i]->calculateDomain(actions, tmp);
        for(set<double>::iterator it = tmp1.begin(); it != tmp1.end(); ++it) {
            for(set<double>::iterator it2 = tmp.begin(); it2 != tmp.end(); ++it2) {
                res.insert(*it * *it2);
            }
        }
        tmp1.clear();
        tmp1 = res;
    }
}

void Disjunction::calculateDomain(ActionState const& actions, set<double>& res) {
    assert(res.empty());
    set<double> tmp1;
    tmp1.insert(1.0);
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        res.clear();
        set<double> tmp;
        exprs[i]->calculateDomain(actions, tmp);
        for(set<double>::iterator it = tmp1.begin(); it != tmp1.end(); ++it) {
            for(set<double>::iterator it2 = tmp.begin(); it2 != tmp.end(); ++it2) {
                res.insert(*it * (1-0-*it2));
            }
        }
        tmp1.clear();
        tmp1 = res;
    }

    res.clear();
    for(set<double>::iterator it = tmp1.begin(); it != tmp1.end(); ++it) {
        res.insert(1.0 - *it);
    }
}

void EqualsExpression::calculateDomain(ActionState const& actions, set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> childRes;
    exprs[0]->calculateDomain(actions, childRes);

    set<double> childRes2;
    exprs[1]->calculateDomain(actions, childRes2);

    if(childRes.size() == 1 || childRes2.size() == 1) {
        if(MathUtils::doubleIsEqual(*childRes.begin(),*childRes2.begin())) {
            res.insert(1.0);
        } else {
            res.insert(0.0);
        }
        return;
    }

    res.insert(0.0);
    for(set<double>::iterator it = childRes.begin(); it != childRes.end(); ++it) {
        if(childRes2.find(*it) != childRes2.end()) {
            res.insert(1.0);
            break;
        }
    }

}

void GreaterExpression::calculateDomain(ActionState const& actions, set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> childRes;
    exprs[0]->calculateDomain(actions, childRes);

    set<double> childRes2;
    exprs[1]->calculateDomain(actions, childRes2);

    if(MathUtils::doubleIsGreater(*childRes.rbegin(),*childRes2.begin())) {
        res.insert(1.0);
    } else {
        res.insert(0.0);
    }

    if(!MathUtils::doubleIsGreater(*childRes.begin(),*childRes2.rbegin())) {
        res.insert(0.0);
    } else {
        res.insert(1.0);
    }
}

void LowerExpression::calculateDomain(ActionState const& actions, set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> childRes;
    exprs[0]->calculateDomain(actions, childRes);

    set<double> childRes2;
    exprs[1]->calculateDomain(actions, childRes2);

    if(MathUtils::doubleIsSmaller(*childRes.rbegin(),*childRes2.begin())) {
        res.insert(1.0);
    } else {
        res.insert(0.0);
    }

    if(!MathUtils::doubleIsSmaller(*childRes.begin(),*childRes2.rbegin())) {
        res.insert(0.0);
    } else {
        res.insert(1.0);
    } 
}

void GreaterEqualsExpression::calculateDomain(ActionState const& actions, set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> childRes;
    exprs[0]->calculateDomain(actions, childRes);

    set<double> childRes2;
    exprs[1]->calculateDomain(actions, childRes2);

    if(MathUtils::doubleIsGreaterOrEqual(*childRes.rbegin(),*childRes2.begin())) {
        res.insert(1.0);
    } else {
        res.insert(0.0);
    }

    if(!MathUtils::doubleIsGreaterOrEqual(*childRes.begin(),*childRes2.rbegin())) {
        res.insert(0.0);
    } else {
        res.insert(1.0);
    }
}

void LowerEqualsExpression::calculateDomain(ActionState const& actions, set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());
    set<double> childRes;
    exprs[0]->calculateDomain(actions, childRes);

    set<double> childRes2;
    exprs[1]->calculateDomain(actions, childRes2);

    if(MathUtils::doubleIsSmallerOrEqual(*childRes.rbegin(),*childRes2.begin())) {
        res.insert(1.0);
    } else {
        res.insert(0.0);
    }

    if(!MathUtils::doubleIsSmallerOrEqual(*childRes.begin(),*childRes2.rbegin())) {
        res.insert(0.0);
    } else {
        res.insert(1.0);
    } 
}

void Addition::calculateDomain(ActionState const& actions, set<double>& res) {
    assert(res.empty());
    set<double> childRes;
    exprs[0]->calculateDomain(actions, childRes);

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        res.clear();
        set<double> childRes2;
        exprs[i]->calculateDomain(actions, childRes2);
        for(set<double>::iterator it = childRes.begin(); it != childRes.end(); ++it) {
            for(set<double>::iterator it2 = childRes2.begin(); it2 != childRes2.end(); ++it2) {
                res.insert(*it + *it2);
            }
        }
        childRes.clear();
        childRes = res;
    }
}

void Subtraction::calculateDomain(ActionState const& actions, set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());
    set<double> childRes1;
    exprs[0]->calculateDomain(actions, childRes1);
    set<double> childRes2;
    exprs[1]->calculateDomain(actions, childRes2);
    for(set<double>::iterator it = childRes1.begin(); it != childRes1.end(); ++it) {
        for(set<double>::iterator it2 = childRes2.begin(); it2 != childRes2.end(); ++it2) {
            res.insert(*it - *it2);
        }
    }
}

void Multiplication::calculateDomain(ActionState const& actions, set<double>& res) {
    assert(res.empty());
    set<double> childRes;
    exprs[0]->calculateDomain(actions, childRes);

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        res.clear();
        set<double> childRes2;
        exprs[i]->calculateDomain(actions, childRes2);
        for(set<double>::iterator it = childRes.begin(); it != childRes.end(); ++it) {
            for(set<double>::iterator it2 = childRes2.begin(); it2 != childRes2.end(); ++it2) {
                res.insert(*it * *it2);
            }
        }
        childRes.clear();
        childRes = res;
    }
}

void Division::calculateDomain(ActionState const& actions, set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> childRes1;
    exprs[0]->calculateDomain(actions, childRes1);
    set<double> childRes2;
    exprs[1]->calculateDomain(actions, childRes2);
    for(set<double>::iterator it = childRes1.begin(); it != childRes1.end(); ++it) {
        for(set<double>::iterator it2 = childRes2.begin(); it2 != childRes2.end(); ++it2) {
            res.insert(*it / *it2);
        }
    }

}

void BernoulliDistribution::calculateDomain(ActionState const& actions, set<double>& res) {
    expr->calculateDomain(actions,res);
}

void IfThenElseExpression::calculateDomain(ActionState const& actions, set<double>& res) {
    assert(res.empty());
    condition->calculateDomain(actions, res);
    assert(!res.empty());
    if(res.size() == 1 && (MathUtils::doubleIsEqual(*res.begin(),0.0))) {
        res.clear();
        valueIfFalse->calculateDomain(actions, res);
    } else if(res.size() == 1 && (MathUtils::doubleIsEqual(*res.begin(),1.0))) {
        res.clear();
        valueIfTrue->calculateDomain(actions, res);
    } else {
        res.clear();
        valueIfTrue->calculateDomain(actions, res);
        set<double> tmp;
        valueIfFalse->calculateDomain(actions, tmp);
        res.insert(tmp.begin(),tmp.end());
    }
}

void MultiConditionChecker::calculateDomain(ActionState const& actions, set<double>& res) {
    for(unsigned int i = 0; i < conditions.size(); ++i) {
        set<double> tmp;
        conditions[i]->calculateDomain(actions, tmp);
        assert(!tmp.empty());
        if(tmp.size() > 1 || !MathUtils::doubleIsEqual(*tmp.begin(),0.0)) {
            tmp.clear();
            effects[i]->calculateDomain(actions, tmp);
            res.insert(tmp.begin(),tmp.end());
        }
    }
}

void NegateExpression::calculateDomain(ActionState const& actions, set<double>& res) {
    set<double> tmp;
    expr->calculateDomain(actions, tmp);
    for(set<double>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
        assert(MathUtils::doubleIsSmallerOrEqual(*it,1.0));
        res.insert(1.0 - (*it));
    }
}


