void LogicalExpression::classifyActionFluents(
        set<ActionFluent*>& /*positiveDependentActionFluents*/,
        set<ActionFluent*>& /*negativeDependentActionFluents*/) {
    print(cout);
    assert(false);
    SystemUtils::abort(
            "Error: classify action fluents called on logical expression that doesn't support it.");
}

/*****************************************************************
                           Atomics
*****************************************************************/

void StateFluent::classifyActionFluents(
        set<ActionFluent*>& /*positiveDependentActionFluents*/,
        set<ActionFluent*>& /*negativeDependentActionFluents*/) {}

void ActionFluent::classifyActionFluents(
        set<ActionFluent*>& positiveDependentActionFluents,
        set<ActionFluent*>& /*negativeDependentActionFluents*/) {
    positiveDependentActionFluents.insert(this);
}

void NumericConstant::classifyActionFluents(
        set<ActionFluent*>& /*positiveDependentActionFluents*/,
        set<ActionFluent*>& /*negativeDependentActionFluents*/) {}

/*****************************************************************
                           Connectives
*****************************************************************/

void Connective::classifyActionFluents(
        set<ActionFluent*>& positiveDependentActionFluents,
        set<ActionFluent*>& negativeDependentActionFluents) {
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->classifyActionFluents(positiveDependentActionFluents,
                negativeDependentActionFluents);
    }
}

void EqualsExpression::classifyActionFluents(
        set<ActionFluent*>& positiveDependentActionFluents,
        set<ActionFluent*>& negativeDependentActionFluents) {
    set<ActionFluent*> fluents;
    Connective::classifyActionFluents(fluents, fluents);

    // All actions depend positively and negatively
    positiveDependentActionFluents.insert(fluents.begin(), fluents.end());
    negativeDependentActionFluents.insert(fluents.begin(), fluents.end());
}

void GreaterExpression::classifyActionFluents(
        set<ActionFluent*>& positiveDependentActionFluents,
        set<ActionFluent*>& negativeDependentActionFluents) {
    set<ActionFluent*> fluents;
    Connective::classifyActionFluents(fluents, fluents);

    // All actions depend positively and negatively
    positiveDependentActionFluents.insert(fluents.begin(), fluents.end());
    negativeDependentActionFluents.insert(fluents.begin(), fluents.end());
}

void LowerExpression::classifyActionFluents(
        set<ActionFluent*>& positiveDependentActionFluents,
        set<ActionFluent*>& negativeDependentActionFluents) {
    set<ActionFluent*> fluents;
    Connective::classifyActionFluents(fluents, fluents);

    // All actions depend positively and negatively
    positiveDependentActionFluents.insert(fluents.begin(), fluents.end());
    negativeDependentActionFluents.insert(fluents.begin(), fluents.end());
}

void GreaterEqualsExpression::classifyActionFluents(
        set<ActionFluent*>& positiveDependentActionFluents,
        set<ActionFluent*>& negativeDependentActionFluents) {
    set<ActionFluent*> fluents;
    Connective::classifyActionFluents(fluents, fluents);

    // All actions depend positively and negatively
    positiveDependentActionFluents.insert(fluents.begin(), fluents.end());
    negativeDependentActionFluents.insert(fluents.begin(), fluents.end());
}

void LowerEqualsExpression::classifyActionFluents(
        set<ActionFluent*>& positiveDependentActionFluents,
        set<ActionFluent*>& negativeDependentActionFluents) {
    set<ActionFluent*> fluents;
    Connective::classifyActionFluents(fluents, fluents);

    // All actions depend positively and negatively
    positiveDependentActionFluents.insert(fluents.begin(), fluents.end());
    negativeDependentActionFluents.insert(fluents.begin(), fluents.end());
}

void Subtraction::classifyActionFluents(
        set<ActionFluent*>& positiveDependentActionFluents,
        set<ActionFluent*>& negativeDependentActionFluents) {
    exprs[0]->classifyActionFluents(positiveDependentActionFluents,
            negativeDependentActionFluents);

    for (unsigned int i = 0; i < exprs.size(); ++i) {
        // Negative and positive are reversed for all elements besides the first
        exprs[1]->classifyActionFluents(negativeDependentActionFluents,
                positiveDependentActionFluents);
    }
}

void Multiplication::classifyActionFluents(
        set<ActionFluent*>& positiveDependentActionFluents,
        set<ActionFluent*>& negativeDependentActionFluents) {
    assert(exprs.size() == 2);

    set<ActionFluent*> tmpPos0;
    set<ActionFluent*> tmpNeg0;
    exprs[0]->classifyActionFluents(tmpPos0, tmpNeg0);

    set<ActionFluent*> tmpPos1;
    set<ActionFluent*> tmpNeg1;
    exprs[1]->classifyActionFluents(tmpPos1, tmpNeg1);

    NumericConstant* tmp0 = dynamic_cast<NumericConstant*>(exprs[0]);
    NumericConstant* tmp1 = dynamic_cast<NumericConstant*>(exprs[1]);

    if ((tmp0 &&
         tmp1) ||
        (!tmp0 &&
         !tmp1) ||
        (tmp0 &&
         MathUtils::doubleIsGreaterOrEqual(tmp0->value,
                 0.0)) ||
        (tmp1 && MathUtils::doubleIsGreaterOrEqual(tmp1->value, 0.0))) {
        positiveDependentActionFluents.insert(tmpPos0.begin(), tmpPos0.end());
        positiveDependentActionFluents.insert(tmpPos1.begin(), tmpPos1.end());

        negativeDependentActionFluents.insert(tmpNeg0.begin(), tmpNeg0.end());
        negativeDependentActionFluents.insert(tmpNeg1.begin(), tmpNeg1.end());
    } else if (tmp0) {
        assert(MathUtils::doubleIsSmaller(tmp0->value, 0.0));
        assert(tmpPos0.empty() && tmpNeg0.empty());
        positiveDependentActionFluents.insert(tmpNeg1.begin(), tmpNeg1.end());
        negativeDependentActionFluents.insert(tmpPos1.begin(), tmpPos1.end());
    } else {
        assert(tmp1 && MathUtils::doubleIsSmaller(tmp1->value, 0.0));
        assert(tmpPos1.empty() && tmpNeg1.empty());
        positiveDependentActionFluents.insert(tmpNeg0.begin(), tmpNeg0.end());
        negativeDependentActionFluents.insert(tmpPos0.begin(), tmpPos0.end());
    }
}

void Division::classifyActionFluents(
        set<ActionFluent*>& positiveDependentActionFluents,
        set<ActionFluent*>& negativeDependentActionFluents) {
    assert(exprs.size() == 2);

    set<ActionFluent*> tmpPos0;
    set<ActionFluent*> tmpNeg0;
    exprs[0]->classifyActionFluents(tmpPos0, tmpNeg0);

    set<ActionFluent*> tmpPos1;
    set<ActionFluent*> tmpNeg1;
    exprs[1]->classifyActionFluents(tmpPos1, tmpNeg1);

    NumericConstant* tmp0 = dynamic_cast<NumericConstant*>(exprs[0]);
    NumericConstant* tmp1 = dynamic_cast<NumericConstant*>(exprs[1]);

    if ((tmp0 &&
         tmp1) ||
        (!tmp0 &&
         !tmp1) ||
        (tmp0 &&
         MathUtils::doubleIsGreaterOrEqual(tmp0->value,
                 0.0)) ||
        (tmp1 && MathUtils::doubleIsGreaterOrEqual(tmp1->value, 0.0))) {
        positiveDependentActionFluents.insert(tmpPos0.begin(), tmpPos0.end());
        positiveDependentActionFluents.insert(tmpPos1.begin(), tmpPos1.end());

        negativeDependentActionFluents.insert(tmpNeg0.begin(), tmpNeg0.end());
        negativeDependentActionFluents.insert(tmpNeg1.begin(), tmpNeg1.end());
    } else if (tmp0) {
        assert(MathUtils::doubleIsSmaller(tmp0->value, 0.0));
        assert(tmpPos0.empty() && tmpNeg0.empty());
        positiveDependentActionFluents.insert(tmpNeg1.begin(), tmpNeg1.end());
        negativeDependentActionFluents.insert(tmpPos1.begin(), tmpPos1.end());
    } else {
        assert(tmp1 && MathUtils::doubleIsSmaller(tmp1->value, 0.0));
        assert(tmpPos1.empty() && tmpNeg1.empty());
        positiveDependentActionFluents.insert(tmpNeg0.begin(), tmpNeg0.end());
        negativeDependentActionFluents.insert(tmpPos0.begin(), tmpPos0.end());
    }
}

/*****************************************************************
                          Unaries
*****************************************************************/

void Negation::classifyActionFluents(
        set<ActionFluent*>& positiveDependentActionFluents,
        set <ActionFluent*>& negativeDependentActionFluents) {
    set<ActionFluent*> pos;
    set<ActionFluent*> neg;
    expr->classifyActionFluents(neg, pos);
    positiveDependentActionFluents.insert(pos.begin(), pos.end());
    negativeDependentActionFluents.insert(neg.begin(), neg.end());
}

/*****************************************************************
                         Conditionals
*****************************************************************/

void IfThenElseExpression::classifyActionFluents(
        set<ActionFluent*>& positiveDependentActionFluents,
        set<ActionFluent*>& negativeDependentActionFluents) {
    // TODO: For now, to simplify things, all action fluents that occur in the
    // condition are both positive and negative
    set<ActionFluent*> pos;
    set<ActionFluent*> neg;
    condition->classifyActionFluents(pos, neg);
    positiveDependentActionFluents.insert(pos.begin(), pos.end());
    negativeDependentActionFluents.insert(neg.begin(), neg.end());

    pos.clear();
    neg.clear();
    valueIfTrue->classifyActionFluents(pos, neg);
    positiveDependentActionFluents.insert(pos.begin(), pos.end());
    negativeDependentActionFluents.insert(neg.begin(), neg.end());

    pos.clear();
    neg.clear();
    valueIfFalse->classifyActionFluents(pos, neg);
    positiveDependentActionFluents.insert(pos.begin(), pos.end());
    negativeDependentActionFluents.insert(neg.begin(), neg.end());
}

void MultiConditionChecker::classifyActionFluents(
        set<ActionFluent*>& positiveDependentActionFluents,
        set<ActionFluent*>& negativeDependentActionFluents) {
    for (unsigned int i = 0; i < conditions.size(); ++i) {
        // TODO: For now, to simplify things, all action fluents that occur in
        // the condition are both positive and negative
        set<ActionFluent*> pos;
        set<ActionFluent*> neg;
        conditions[i]->classifyActionFluents(pos, neg);
        positiveDependentActionFluents.insert(pos.begin(), pos.end());
        negativeDependentActionFluents.insert(neg.begin(), neg.end());

        pos.clear();
        neg.clear();
        effects[i]->classifyActionFluents(pos, neg);
        positiveDependentActionFluents.insert(pos.begin(), pos.end());
        negativeDependentActionFluents.insert(neg.begin(), neg.end());
    }
}
