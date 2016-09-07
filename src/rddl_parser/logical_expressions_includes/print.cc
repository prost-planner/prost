void LogicalExpression::print(ostream& /*out*/) const {
    assert(false);
}

/*****************************************************************
                         Schematics
*****************************************************************/

void Parameter::print(ostream& out) const {
    out << name << " ";
}

void ParameterList::print(ostream& out) const {
    out << "(";
    for (unsigned int i = 0; i < params.size(); ++i) {
        out << params[i]->name << " : " << types[i]->name;
        if (i != (params.size() - 1)) {
            out << ", ";
        }
    }
    out << ") ";
}

void ActionFluent::print(ostream& out) const {
    out << "$a(" << index << ")";
    // out << fullName;
}

void StateFluent::print(ostream& out) const {
    out << "$s(" << index << ")";
    // out << fullName;
}

void ParametrizedVariable::print(ostream& out) const {
    out << fullName;
}

/*****************************************************************
                           Atomics
*****************************************************************/

void NumericConstant::print(ostream& out) const {
    out << "$c(" << value << ")";
    // out << value;
}

void Object::print(ostream& out) const {
    out << name << "(" << value << ") ";
}

/*****************************************************************
                          Quantifier
*****************************************************************/

void Sumation::print(ostream& out) const {
    out << "sum_{";
    paramList->print(out);
    out << "} [";
    expr->print(out);
    out << "]";
}

void Product::print(ostream& out) const {
    out << "prod_{";
    paramList->print(out);
    out << "} [";
    expr->print(out);
    out << "]";
}

void UniversalQuantification::print(ostream& out) const {
    out << "forall_{";
    paramList->print(out);
    out << "} [";
    expr->print(out);
    out << "]";
}

void ExistentialQuantification::print(ostream& out) const {
    out << "exists_{";
    paramList->print(out);
    out << "} [";
    expr->print(out);
    out << "]";
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Connective::print(ostream& out) const {
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->print(out);
        if (i != exprs.size() - 1) {
            out << " ";
        }
    }
}

void Conjunction::print(ostream& out) const {
    out << "and(";
    Connective::print(out);
    out << ")";
}

void Disjunction::print(ostream& out) const {
    out << "or(";
    Connective::print(out);
    out << ")";
}

void EqualsExpression::print(ostream& out) const {
    out << "==(";
    Connective::print(out);
    out << ")";
}

void GreaterExpression::print(ostream& out) const {
    assert(exprs.size() == 2);
    out << ">(";
    Connective::print(out);
    out << ")";
}

void LowerExpression::print(ostream& out) const {
    assert(exprs.size() == 2);
    out << "<(";
    Connective::print(out);
    out << ")";
}

void GreaterEqualsExpression::print(ostream& out) const {
    assert(exprs.size() == 2);
    out << ">=(";
    Connective::print(out);
    out << ")";
}

void LowerEqualsExpression::print(ostream& out) const {
    assert(exprs.size() == 2);
    out << "<=(";
    Connective::print(out);
    out << ")";
}

void Addition::print(ostream& out) const {
    out << "+(";
    Connective::print(out);
    out << ")";
}

void Subtraction::print(ostream& out) const {
    out << "-(";
    Connective::print(out);
    out << ")";
}

void Multiplication::print(ostream& out) const {
    out << "*(";
    Connective::print(out);
    out << ")";
}

void Division::print(ostream& out) const {
    out << "/(";
    Connective::print(out);
    out << ")";
}

/*****************************************************************
                          Unaries
*****************************************************************/

void Negation::print(ostream& out) const {
    out << "~(";
    expr->print(out);
    out << ")";
}

void ExponentialFunction::print(ostream& out) const {
    out << "exp(";
    expr->print(out);
    out << ")";
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

void KronDeltaDistribution::print(ostream& out) const {
    expr->print(out);
}

void BernoulliDistribution::print(ostream& out) const {
    out << "Bernoulli(";
    expr->print(out);
    out << ")";
}

void DiscreteDistribution::print(ostream& out) const {
    out << "Discrete( ";
    for (unsigned int i = 0; i < values.size(); ++i) {
        out << "(";
        values[i]->print(out);
        out << " : ";
        probabilities[i]->print(out);
        out << ") ";
    }
    out << ")";
}

/*****************************************************************
                         Conditionals
*****************************************************************/

void IfThenElseExpression::print(ostream& out) const {
    out << "switch( (";
    condition->print(out);
    out << " : ";
    valueIfTrue->print(out);
    // IfThenElseExpression and MultiConditionChecker are no longer
    // distinguished in the search part, so a condition is inserted for the else
    // case (the constant 1)
    out << ") ($c(1) : ";
    valueIfFalse->print(out);
    out << ") )";
}

void MultiConditionChecker::print(ostream& out) const {
    out << "switch( ";
    for (unsigned int i = 0; i < conditions.size(); ++i) {
        out << "(";
        conditions[i]->print(out);
        out << " : ";
        effects[i]->print(out);
        out << ") ";
    }
    out << ")";
}
