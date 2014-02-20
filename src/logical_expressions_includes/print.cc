/*****************************************************************
                         Schematics
*****************************************************************/

void VariableDefinition::print(ostream& out) {
    out << name << "(";
    for(unsigned int i = 0; i < params.size(); ++i) {
        out << params[i]->name;
        if(i != params.size()-1) {
            out << ", ";
        }
    }
    out << ") : " << "VariableType = ";
    switch(variableType) {
    case VariableDefinition::STATE_FLUENT:
        out << "state-fluent, ";
        break;
    case VariableDefinition::ACTION_FLUENT:
        out << "action-fluent, ";
        break;
    case VariableDefinition::INTERM_FLUENT:
        out << "interm-fluent, ";
        break;
    case VariableDefinition::NON_FLUENT:
        out << "non-fluent, ";
        break;
    }
    out << "ValueType = " << valueType->name << ", defaultValue = " << defaultValue << ", level = " << level;
}

void Parameter::print(ostream& out) {
    out << name << " ";
}

void ParameterList::print(ostream& out) {
    out << "(";
    for(unsigned int i = 0; i < params.size(); ++i) {
        out << params[i]->name << " : " << types[i]->name;
        if(i != (params.size() -1)) {
            out << ", ";
        }
    }
    out << ") ";
}

void UninstantiatedVariable::print(ostream& out) {
    out << name;
}

/*****************************************************************
                           Atomics
*****************************************************************/

void AtomicLogicalExpression::print(ostream& out) {
    out << name;
}

void NumericConstant::print(ostream& out) {
    out << value;
}

void Object::print(ostream& out) {
    assert(value < type->domain.size());
    assert(name == type->domain[value]->name);
    out << name << " (" << value << ") ";
}


/*****************************************************************
                          Quantifier
*****************************************************************/

void Quantifier::print(ostream& out) {
    paramList->print(out);
    expr->print(out);
}

void Sumation::print(ostream& out) {
    out << "(sum ";
    Quantifier::print(out);
    out << ")";
}

void Product::print(ostream& out) {
    out << "(prod ";
    Quantifier::print(out);
    out << ")";
}

void UniversalQuantification::print(ostream& out) {
    out << "(forall ";
    Quantifier::print(out);
    out << ")";
}

void ExistentialQuantification::print(ostream& out) {
    out << "(exists ";
    Quantifier::print(out);
    out << ")";
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Connective::print(ostream& out) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        out << " ";
        exprs[i]->print(out);
    }
}

void Conjunction::print(ostream& out) {
    out << "(and";
    Connective::print(out);
    out << ") ";
}

void Disjunction::print(ostream& out) {
    out << "(or";
    Connective::print(out);
    out << ") ";
}

void EqualsExpression::print(ostream& out) {
    out << "(==";
    Connective::print(out);
    out << ") ";
}

void GreaterExpression::print(ostream& out) {
    assert(exprs.size() == 2);
    out << "(> ";
    Connective::print(out);
    out << ") ";
}

void LowerExpression::print(ostream& out) {
    assert(exprs.size() == 2);
    out << "(< ";
    Connective::print(out);
    out << ") ";
}

void GreaterEqualsExpression::print(ostream& out) {
    assert(exprs.size() == 2);
    out << "(>= ";
    Connective::print(out);
    out << ") ";
}

void LowerEqualsExpression::print(ostream& out) {
    assert(exprs.size() == 2);
    out << "(<= ";
    Connective::print(out);
    out << ") ";
}

void Addition::print(ostream& out) {
    out << "(+";
    Connective::print(out);
    out << ") ";
}

void Subtraction::print(ostream& out) {
    out << "(-";
    Connective::print(out);
    out << ") ";
}

void Multiplication::print(ostream& out) {
    out << "(*";
    Connective::print(out);
    out << ") ";
}

void Division::print(ostream& out) {
    out << "(/";
    Connective::print(out);
    out << ") ";
}

/*****************************************************************
                          Unaries
*****************************************************************/

void Negation::print(ostream& out) {
    out << "(not ";
    expr->print(out);
    out << ") ";
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

void KronDeltaDistribution::print(ostream& out) {
    out << "KronDelta(";
    expr->print(out);
    out << ") ";
}

void BernoulliDistribution::print(ostream& out) {
    out << "Bernoulli(";
    expr->print(out);
    out << ") ";
}

void DiscreteDistribution::print(ostream& out) {
    out << "Discrete( ";
    for(unsigned int i = 0; i < values.size(); ++i) {
        out << "[";
        values[i]->print(out);
        out << " : ";
        probabilities[i]->print(out);
        out << "] ";	
    }
    out << ")";
}

/*****************************************************************
                         Conditionals
*****************************************************************/

void IfThenElseExpression::print(ostream& out) {
    out << "(if ";
    condition->print(out);
    out << " then ";
    valueIfTrue->print(out);
    out << " else ";
    valueIfFalse->print(out);
    out << ")";
}

void MultiConditionChecker::print(ostream& out) {
    out << " [ ";
    for(unsigned int i = 0; i < conditions.size(); ++i) {
        if(i == 0) {
            out << "(if ";
            conditions[i]->print(out);
            out << " then ";
        } else if(i == conditions.size() -1) {
            out << "(default ";
            assert(MathUtils::doubleIsEqual(dynamic_cast<NumericConstant*>(conditions[i])->value, 1.0));
        } else {
            out << "(elif ";
            conditions[i]->print(out);
            out << " then ";
        }
        effects[i]->print(out);
        out << " ) ";
    }
    out << " ] ";
}
