void VariableDefinition::print() {
    cout << name << "(";
    for(unsigned int i = 0; i < params.size(); ++i) {
        cout << params[i]->name;
        if(i != params.size()-1) {
            cout << ", ";
        }
    }
    cout << ") : " << "VariableType = ";
    switch(variableType) {
    case VariableDefinition::STATE_FLUENT:
        cout << "state-fluent, ";
        break;
    case VariableDefinition::ACTION_FLUENT:
        cout << "action-fluent, ";
        break;
    case VariableDefinition::INTERM_FLUENT:
        cout << "interm-fluent, ";
        break;
    case VariableDefinition::NON_FLUENT:
        cout << "non-fluent, ";
        break;
    }
    cout << "ValueType = " << valueType->name << ", defaultValue = " << defaultValue << ", level = " << level;
}

/*
void ConditionalProbabilityFunctionDefinition::print() {
    head->print();
    cout << " = ";
    body->print();
    cout << endl;
}
*/

void StateActionConstraint::print() {
    sac->print();
    cout << endl;
}

void ParameterDefinition::print() {
    cout << "(";
    cout << parameterName;
    cout << " : ";
    cout << parameterType->name;
    cout << ") ";
}

void ParameterDefinitionSet::print() {
    cout << "(";
    for(unsigned int i = 0; i < parameterDefs.size(); ++i) {
        parameterDefs[i]->print();
        cout << " ";
    }
    cout << ") ";
}

/*
void ConditionalProbabilityFunction::print() {
    head->print();
    cout << " = ";
    body->print();
    cout << endl;
}
*/

void UninstantiatedVariable::print() {
    cout << name;
}

void AtomicLogicalExpression::print() {
    cout << name;
}

void NumericConstant::print() {
    cout << value;
}

void Quantifier::print() {
    parameterDefsSet->print();
    expr->print();
}

void Sumation::print() {
    cout << "(sum ";
    Quantifier::print();
    cout << ")";
}

void Product::print() {
    cout << "(prod ";
    Quantifier::print();
    cout << ")";
}

void UniversalQuantification::print() {
    cout << "(forall ";
    Quantifier::print();
    cout << ")";
}

void ExistentialQuantification::print() {
    cout << "(exists ";
    Quantifier::print();
    cout << ")";
}

void Connective::print() {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        cout << " ";
        exprs[i]->print();
    }
}

void Conjunction::print() {
    cout << "(and";
    Connective::print();
    cout << ") ";
}

void Disjunction::print() {
    cout << "(or";
    Connective::print();
    cout << ") ";
}

void EqualsExpression::print() {
    cout << "(=";
    Connective::print();
    cout << ") ";
}

void GreaterExpression::print() {
    assert(exprs.size() == 2);
    cout << "(> ";
    Connective::print();
    cout << ") ";
}

void LowerExpression::print() {
    assert(exprs.size() == 2);
    cout << "(< ";
    Connective::print();
    cout << ") ";
}

void GreaterEqualsExpression::print() {
    assert(exprs.size() == 2);
    cout << "(>= ";
    Connective::print();
    cout << ") ";
}

void LowerEqualsExpression::print() {
    assert(exprs.size() == 2);
    cout << "(<= ";
    Connective::print();
    cout << ") ";
}

void Addition::print() {
    cout << "(+";
    Connective::print();
    cout << ") ";
}

void Subtraction::print() {
    cout << "(-";
    Connective::print();
    cout << ") ";
}

void Multiplication::print() {
    cout << "(*";
    Connective::print();
    cout << ") ";
}

void Division::print() {
    cout << "(/";
    Connective::print();
    cout << ") ";
}

void BernoulliDistribution::print() {
    cout << "Bernoulli(";
    expr->print();
    cout << ") ";
}

void KronDeltaDistribution::print() {
    cout << "KronDelta(";
    expr->print();
    cout << ") ";
}

void IfThenElseExpression::print() {
    cout << "(if ";
    condition->print();
    cout << " then ";
    valueIfTrue->print();
    cout << " else ";
    valueIfFalse->print();
    cout << ")";
}

void MultiConditionChecker::print() {
    cout << " [ ";
    for(unsigned int i = 0; i < conditions.size(); ++i) {
        if(i == 0) {
            cout << "(if ";
            conditions[i]->print();
            cout << " then ";
        } else if(i == conditions.size() -1) {
            cout << "(default ";
            assert(dynamic_cast<NumericConstant*>(conditions[i]) == NumericConstant::truth());
        } else {
            cout << "(elif ";
            conditions[i]->print();
            cout << " then ";
        }
        effects[i]->print();
        cout << " ) ";
    }
    cout << " ] ";
}

void NegateExpression::print() {
    cout << "(not ";
    expr->print();
    cout << ") ";
}
