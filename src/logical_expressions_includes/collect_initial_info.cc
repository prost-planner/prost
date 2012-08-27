//bool collectInitialInfo

void LogicalExpression::collectInitialInfo(bool& /*isProbabilistic*/, vector<StateFluent*>& /*dependentStateFluents*/, 
                                           vector<ActionFluent*>& /*positiveDependentActionFluents*/, 
                                           vector<ActionFluent*>& /*negativeDependentActionFluents*/) {
}

void StateFluent::collectInitialInfo(bool& /*isProbabilistic*/, vector<StateFluent*>& dependentStateFluents,
                                     vector<ActionFluent*>& /*positiveDependentActionFluents*/, 
                                     vector<ActionFluent*>& /*negativeDependentActionFluents*/) {
    for(unsigned int i = 0; i < dependentStateFluents.size(); ++i) {
        if(dependentStateFluents[i] == this) {
            return;
        }
    }
    dependentStateFluents.push_back(this);
}

void ActionFluent::collectInitialInfo(bool& /*isProbabilistic*/, vector<StateFluent*>& /*dependentStateFluents*/,
                                      vector<ActionFluent*>& positiveDependentActionFluents, 
                                      vector<ActionFluent*>& /*negativeDependentActionFluents*/) {
    positiveDependentActionFluents.push_back(this);
    /*
    for(unsigned int i = 0; i < dependentActionFluents.size(); ++i) {
        if(dependentActionFluents[i] == this) {
            return;
        }
    }
    dependentActionFluents.push_back(this);
    */
}

void Conjunction::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                     vector<ActionFluent*>& positiveDependentActionFluents, 
                                     vector<ActionFluent*>& negativeDependentActionFluents) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    }
}

void Disjunction::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                     vector<ActionFluent*>& positiveDependentActionFluents, 
                                     vector<ActionFluent*>& negativeDependentActionFluents) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    }
}

void EqualsExpression::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                     vector<ActionFluent*>& positiveDependentActionFluents, 
                                     vector<ActionFluent*>& negativeDependentActionFluents) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    }

    //All actions depend positively and negatively
    positiveDependentActionFluents.insert(positiveDependentActionFluents.end(),negativeDependentActionFluents.begin(),negativeDependentActionFluents.end());
    negativeDependentActionFluents.clear();
    negativeDependentActionFluents = positiveDependentActionFluents;
}

void GreaterExpression::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                     vector<ActionFluent*>& positiveDependentActionFluents, 
                                     vector<ActionFluent*>& negativeDependentActionFluents) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    }

    //All actions depend positively and negatively
    positiveDependentActionFluents.insert(positiveDependentActionFluents.end(),negativeDependentActionFluents.begin(),negativeDependentActionFluents.end());
    negativeDependentActionFluents.clear();
    negativeDependentActionFluents = positiveDependentActionFluents;
}

void LowerExpression::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                     vector<ActionFluent*>& positiveDependentActionFluents, 
                                     vector<ActionFluent*>& negativeDependentActionFluents) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    }

    //All actions depend positively and negatively
    positiveDependentActionFluents.insert(positiveDependentActionFluents.end(),negativeDependentActionFluents.begin(),negativeDependentActionFluents.end());
    negativeDependentActionFluents.clear();
    negativeDependentActionFluents = positiveDependentActionFluents;
}

void GreaterEqualsExpression::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                     vector<ActionFluent*>& positiveDependentActionFluents, 
                                     vector<ActionFluent*>& negativeDependentActionFluents) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    }

    //All actions depend positively and negatively
    positiveDependentActionFluents.insert(positiveDependentActionFluents.end(),negativeDependentActionFluents.begin(),negativeDependentActionFluents.end());
    negativeDependentActionFluents.clear();
    negativeDependentActionFluents = positiveDependentActionFluents;
}

void LowerEqualsExpression::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                     vector<ActionFluent*>& positiveDependentActionFluents, 
                                     vector<ActionFluent*>& negativeDependentActionFluents) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    }

    //All actions depend positively and negatively
    positiveDependentActionFluents.insert(positiveDependentActionFluents.end(),negativeDependentActionFluents.begin(),negativeDependentActionFluents.end());
    negativeDependentActionFluents.clear();
    negativeDependentActionFluents = positiveDependentActionFluents;
}

void Addition::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                     vector<ActionFluent*>& positiveDependentActionFluents, 
                                     vector<ActionFluent*>& negativeDependentActionFluents) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    }
}

void Subtraction::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                     vector<ActionFluent*>& positiveDependentActionFluents, 
                                     vector<ActionFluent*>& negativeDependentActionFluents) {
    assert(exprs.size() == 2);
    exprs[0]->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    exprs[1]->collectInitialInfo(isProbabilistic, dependentStateFluents, negativeDependentActionFluents, positiveDependentActionFluents);
}

void Multiplication::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                     vector<ActionFluent*>& positiveDependentActionFluents, 
                                     vector<ActionFluent*>& negativeDependentActionFluents) {
    assert(exprs.size() == 2);

    vector<ActionFluent*> tmpPos0;
    vector<ActionFluent*> tmpNeg0;    
    exprs[0]->collectInitialInfo(isProbabilistic, dependentStateFluents, tmpPos0, tmpNeg0);

    vector<ActionFluent*> tmpPos1;
    vector<ActionFluent*> tmpNeg1;    
    exprs[1]->collectInitialInfo(isProbabilistic, dependentStateFluents, tmpPos1, tmpNeg1);

    NumericConstant* tmp0 = dynamic_cast<NumericConstant*>(exprs[0]);
    NumericConstant* tmp1 = dynamic_cast<NumericConstant*>(exprs[1]);

    if((tmp0 && tmp1) || (!tmp0 && !tmp1) || (tmp0 && MathUtils::doubleIsGreaterOrEqual(tmp0->value,0.0)) || (tmp1 && MathUtils::doubleIsGreaterOrEqual(tmp1->value,0.0))) {
        positiveDependentActionFluents.insert(positiveDependentActionFluents.end(),tmpPos0.begin(),tmpPos0.end());
        positiveDependentActionFluents.insert(positiveDependentActionFluents.end(),tmpPos1.begin(),tmpPos1.end());

        negativeDependentActionFluents.insert(negativeDependentActionFluents.end(),tmpNeg0.begin(),tmpNeg0.end());
        negativeDependentActionFluents.insert(negativeDependentActionFluents.end(),tmpNeg1.begin(),tmpNeg1.end());
    } else if(tmp0) {
        assert(MathUtils::doubleIsSmaller(tmp0->value, 0.0));
        assert(tmpPos0.empty() && tmpNeg0.empty());
        positiveDependentActionFluents.insert(positiveDependentActionFluents.end(),tmpNeg1.begin(),tmpNeg1.end());
        negativeDependentActionFluents.insert(negativeDependentActionFluents.end(),tmpPos1.begin(),tmpPos1.end());
    } else {
        assert(tmp1 && MathUtils::doubleIsSmaller(tmp1->value, 0.0));
        assert(tmpPos1.empty() && tmpNeg1.empty());
        positiveDependentActionFluents.insert(positiveDependentActionFluents.end(),tmpNeg0.begin(),tmpNeg0.end());
        negativeDependentActionFluents.insert(negativeDependentActionFluents.end(),tmpPos0.begin(),tmpPos0.end());
    }
}

void Division::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                     vector<ActionFluent*>& positiveDependentActionFluents, 
                                     vector<ActionFluent*>& negativeDependentActionFluents) {
    assert(exprs.size() == 2);

    vector<ActionFluent*> tmpPos0;
    vector<ActionFluent*> tmpNeg0;    
    exprs[0]->collectInitialInfo(isProbabilistic, dependentStateFluents, tmpPos0, tmpNeg0);

    vector<ActionFluent*> tmpPos1;
    vector<ActionFluent*> tmpNeg1;    
    exprs[1]->collectInitialInfo(isProbabilistic, dependentStateFluents, tmpPos1, tmpNeg1);

    NumericConstant* tmp0 = dynamic_cast<NumericConstant*>(exprs[0]);
    NumericConstant* tmp1 = dynamic_cast<NumericConstant*>(exprs[1]);

    if((tmp0 && tmp1) || (!tmp0 && !tmp1) || (tmp0 && MathUtils::doubleIsGreaterOrEqual(tmp0->value,0.0)) || (tmp1 && MathUtils::doubleIsGreaterOrEqual(tmp1->value,0.0))) {
        positiveDependentActionFluents.insert(positiveDependentActionFluents.end(),tmpPos0.begin(),tmpPos0.end());
        positiveDependentActionFluents.insert(positiveDependentActionFluents.end(),tmpPos1.begin(),tmpPos1.end());

        negativeDependentActionFluents.insert(negativeDependentActionFluents.end(),tmpNeg0.begin(),tmpNeg0.end());
        negativeDependentActionFluents.insert(negativeDependentActionFluents.end(),tmpNeg1.begin(),tmpNeg1.end());
    } else if(tmp0) {
        assert(MathUtils::doubleIsSmaller(tmp0->value, 0.0));
        assert(tmpPos0.empty() && tmpNeg0.empty());
        positiveDependentActionFluents.insert(positiveDependentActionFluents.end(),tmpNeg1.begin(),tmpNeg1.end());
        negativeDependentActionFluents.insert(negativeDependentActionFluents.end(),tmpPos1.begin(),tmpPos1.end());
    } else {
        assert(tmp1 && MathUtils::doubleIsSmaller(tmp1->value, 0.0));
        assert(tmpPos1.empty() && tmpNeg1.empty());
        positiveDependentActionFluents.insert(positiveDependentActionFluents.end(),tmpNeg0.begin(),tmpNeg0.end());
        negativeDependentActionFluents.insert(negativeDependentActionFluents.end(),tmpPos0.begin(),tmpPos0.end());
    }
}

void BernoulliDistribution::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                               vector<ActionFluent*>& positiveDependentActionFluents,
                                               vector<ActionFluent*>& negativeDependentActionFluents) {
    expr->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    isProbabilistic = true;
}

void IfThenElseExpression::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                              vector<ActionFluent*>& positiveDependentActionFluents,
                                              vector<ActionFluent*>& negativeDependentActionFluents) {
    condition->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    valueIfTrue->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    valueIfFalse->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
}

void MultiConditionChecker::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                               vector<ActionFluent*>& positiveDependentActionFluents,
                                               vector<ActionFluent*>& negativeDependentActionFluents) {
    for(unsigned int i = 0; i < conditions.size(); ++i) {
        conditions[i]->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
        effects[i]->collectInitialInfo(isProbabilistic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    }
}

void NegateExpression::collectInitialInfo(bool& isProbabilistic, vector<StateFluent*>& dependentStateFluents,
                                          vector<ActionFluent*>& positiveDependentActionFluents,
                                          vector<ActionFluent*>& negativeDependentActionFluents) {
    expr->collectInitialInfo(isProbabilistic, dependentStateFluents, negativeDependentActionFluents, positiveDependentActionFluents);
}

