void LogicalExpression::collectInitialInfo(
    bool& /*isProbabilistic*/, bool& /*containsArithmeticFunction*/,
    StateFluentSet& /*dependentStateFluents*/,
    ActionFluentSet& /*dependentActionFluents*/) {
    print(cout);
    assert(false);
}

/*****************************************************************
                           Atomics
*****************************************************************/

void StateFluent::collectInitialInfo(
    bool& /*isProbabilistic*/, bool& /*containsArithmeticFunction*/,
    StateFluentSet& dependentStateFluents,
    ActionFluentSet& /*dependentActionFluents*/) {
    dependentStateFluents.insert(this);
}

void ActionFluent::collectInitialInfo(bool& /*isProbabilistic*/,
                                      bool& /*containsArithmeticFunction*/,
                                      StateFluentSet& /*dependentStateFluents*/,
                                      ActionFluentSet& dependentActionFluents) {
    dependentActionFluents.insert(this);
}

void NumericConstant::collectInitialInfo(
    bool& /*isProbabilistic*/, bool& /*containsArithmeticFunction*/,
    StateFluentSet& /*dependentStateFluents*/,
    ActionFluentSet& /*dependentActionFluents*/) {}

/*****************************************************************
                           Connectives
*****************************************************************/

void Conjunction::collectInitialInfo(bool& isProbabilistic,
                                     bool& containsArithmeticFunction,
                                     StateFluentSet& dependentStateFluents,
                                     ActionFluentSet& dependentActionFluents) {
    containsArithmeticFunction = false;
    bool containsArithmetic = false;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, containsArithmetic,
                                     dependentStateFluents,
                                     dependentActionFluents);
        containsArithmeticFunction =
            containsArithmeticFunction || containsArithmetic;
    }
}

void Disjunction::collectInitialInfo(bool& isProbabilistic,
                                     bool& containsArithmeticFunction,
                                     StateFluentSet& dependentStateFluents,
                                     ActionFluentSet& dependentActionFluents) {
    containsArithmeticFunction = false;
    bool containsArithmetic = false;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, containsArithmetic,
                                     dependentStateFluents,
                                     dependentActionFluents);
        containsArithmeticFunction =
            containsArithmeticFunction || containsArithmetic;
    }
}

void EqualsExpression::collectInitialInfo(
    bool& isProbabilistic, bool& containsArithmeticFunction,
    StateFluentSet& dependentStateFluents,
    ActionFluentSet& dependentActionFluents) {
    containsArithmeticFunction = false;
    bool containsArithmetic = false;
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(isProbabilistic, containsArithmetic,
                                     dependentStateFluents,
                                     dependentActionFluents);
        containsArithmeticFunction =
            containsArithmeticFunction || containsArithmetic;
    }
}

void GreaterExpression::collectInitialInfo(
    bool& isProbabilistic, bool& containsArithmeticFunction,
    StateFluentSet& dependentStateFluents,
    ActionFluentSet& dependentActionFluents) {
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(
            isProbabilistic, containsArithmeticFunction, dependentStateFluents,
            dependentActionFluents);
    }
    containsArithmeticFunction = true;
}

void LowerExpression::collectInitialInfo(
    bool& isProbabilistic, bool& containsArithmeticFunction,
    StateFluentSet& dependentStateFluents,
    ActionFluentSet& dependentActionFluents) {
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(
            isProbabilistic, containsArithmeticFunction, dependentStateFluents,
            dependentActionFluents);
    }
    containsArithmeticFunction = true;
}

void GreaterEqualsExpression::collectInitialInfo(
    bool& isProbabilistic, bool& containsArithmeticFunction,
    StateFluentSet& dependentStateFluents,
    ActionFluentSet& dependentActionFluents) {
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(
            isProbabilistic, containsArithmeticFunction, dependentStateFluents,
            dependentActionFluents);
    }

    containsArithmeticFunction = true;
}

void LowerEqualsExpression::collectInitialInfo(
    bool& isProbabilistic, bool& containsArithmeticFunction,
    StateFluentSet& dependentStateFluents,
    ActionFluentSet& dependentActionFluents) {
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(
            isProbabilistic, containsArithmeticFunction, dependentStateFluents,
            dependentActionFluents);
    }

    containsArithmeticFunction = true;
}

void Addition::collectInitialInfo(bool& isProbabilistic,
                                  bool& containsArithmeticFunction,
                                  StateFluentSet& dependentStateFluents,
                                  ActionFluentSet& dependentActionFluents) {
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(
            isProbabilistic, containsArithmeticFunction, dependentStateFluents,
            dependentActionFluents);
    }

    containsArithmeticFunction = true;
}

void Subtraction::collectInitialInfo(bool& isProbabilistic,
                                     bool& containsArithmeticFunction,
                                     StateFluentSet& dependentStateFluents,
                                     ActionFluentSet& dependentActionFluents) {
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(
            isProbabilistic, containsArithmeticFunction, dependentStateFluents,
            dependentActionFluents);
    }

    containsArithmeticFunction = true;
}

void Multiplication::collectInitialInfo(
    bool& isProbabilistic, bool& containsArithmeticFunction,
    StateFluentSet& dependentStateFluents,
    ActionFluentSet& dependentActionFluents) {
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(
            isProbabilistic, containsArithmeticFunction, dependentStateFluents,
            dependentActionFluents);
    }

    containsArithmeticFunction = true;
}

void Division::collectInitialInfo(bool& isProbabilistic,
                                  bool& containsArithmeticFunction,
                                  StateFluentSet& dependentStateFluents,
                                  ActionFluentSet& dependentActionFluents) {
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->collectInitialInfo(
            isProbabilistic, containsArithmeticFunction, dependentStateFluents,
            dependentActionFluents);
    }

    containsArithmeticFunction = true;
}

/*****************************************************************
                          Unaries
*****************************************************************/

void Negation::collectInitialInfo(bool& isProbabilistic,
                                  bool& containsArithmeticFunction,
                                  StateFluentSet& dependentStateFluents,
                                  ActionFluentSet& dependentActionFluents) {
    expr->collectInitialInfo(isProbabilistic, containsArithmeticFunction,
                             dependentStateFluents, dependentActionFluents);
}

void ExponentialFunction::collectInitialInfo(
    bool& isProbabilistic, bool& containsArithmeticFunction,
    StateFluentSet& dependentStateFluents,
    ActionFluentSet& dependentActionFluents) {
    expr->collectInitialInfo(isProbabilistic, containsArithmeticFunction,
                             dependentStateFluents, dependentActionFluents);

    containsArithmeticFunction = true;
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

void BernoulliDistribution::collectInitialInfo(
    bool& isProbabilistic, bool& containsArithmeticFunction,
    StateFluentSet& dependentStateFluents,
    ActionFluentSet& dependentActionFluents) {
    expr->collectInitialInfo(isProbabilistic, containsArithmeticFunction,
                             dependentStateFluents, dependentActionFluents);
    isProbabilistic = true;
}

void DiscreteDistribution::collectInitialInfo(
    bool& isProbabilistic, bool& containsArithmeticFunction,
    StateFluentSet& dependentStateFluents,
    ActionFluentSet& dependentActionFluents) {
    containsArithmeticFunction = false;
    for (unsigned int i = 0; i < probabilities.size(); ++i) {
        bool containsArithmetic = false;

        values[i]->collectInitialInfo(isProbabilistic, containsArithmetic,
                                      dependentStateFluents,
                                      dependentActionFluents);
        assert(!containsArithmetic);

        probabilities[i]->collectInitialInfo(
            isProbabilistic, containsArithmetic, dependentStateFluents,
            dependentActionFluents);
        containsArithmeticFunction =
            containsArithmeticFunction || containsArithmetic;
    }
    isProbabilistic = true;
}

/*****************************************************************
                         Conditionals
*****************************************************************/

void IfThenElseExpression::collectInitialInfo(
    bool& isProbabilistic, bool& containsArithmeticFunction,
    StateFluentSet& dependentStateFluents,
    ActionFluentSet& dependentActionFluents) {
    containsArithmeticFunction = false;
    bool containsArithmetic = false;

    condition->collectInitialInfo(isProbabilistic, containsArithmetic,
                                  dependentStateFluents,
                                  dependentActionFluents);
    containsArithmeticFunction =
        containsArithmeticFunction || containsArithmetic;

    valueIfTrue->collectInitialInfo(isProbabilistic, containsArithmetic,
                                    dependentStateFluents,
                                    dependentActionFluents);
    containsArithmeticFunction =
        containsArithmeticFunction || containsArithmetic;

    valueIfFalse->collectInitialInfo(isProbabilistic, containsArithmetic,
                                     dependentStateFluents,
                                     dependentActionFluents);
    containsArithmeticFunction =
        containsArithmeticFunction || containsArithmetic;
}

void MultiConditionChecker::collectInitialInfo(
    bool& isProbabilistic, bool& containsArithmeticFunction,
    StateFluentSet& dependentStateFluents,
    ActionFluentSet& dependentActionFluents) {
    containsArithmeticFunction = false;
    bool containsArithmetic = false;

    for (unsigned int i = 0; i < conditions.size(); ++i) {
        conditions[i]->collectInitialInfo(isProbabilistic, containsArithmetic,
                                          dependentStateFluents,
                                          dependentActionFluents);
        containsArithmeticFunction =
            containsArithmeticFunction || containsArithmetic;

        effects[i]->collectInitialInfo(isProbabilistic, containsArithmetic,
                                       dependentStateFluents,
                                       dependentActionFluents);
        containsArithmeticFunction =
            containsArithmeticFunction || containsArithmetic;
    }
}
