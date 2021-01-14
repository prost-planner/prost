void LogicalExpression::prettyPrint(ostream& /*out*/) const {
    assert(false);
}

/*****************************************************************
                         Schematics
*****************************************************************/

void Parameter::prettyPrint(ostream& out) const {
    out << name << " ";
}

void ParameterList::prettyPrint(ostream& out) const {
    out << "(";
    for (unsigned int i = 0; i < params.size(); ++i) {
        out << params[i]->name << " : " << types[i]->name;
        if (i != (params.size() - 1)) {
            out << ", ";
        }
    }
    out << ") ";
}

void ActionFluent::prettyPrint(ostream& out) const {
    out << fullName;
}

void StateFluent::prettyPrint(ostream& out) const {
    out << fullName;
}

void ParametrizedVariable::prettyPrint(ostream& out) const {
    out << fullName;
}

/*****************************************************************
                           Atomics
*****************************************************************/

void NumericConstant::prettyPrint(ostream& out) const {
    out << value;
}

void Object::prettyPrint(ostream& out) const {
    out << name << "(" << value << ") ";
}

/*****************************************************************
                          Quantifier
*****************************************************************/

void Sumation::prettyPrint(ostream& out) const {
    out << "sum_{";
    paramList->prettyPrint(out);
    out << "} [";
    expr->prettyPrint(out);
    out << "]";
}

void Product::prettyPrint(ostream& out) const {
    out << "prod_{";
    paramList->prettyPrint(out);
    out << "} [";
    expr->prettyPrint(out);
    out << "]";
}

void UniversalQuantification::prettyPrint(ostream& out) const {
    out << "forall_{";
    paramList->prettyPrint(out);
    out << "} [";
    expr->prettyPrint(out);
    out << "]";
}

void ExistentialQuantification::prettyPrint(ostream& out) const {
    out << "exists_{";
    paramList->prettyPrint(out);
    out << "} [";
    expr->prettyPrint(out);
    out << "]";
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Connective::prettyPrint(ostream& out) const {
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->prettyPrint(out);
        if (i != exprs.size() - 1) {
            out << " ";
        }
    }
}

void Conjunction::prettyPrint(ostream& out) const {
    out << "and(";
    Connective::prettyPrint(out);
    out << ")";
}

void Disjunction::prettyPrint(ostream& out) const {
    out << "or(";
    Connective::prettyPrint(out);
    out << ")";
}

void EqualsExpression::prettyPrint(ostream& out) const {
    if ((exprs.size() == 2)) {
        ActionFluent* af = static_cast<ActionFluent*>(exprs[0]);
        NumericConstant* val = static_cast<NumericConstant*>(exprs[1]);
        if (af && val) {
            out << af->valueType->objects[val->value]->name;
            return;
        }
    }
    out << "==(";
    Connective::prettyPrint(out);
    out << ")";
}

void GreaterExpression::prettyPrint(ostream& out) const {
    assert(exprs.size() == 2);
    out << ">(";
    Connective::prettyPrint(out);
    out << ")";
}

void LowerExpression::prettyPrint(ostream& out) const {
    assert(exprs.size() == 2);
    out << "<(";
    Connective::prettyPrint(out);
    out << ")";
}

void GreaterEqualsExpression::prettyPrint(ostream& out) const {
    assert(exprs.size() == 2);
    out << ">=(";
    Connective::prettyPrint(out);
    out << ")";
}

void LowerEqualsExpression::prettyPrint(ostream& out) const {
    assert(exprs.size() == 2);
    out << "<=(";
    Connective::prettyPrint(out);
    out << ")";
}

void Addition::prettyPrint(ostream& out) const {
    out << "+(";
    Connective::prettyPrint(out);
    out << ")";
}

void Subtraction::prettyPrint(ostream& out) const {
    out << "-(";
    Connective::prettyPrint(out);
    out << ")";
}

void Multiplication::prettyPrint(ostream& out) const {
    out << "*(";
    Connective::prettyPrint(out);
    out << ")";
}

void Division::prettyPrint(ostream& out) const {
    out << "/(";
    Connective::prettyPrint(out);
    out << ")";
}

/*****************************************************************
                          Unaries
*****************************************************************/

void Negation::prettyPrint(ostream& out) const {
    out << "not(";
    expr->prettyPrint(out);
    out << ")";
}

void ExponentialFunction::prettyPrint(ostream& out) const {
    out << "exp(";
    expr->prettyPrint(out);
    out << ")";
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

void KronDeltaDistribution::prettyPrint(ostream& out) const {
    expr->prettyPrint(out);
}

void BernoulliDistribution::prettyPrint(ostream& out) const {
    out << "Bernoulli(";
    expr->prettyPrint(out);
    out << ")";
}

void DiscreteDistribution::prettyPrint(ostream& out) const {
    out << "Discrete( ";
    for (unsigned int i = 0; i < values.size(); ++i) {
        out << "(";
        values[i]->prettyPrint(out);
        out << " : ";
        probabilities[i]->prettyPrint(out);
        out << ") ";
    }
    out << ")";
}

/*****************************************************************
                         Conditionals
*****************************************************************/

void IfThenElseExpression::prettyPrint(ostream& out) const {
    out << "switch( (";
    condition->prettyPrint(out);
    out << " : ";
    valueIfTrue->prettyPrint(out);
    // IfThenElseExpression and MultiConditionChecker are no longer
    // distinguished in the search part, so a condition is inserted for the else
    // case (the constant 1)
    out << ") ($c(1) : ";
    valueIfFalse->prettyPrint(out);
    out << ") )";
}

void MultiConditionChecker::prettyPrint(ostream& out) const {
    out << "switch( ";
    for (unsigned int i = 0; i < conditions.size(); ++i) {
        out << "(";
        conditions[i]->prettyPrint(out);
        out << " : ";
        effects[i]->prettyPrint(out);
        out << ") ";
    }
    out << ")";
}
