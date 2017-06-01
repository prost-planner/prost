void LogicalExpression::classifyActionFluents(ActionFluentSet& /*positive*/,
                                              ActionFluentSet& /*negative*/) {
    print(cout);
    assert(false);
    SystemUtils::abort(
        "Error: classify action fluents called on logical expression that "
        "doesn't support it.");
}

/*****************************************************************
                           Atomics
*****************************************************************/

void StateFluent::classifyActionFluents(ActionFluentSet& /*positive*/,
                                        ActionFluentSet& /*negative*/) {}

void ActionFluent::classifyActionFluents(ActionFluentSet& positive,
                                         ActionFluentSet& /*negative*/) {
    positive.insert(this);
}

void NumericConstant::classifyActionFluents(ActionFluentSet& /*positive*/,
                                            ActionFluentSet& /*negative*/) {}

/*****************************************************************
                           Connectives
*****************************************************************/

void Connective::classifyActionFluents(ActionFluentSet& positive,
                                       ActionFluentSet& negative) {
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->classifyActionFluents(positive, negative);
    }
}

void EqualsExpression::classifyActionFluents(ActionFluentSet& positive,
                                             ActionFluentSet& negative) {
    ActionFluentSet fluents;
    Connective::classifyActionFluents(fluents, fluents);

    // All actions depend positively and negatively
    positive.insert(fluents.begin(), fluents.end());
    negative.insert(fluents.begin(), fluents.end());
}

void GreaterExpression::classifyActionFluents(ActionFluentSet& positive,
                                              ActionFluentSet& negative) {
    ActionFluentSet fluents;
    Connective::classifyActionFluents(fluents, fluents);

    // All actions depend positively and negatively
    positive.insert(fluents.begin(), fluents.end());
    negative.insert(fluents.begin(), fluents.end());
}

void LowerExpression::classifyActionFluents(ActionFluentSet& positive,
                                            ActionFluentSet& negative) {
    ActionFluentSet fluents;
    Connective::classifyActionFluents(fluents, fluents);

    // All actions depend positively and negatively
    positive.insert(fluents.begin(), fluents.end());
    negative.insert(fluents.begin(), fluents.end());
}

void GreaterEqualsExpression::classifyActionFluents(ActionFluentSet& positive,
                                                    ActionFluentSet& negative) {
    ActionFluentSet fluents;
    Connective::classifyActionFluents(fluents, fluents);

    // All actions depend positively and negatively
    positive.insert(fluents.begin(), fluents.end());
    negative.insert(fluents.begin(), fluents.end());
}

void LowerEqualsExpression::classifyActionFluents(ActionFluentSet& positive,
                                                  ActionFluentSet& negative) {
    ActionFluentSet fluents;
    Connective::classifyActionFluents(fluents, fluents);

    // All actions depend positively and negatively
    positive.insert(fluents.begin(), fluents.end());
    negative.insert(fluents.begin(), fluents.end());
}

void Subtraction::classifyActionFluents(ActionFluentSet& positive,
                                        ActionFluentSet& negative) {
    exprs[0]->classifyActionFluents(positive, negative);

    for (unsigned int i = 0; i < exprs.size(); ++i) {
        // Negative and positive are reversed for all elements besides the first
        exprs[i]->classifyActionFluents(negative, positive);
    }
}

void Multiplication::classifyActionFluents(ActionFluentSet& positive,
                                           ActionFluentSet& negative) {
    assert(exprs.size() == 2);

    ActionFluentSet tmpPos0;
    ActionFluentSet tmpNeg0;
    exprs[0]->classifyActionFluents(tmpPos0, tmpNeg0);

    ActionFluentSet tmpPos1;
    ActionFluentSet tmpNeg1;
    exprs[1]->classifyActionFluents(tmpPos1, tmpNeg1);

    NumericConstant* tmp0 = dynamic_cast<NumericConstant*>(exprs[0]);
    NumericConstant* tmp1 = dynamic_cast<NumericConstant*>(exprs[1]);

    if ((tmp0 && tmp1) || (!tmp0 && !tmp1) ||
        (tmp0 && MathUtils::doubleIsGreaterOrEqual(tmp0->value, 0.0)) ||
        (tmp1 && MathUtils::doubleIsGreaterOrEqual(tmp1->value, 0.0))) {
        positive.insert(tmpPos0.begin(), tmpPos0.end());
        positive.insert(tmpPos1.begin(), tmpPos1.end());

        negative.insert(tmpNeg0.begin(), tmpNeg0.end());
        negative.insert(tmpNeg1.begin(), tmpNeg1.end());
    } else if (tmp0) {
        assert(MathUtils::doubleIsSmaller(tmp0->value, 0.0));
        assert(tmpPos0.empty() && tmpNeg0.empty());
        positive.insert(tmpNeg1.begin(), tmpNeg1.end());
        negative.insert(tmpPos1.begin(), tmpPos1.end());
    } else {
        assert(tmp1 && MathUtils::doubleIsSmaller(tmp1->value, 0.0));
        assert(tmpPos1.empty() && tmpNeg1.empty());
        positive.insert(tmpNeg0.begin(), tmpNeg0.end());
        negative.insert(tmpPos0.begin(), tmpPos0.end());
    }
}

void Division::classifyActionFluents(ActionFluentSet& positive,
                                     ActionFluentSet& negative) {
    assert(exprs.size() == 2);

    ActionFluentSet tmpPos0;
    ActionFluentSet tmpNeg0;
    exprs[0]->classifyActionFluents(tmpPos0, tmpNeg0);

    ActionFluentSet tmpPos1;
    ActionFluentSet tmpNeg1;
    exprs[1]->classifyActionFluents(tmpPos1, tmpNeg1);

    NumericConstant* tmp0 = dynamic_cast<NumericConstant*>(exprs[0]);
    NumericConstant* tmp1 = dynamic_cast<NumericConstant*>(exprs[1]);

    if ((tmp0 && tmp1) || (!tmp0 && !tmp1) ||
        (tmp0 && MathUtils::doubleIsGreaterOrEqual(tmp0->value, 0.0)) ||
        (tmp1 && MathUtils::doubleIsGreaterOrEqual(tmp1->value, 0.0))) {
        positive.insert(tmpPos0.begin(), tmpPos0.end());
        positive.insert(tmpPos1.begin(), tmpPos1.end());

        negative.insert(tmpNeg0.begin(), tmpNeg0.end());
        negative.insert(tmpNeg1.begin(), tmpNeg1.end());
    } else if (tmp0) {
        assert(MathUtils::doubleIsSmaller(tmp0->value, 0.0));
        assert(tmpPos0.empty() && tmpNeg0.empty());
        positive.insert(tmpNeg1.begin(), tmpNeg1.end());
        negative.insert(tmpPos1.begin(), tmpPos1.end());
    } else {
        assert(tmp1 && MathUtils::doubleIsSmaller(tmp1->value, 0.0));
        assert(tmpPos1.empty() && tmpNeg1.empty());
        positive.insert(tmpNeg0.begin(), tmpNeg0.end());
        negative.insert(tmpPos0.begin(), tmpPos0.end());
    }
}

/*****************************************************************
                          Unaries
*****************************************************************/

void Negation::classifyActionFluents(ActionFluentSet& positive,
                                     set<ActionFluent*>& negative) {
    ActionFluentSet pos;
    ActionFluentSet neg;
    expr->classifyActionFluents(neg, pos);
    positive.insert(pos.begin(), pos.end());
    negative.insert(neg.begin(), neg.end());
}

void ExponentialFunction::classifyActionFluents(ActionFluentSet& positive,
                                                set<ActionFluent*>& negative) {
    expr->classifyActionFluents(positive, negative);
}

/*****************************************************************
                         Conditionals
*****************************************************************/

void IfThenElseExpression::classifyActionFluents(ActionFluentSet& positive,
                                                 ActionFluentSet& negative) {
    // TODO: For now, to simplify things, all action fluents that occur in the
    // condition are both positive and negative
    ActionFluentSet pos;
    ActionFluentSet neg;
    condition->classifyActionFluents(pos, neg);
    positive.insert(pos.begin(), pos.end());
    negative.insert(neg.begin(), neg.end());

    pos.clear();
    neg.clear();
    valueIfTrue->classifyActionFluents(pos, neg);
    positive.insert(pos.begin(), pos.end());
    negative.insert(neg.begin(), neg.end());

    pos.clear();
    neg.clear();
    valueIfFalse->classifyActionFluents(pos, neg);
    positive.insert(pos.begin(), pos.end());
    negative.insert(neg.begin(), neg.end());
}

void MultiConditionChecker::classifyActionFluents(ActionFluentSet& positive,
                                                  ActionFluentSet& negative) {
    for (unsigned int i = 0; i < conditions.size(); ++i) {
        // TODO: For now, to simplify things, all action fluents that occur in
        // the condition are both positive and negative
        ActionFluentSet pos;
        ActionFluentSet neg;
        conditions[i]->classifyActionFluents(pos, neg);
        positive.insert(pos.begin(), pos.end());
        negative.insert(neg.begin(), neg.end());

        pos.clear();
        neg.clear();
        effects[i]->classifyActionFluents(pos, neg);
        positive.insert(pos.begin(), pos.end());
        negative.insert(neg.begin(), neg.end());
    }
}
