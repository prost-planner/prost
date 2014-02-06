void LogicalExpression::calculateProbDomain(ActionState const& /*actions*/, set<double>& /*res*/) {
    assert(false);
}

/*****************************************************************
                           Atomics
*****************************************************************/

void StateFluent::calculateProbDomain(ActionState const& /*actions*/, set<double>& res) {
    res.insert(0.0);
    res.insert(1.0);
}

void ActionFluent::calculateProbDomain(ActionState const& actions, set<double>& res) {
    assert(index < actions.state.size());
    res.insert(actions[index]);
}

void NumericConstant::calculateProbDomain(ActionState const& /*actions*/, set<double>& res) {
    res.insert(value);
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Conjunction::calculateProbDomain(ActionState const& actions, set<double>& res) {
    assert(res.empty());
    set<double> tmp1;
    tmp1.insert(1.0);
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        res.clear();
        set<double> tmp;
        exprs[i]->calculateProbDomain(actions, tmp);
        for(set<double>::iterator it = tmp1.begin(); it != tmp1.end(); ++it) {
            for(set<double>::iterator it2 = tmp.begin(); it2 != tmp.end(); ++it2) {
                res.insert(*it * *it2);
            }
        }
        tmp1.clear();
        tmp1 = res;
    }
}

void Disjunction::calculateProbDomain(ActionState const& actions, set<double>& res) {
    assert(res.empty());
    set<double> tmp1;
    tmp1.insert(1.0);
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        res.clear();
        set<double> tmp;
        exprs[i]->calculateProbDomain(actions, tmp);
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

void EqualsExpression::calculateProbDomain(ActionState const& actions, set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());
    set<double> childRes;
    exprs[0]->calculateProbDomain(actions, childRes);
    //for(set<double>::iterator it = childRes.begin(); it != childRes.end(); ++it) {
        //cout << *it << " ";
        //}
    //cout << endl;
    set<double> childRes2;
    exprs[0]->calculateProbDomain(actions, childRes2);
    //for(set<double>::iterator it = childRes2.begin(); it != childRes2.end(); ++it) {
    //    cout << *it << " ";
    //}
    //cout << endl;
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

void GreaterExpression::calculateProbDomain(ActionState const& /*actions*/, set<double>& /*res*/) {
    assert(false);
}

void LowerExpression::calculateProbDomain(ActionState const& /*actions*/, set<double>& /*res*/) {
    assert(false);
}

void GreaterEqualsExpression::calculateProbDomain(ActionState const& actions, set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());
    set<double> childRes;
    exprs[0]->calculateProbDomain(actions, childRes);
    //for(set<double>::iterator it = childRes.begin(); it != childRes.end(); ++it) {
        //cout << *it << " ";
        //}
    //cout << endl;
    set<double> childRes2;
    exprs[0]->calculateProbDomain(actions, childRes2);
    //for(set<double>::iterator it = childRes2.begin(); it != childRes2.end(); ++it) {
    //    cout << *it << " ";
    //}
    //cout << endl;


    if(MathUtils::doubleIsGreaterOrEqual(*childRes.rbegin(),*childRes2.begin())) {
        res.insert(1.0);
        //cout << "might be bigger or equal" << endl;
    } else {
        res.insert(0.0);
    }

    if(!MathUtils::doubleIsGreaterOrEqual(*childRes.begin(),*childRes2.rbegin())) {
        res.insert(0.0);
        //cout << "might be smaller" << endl;
    } else {
        res.insert(1.0);
    }
}

void LowerEqualsExpression::calculateProbDomain(ActionState const& actions, set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());
    set<double> childRes;
    exprs[0]->calculateProbDomain(actions, childRes);
    //for(set<double>::iterator it = childRes.begin(); it != childRes.end(); ++it) {
        //cout << *it << " ";
        //}
    //cout << endl;
    set<double> childRes2;
    exprs[0]->calculateProbDomain(actions, childRes2);
    //for(set<double>::iterator it = childRes2.begin(); it != childRes2.end(); ++it) {
    //    cout << *it << " ";
    //}
    //cout << endl;


    if(MathUtils::doubleIsSmallerOrEqual(*childRes.rbegin(),*childRes2.begin())) {
        res.insert(1.0);
        //cout << "might be bigger or equal" << endl;
    } else {
        res.insert(0.0);
    }

    if(!MathUtils::doubleIsSmallerOrEqual(*childRes.begin(),*childRes2.rbegin())) {
        res.insert(0.0);
        //cout << "might be smaller" << endl;
    } else {
        res.insert(1.0);
    } 
}

void Addition::calculateProbDomain(ActionState const& actions, set<double>& res) {
    assert(res.empty());
    set<double> childRes;
    exprs[0]->calculateProbDomain(actions, childRes);

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        res.clear();
        set<double> childRes2;
        exprs[i]->calculateProbDomain(actions, childRes2);
        for(set<double>::iterator it = childRes.begin(); it != childRes.end(); ++it) {
            for(set<double>::iterator it2 = childRes2.begin(); it2 != childRes2.end(); ++it2) {
                res.insert(*it + *it2);
            }
        }
        childRes.clear();
        childRes = res;
    }
}

void Subtraction::calculateProbDomain(ActionState const& actions, set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());
    set<double> childRes1;
    exprs[0]->calculateProbDomain(actions, childRes1);
    set<double> childRes2;
    exprs[1]->calculateProbDomain(actions, childRes2);
    for(set<double>::iterator it = childRes1.begin(); it != childRes1.end(); ++it) {
        for(set<double>::iterator it2 = childRes2.begin(); it2 != childRes2.end(); ++it2) {
            res.insert(*it - *it2);
        }
    }
}

void Multiplication::calculateProbDomain(ActionState const& actions, set<double>& res) {
    assert(res.empty());
    set<double> childRes;
    exprs[0]->calculateProbDomain(actions, childRes);

    for(unsigned int i = 1; i < exprs.size(); ++i) {
        res.clear();
        set<double> childRes2;
        exprs[i]->calculateProbDomain(actions, childRes2);
        for(set<double>::iterator it = childRes.begin(); it != childRes.end(); ++it) {
            for(set<double>::iterator it2 = childRes2.begin(); it2 != childRes2.end(); ++it2) {
                res.insert(*it * *it2);
            }
        }
        childRes.clear();
        childRes = res;
    }
}

void Division::calculateProbDomain(ActionState const& actions, set<double>& res) {
    assert(exprs.size() == 2);
    assert(res.empty());

    set<double> childRes1;
    exprs[0]->calculateProbDomain(actions, childRes1);
    set<double> childRes2;
    exprs[1]->calculateProbDomain(actions, childRes2);
    for(set<double>::iterator it = childRes1.begin(); it != childRes1.end(); ++it) {
        for(set<double>::iterator it2 = childRes2.begin(); it2 != childRes2.end(); ++it2) {
            res.insert(*it / *it2);
        }
    }
}

/*****************************************************************
                          Unaries
*****************************************************************/

void NegateExpression::calculateProbDomain(ActionState const& actions, set<double>& res) {
    set<double> tmp;
    expr->calculateProbDomain(actions, tmp);
    for(set<double>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
        assert(MathUtils::doubleIsSmallerOrEqual(*it,1.0));
        res.insert(1.0 - (*it));
    }
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

void BernoulliDistribution::calculateProbDomain(ActionState const& actions, set<double>& res) {
    expr->calculateProbDomain(actions,res);
}

void DiscreteDistribution::calculateProbDomain(ActionState const& /*actions*/, set<double>& /*res*/) {
    assert(false);
}

/*****************************************************************
                         Conditionals
*****************************************************************/

void IfThenElseExpression::calculateProbDomain(ActionState const& actions, set<double>& res) {
    assert(res.empty());
    condition->calculateProbDomain(actions, res);
    assert(!res.empty());
    if(res.size() == 1 && (MathUtils::doubleIsEqual(*res.begin(),0.0))) {
        res.clear();
        valueIfFalse->calculateProbDomain(actions, res);
    } else if(res.size() == 1 && (MathUtils::doubleIsEqual(*res.begin(),1.0))) {
        res.clear();
        valueIfTrue->calculateProbDomain(actions, res);
    } else {
        res.clear();
        valueIfTrue->calculateProbDomain(actions, res);
        set<double> tmp;
        valueIfFalse->calculateProbDomain(actions, tmp);
        res.insert(tmp.begin(),tmp.end());
    }
}

void MultiConditionChecker::calculateProbDomain(ActionState const& actions, set<double>& res) {
    for(unsigned int i = 0; i < conditions.size(); ++i) {
        set<double> tmp;
        conditions[i]->calculateProbDomain(actions, tmp);
        assert(!tmp.empty());
        if(tmp.size() > 1 || !MathUtils::doubleIsEqual(*tmp.begin(),0.0)) {
            tmp.clear();
            effects[i]->calculateProbDomain(actions, tmp);
            res.insert(tmp.begin(),tmp.end());
        }
    }
}

