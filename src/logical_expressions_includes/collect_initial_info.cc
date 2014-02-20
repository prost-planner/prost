/*****************************************************************
                           Atomics
*****************************************************************/

void StateFluent::collectInitialInfo(bool& /*isProbabilistic*/,
                                     bool& /*containsArithmeticFunction*/,
                                     set<StateFluent*>& dependentStateFluents,
                                     set<ActionFluent*>& /*positiveDependentActionFluents*/, 
                                     set<ActionFluent*>& /*negativeDependentActionFluents*/) {
    dependentStateFluents.insert(this);
}

void ActionFluent::collectInitialInfo(bool& /*isProbabilistic*/,
                                      bool& /*containsArithmeticFunction*/,
                                      set<StateFluent*>& /*dependentStateFluents*/,
                                      set<ActionFluent*>& positiveDependentActionFluents, 
                                      set<ActionFluent*>& /*negativeDependentActionFluents*/) {
    positiveDependentActionFluents.insert(this);
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Conjunction::collectInitialInfo(bool& isProbabilistic,
                                     bool& containsArithmeticFunction, 
                                     set<StateFluent*>& dependentStateFluents,
                                     set<ActionFluent*>& positiveDependentActionFluents, 
                                     set<ActionFluent*>& negativeDependentActionFluents) {

    containsArithmeticFunction = false;
    bool containsArithmetic = false;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, containsArithmetic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
        containsArithmeticFunction = containsArithmeticFunction || containsArithmetic;
    }
}

void Disjunction::collectInitialInfo(bool& isProbabilistic,
                                     bool& containsArithmeticFunction,
                                     set<StateFluent*>& dependentStateFluents,
                                     set<ActionFluent*>& positiveDependentActionFluents, 
                                     set<ActionFluent*>& negativeDependentActionFluents) {
    containsArithmeticFunction = false;
    bool containsArithmetic = false;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, containsArithmetic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
        containsArithmeticFunction = containsArithmeticFunction || containsArithmetic;
    }
}

void EqualsExpression::collectInitialInfo(bool& isProbabilistic,
                                          bool& containsArithmeticFunction,
                                          set<StateFluent*>& dependentStateFluents,
                                          set<ActionFluent*>& positiveDependentActionFluents, 
                                          set<ActionFluent*>& negativeDependentActionFluents) {
    containsArithmeticFunction = false;
    bool containsArithmetic = false;
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, containsArithmetic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
        containsArithmeticFunction = containsArithmeticFunction || containsArithmetic;
    }

    //All actions depend positively and negatively
    positiveDependentActionFluents.insert(negativeDependentActionFluents.begin(),negativeDependentActionFluents.end());
    negativeDependentActionFluents.clear();
    negativeDependentActionFluents = positiveDependentActionFluents;
}

void GreaterExpression::collectInitialInfo(bool& isProbabilistic,
                                           bool& containsArithmeticFunction,
                                           set<StateFluent*>& dependentStateFluents,
                                           set<ActionFluent*>& positiveDependentActionFluents, 
                                           set<ActionFluent*>& negativeDependentActionFluents) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, containsArithmeticFunction, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    }

    //All actions depend positively and negatively
    positiveDependentActionFluents.insert(negativeDependentActionFluents.begin(),negativeDependentActionFluents.end());
    negativeDependentActionFluents.clear();
    negativeDependentActionFluents = positiveDependentActionFluents;
    containsArithmeticFunction = true;
}

void LowerExpression::collectInitialInfo(bool& isProbabilistic,
                                         bool& containsArithmeticFunction,
                                         set<StateFluent*>& dependentStateFluents,
                                         set<ActionFluent*>& positiveDependentActionFluents, 
                                         set<ActionFluent*>& negativeDependentActionFluents) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, containsArithmeticFunction, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    }

    //All actions depend positively and negatively
    positiveDependentActionFluents.insert(negativeDependentActionFluents.begin(),negativeDependentActionFluents.end());
    negativeDependentActionFluents.clear();
    negativeDependentActionFluents = positiveDependentActionFluents;
    containsArithmeticFunction = true;
}

void GreaterEqualsExpression::collectInitialInfo(bool& isProbabilistic,
                                                 bool& containsArithmeticFunction,
                                                 set<StateFluent*>& dependentStateFluents,
                                                 set<ActionFluent*>& positiveDependentActionFluents, 
                                                 set<ActionFluent*>& negativeDependentActionFluents) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, containsArithmeticFunction, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    }

    //All actions depend positively and negatively
    positiveDependentActionFluents.insert(negativeDependentActionFluents.begin(),negativeDependentActionFluents.end());
    negativeDependentActionFluents.clear();
    negativeDependentActionFluents = positiveDependentActionFluents;
    containsArithmeticFunction = true;
}

void LowerEqualsExpression::collectInitialInfo(bool& isProbabilistic,
                                               bool& containsArithmeticFunction,
                                               set<StateFluent*>& dependentStateFluents,
                                               set<ActionFluent*>& positiveDependentActionFluents, 
                                               set<ActionFluent*>& negativeDependentActionFluents) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, containsArithmeticFunction, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    }

    //All actions depend positively and negatively
    positiveDependentActionFluents.insert(negativeDependentActionFluents.begin(),negativeDependentActionFluents.end());
    negativeDependentActionFluents.clear();
    negativeDependentActionFluents = positiveDependentActionFluents;
    containsArithmeticFunction = true;
}

void Addition::collectInitialInfo(bool& isProbabilistic,
                                  bool& containsArithmeticFunction,
                                  set<StateFluent*>& dependentStateFluents,
                                  set<ActionFluent*>& positiveDependentActionFluents, 
                                  set<ActionFluent*>& negativeDependentActionFluents) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, containsArithmeticFunction, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    }
    containsArithmeticFunction = true;
}

void Subtraction::collectInitialInfo(bool& isProbabilistic,
                                     bool& containsArithmeticFunction,
                                     set<StateFluent*>& dependentStateFluents,
                                     set<ActionFluent*>& positiveDependentActionFluents, 
                                     set<ActionFluent*>& negativeDependentActionFluents) {
    assert(exprs.size() == 2);
    exprs[0]->collectInitialInfo(isProbabilistic, containsArithmeticFunction, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    exprs[1]->collectInitialInfo(isProbabilistic, containsArithmeticFunction, dependentStateFluents, negativeDependentActionFluents, positiveDependentActionFluents);
    containsArithmeticFunction = true;
}

void Multiplication::collectInitialInfo(bool& isProbabilistic,
                                        bool& containsArithmeticFunction,
                                        set<StateFluent*>& dependentStateFluents,
                                        set<ActionFluent*>& positiveDependentActionFluents, 
                                        set<ActionFluent*>& negativeDependentActionFluents) {
    assert(exprs.size() == 2);

    set<ActionFluent*> tmpPos0;
    set<ActionFluent*> tmpNeg0;    
    exprs[0]->collectInitialInfo(isProbabilistic, containsArithmeticFunction, dependentStateFluents, tmpPos0, tmpNeg0);

    set<ActionFluent*> tmpPos1;
    set<ActionFluent*> tmpNeg1;    
    exprs[1]->collectInitialInfo(isProbabilistic, containsArithmeticFunction, dependentStateFluents, tmpPos1, tmpNeg1);

    NumericConstant* tmp0 = dynamic_cast<NumericConstant*>(exprs[0]);
    NumericConstant* tmp1 = dynamic_cast<NumericConstant*>(exprs[1]);

    if((tmp0 && tmp1) || (!tmp0 && !tmp1) || (tmp0 && MathUtils::doubleIsGreaterOrEqual(tmp0->value,0.0)) || (tmp1 && MathUtils::doubleIsGreaterOrEqual(tmp1->value,0.0))) {
        positiveDependentActionFluents.insert(tmpPos0.begin(),tmpPos0.end());
        positiveDependentActionFluents.insert(tmpPos1.begin(),tmpPos1.end());

        negativeDependentActionFluents.insert(tmpNeg0.begin(),tmpNeg0.end());
        negativeDependentActionFluents.insert(tmpNeg1.begin(),tmpNeg1.end());
    } else if(tmp0) {
        assert(MathUtils::doubleIsSmaller(tmp0->value, 0.0));
        assert(tmpPos0.empty() && tmpNeg0.empty());
        positiveDependentActionFluents.insert(tmpNeg1.begin(),tmpNeg1.end());
        negativeDependentActionFluents.insert(tmpPos1.begin(),tmpPos1.end());
    } else {
        assert(tmp1 && MathUtils::doubleIsSmaller(tmp1->value, 0.0));
        assert(tmpPos1.empty() && tmpNeg1.empty());
        positiveDependentActionFluents.insert(tmpNeg0.begin(),tmpNeg0.end());
        negativeDependentActionFluents.insert(tmpPos0.begin(),tmpPos0.end());
    }
    containsArithmeticFunction = true;
}

void Division::collectInitialInfo(bool& isProbabilistic,
                                  bool& containsArithmeticFunction,
                                  set<StateFluent*>& dependentStateFluents,
                                  set<ActionFluent*>& positiveDependentActionFluents, 
                                  set<ActionFluent*>& negativeDependentActionFluents) {
    assert(exprs.size() == 2);

    set<ActionFluent*> tmpPos0;
    set<ActionFluent*> tmpNeg0;    
    exprs[0]->collectInitialInfo(isProbabilistic, containsArithmeticFunction, dependentStateFluents, tmpPos0, tmpNeg0);

    set<ActionFluent*> tmpPos1;
    set<ActionFluent*> tmpNeg1;    
    exprs[1]->collectInitialInfo(isProbabilistic, containsArithmeticFunction, dependentStateFluents, tmpPos1, tmpNeg1);

    NumericConstant* tmp0 = dynamic_cast<NumericConstant*>(exprs[0]);
    NumericConstant* tmp1 = dynamic_cast<NumericConstant*>(exprs[1]);

    if((tmp0 && tmp1) || (!tmp0 && !tmp1) || (tmp0 && MathUtils::doubleIsGreaterOrEqual(tmp0->value,0.0)) || (tmp1 && MathUtils::doubleIsGreaterOrEqual(tmp1->value,0.0))) {
        positiveDependentActionFluents.insert(tmpPos0.begin(),tmpPos0.end());
        positiveDependentActionFluents.insert(tmpPos1.begin(),tmpPos1.end());

        negativeDependentActionFluents.insert(tmpNeg0.begin(),tmpNeg0.end());
        negativeDependentActionFluents.insert(tmpNeg1.begin(),tmpNeg1.end());
    } else if(tmp0) {
        assert(MathUtils::doubleIsSmaller(tmp0->value, 0.0));
        assert(tmpPos0.empty() && tmpNeg0.empty());
        positiveDependentActionFluents.insert(tmpNeg1.begin(),tmpNeg1.end());
        negativeDependentActionFluents.insert(tmpPos1.begin(),tmpPos1.end());
    } else {
        assert(tmp1 && MathUtils::doubleIsSmaller(tmp1->value, 0.0));
        assert(tmpPos1.empty() && tmpNeg1.empty());
        positiveDependentActionFluents.insert(tmpNeg0.begin(),tmpNeg0.end());
        negativeDependentActionFluents.insert(tmpPos0.begin(),tmpPos0.end());
    }
    containsArithmeticFunction = true;
}

/*****************************************************************
                          Unaries
*****************************************************************/

void Negation::collectInitialInfo(bool& isProbabilistic,
                                  bool& containsArithmeticFunction,
                                  set<StateFluent*>& dependentStateFluents,
                                  set<ActionFluent*>& positiveDependentActionFluents,
                                  set<ActionFluent*>& negativeDependentActionFluents) {
    expr->collectInitialInfo(isProbabilistic, containsArithmeticFunction, dependentStateFluents, negativeDependentActionFluents, positiveDependentActionFluents);
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

void BernoulliDistribution::collectInitialInfo(bool& isProbabilistic,
                                               bool& containsArithmeticFunction,
                                               set<StateFluent*>& dependentStateFluents,
                                               set<ActionFluent*>& positiveDependentActionFluents,
                                               set<ActionFluent*>& negativeDependentActionFluents) {
    expr->collectInitialInfo(isProbabilistic, containsArithmeticFunction, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    isProbabilistic = true;
}

void DiscreteDistribution::collectInitialInfo(bool& isProbabilistic,
                                              bool& containsArithmeticFunction,
                                              set<StateFluent*>& dependentStateFluents,
                                              set<ActionFluent*>& positiveDependentActionFluents,
                                              set<ActionFluent*>& negativeDependentActionFluents) {
    containsArithmeticFunction = false;
    bool containsArithmetic = false;
    for(unsigned int i = 0; i < probabilities.size(); ++i) {
        containsArithmetic = false;
        values[i]->collectInitialInfo(isProbabilistic, containsArithmetic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
        assert(!containsArithmetic);
        probabilities[i]->collectInitialInfo(isProbabilistic, containsArithmetic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
        containsArithmeticFunction = containsArithmeticFunction || containsArithmetic;
    }
    isProbabilistic = true;
}

/*****************************************************************
                         Conditionals
*****************************************************************/

void IfThenElseExpression::collectInitialInfo(bool& isProbabilistic,
                                              bool& containsArithmeticFunction,
                                              set<StateFluent*>& dependentStateFluents,
                                              set<ActionFluent*>& positiveDependentActionFluents,
                                              set<ActionFluent*>& negativeDependentActionFluents) {
    containsArithmeticFunction = false;
    bool containsArithmetic = false;
    condition->collectInitialInfo(isProbabilistic, containsArithmetic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    containsArithmeticFunction = containsArithmeticFunction || containsArithmetic;
    valueIfTrue->collectInitialInfo(isProbabilistic, containsArithmetic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    containsArithmeticFunction = containsArithmeticFunction || containsArithmetic;
    valueIfFalse->collectInitialInfo(isProbabilistic, containsArithmetic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
    containsArithmeticFunction = containsArithmeticFunction || containsArithmetic;
}

void MultiConditionChecker::collectInitialInfo(bool& isProbabilistic,
                                               bool& containsArithmeticFunction,
                                               set<StateFluent*>& dependentStateFluents,
                                               set<ActionFluent*>& positiveDependentActionFluents,
                                               set<ActionFluent*>& negativeDependentActionFluents) {
    containsArithmeticFunction = false;
    bool containsArithmetic = false;
    for(unsigned int i = 0; i < conditions.size(); ++i) {
        conditions[i]->collectInitialInfo(isProbabilistic, containsArithmetic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
        containsArithmeticFunction = containsArithmeticFunction || containsArithmetic;
        effects[i]->collectInitialInfo(isProbabilistic, containsArithmetic, dependentStateFluents, positiveDependentActionFluents, negativeDependentActionFluents);
        containsArithmeticFunction = containsArithmeticFunction || containsArithmetic;
    }
}



