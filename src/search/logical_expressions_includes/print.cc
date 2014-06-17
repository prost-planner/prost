/*****************************************************************
                           Atomics
*****************************************************************/

void StateFluent::print(ostream& out) const {
    out << name;
}

void ActionFluent::print(ostream& out) const {
    out << name;
}

void NumericConstant::print(ostream& out) const {
    out << value;
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Conjunction::print(ostream& out) const {
    out << "(and";
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        out << " ";
        exprs[i]->print(out);
    }
    out << ") ";
}

void Disjunction::print(ostream& out) const {
    out << "(or";
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        out << " ";
        exprs[i]->print(out);
    }
    out << ") ";
}

void EqualsExpression::print(ostream& out) const {
    out << "(==";
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        out << " ";
        exprs[i]->print(out);
    }
    out << ") ";
}

void GreaterExpression::print(ostream& out) const {
    assert(exprs.size() == 2);
    out << "(> ";
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        out << " ";
        exprs[i]->print(out);
    }
    out << ") ";
}

void LowerExpression::print(ostream& out) const {
    assert(exprs.size() == 2);
    out << "(< ";
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        out << " ";
        exprs[i]->print(out);
    }
    out << ") ";
}

void GreaterEqualsExpression::print(ostream& out) const {
    assert(exprs.size() == 2);
    out << "(>= ";
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        out << " ";
        exprs[i]->print(out);
    }
    out << ") ";
}

void LowerEqualsExpression::print(ostream& out) const {
    assert(exprs.size() == 2);
    out << "(<= ";
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        out << " ";
        exprs[i]->print(out);
    }
    out << ") ";
}

void Addition::print(ostream& out) const {
    out << "(+";
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        out << " ";
        exprs[i]->print(out);
    }
    out << ") ";
}

void Subtraction::print(ostream& out) const {
    out << "(-";
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        out << " ";
        exprs[i]->print(out);
    }
    out << ") ";
}

void Multiplication::print(ostream& out) const {
    out << "(*";
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        out << " ";
        exprs[i]->print(out);
    }
    out << ") ";
}

void Division::print(ostream& out) const {
    out << "(/";
    for (unsigned int i = 0; i < exprs.size(); ++i) {
        out << " ";
        exprs[i]->print(out);
    }
    out << ") ";
}

/*****************************************************************
                          Unaries
*****************************************************************/

void Negation::print(ostream& out) const {
    out << "(not ";
    expr->print(out);
    out << ") ";
}

void ExponentialFunction::print(ostream& out) const {
    out << "(exp ";
    expr->print(out);
    out << ")";
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

void BernoulliDistribution::print(ostream& out) const {
    out << "Bernoulli(";
    expr->print(out);
    out << ") ";
}

void DiscreteDistribution::print(ostream& out) const {
    out << "Discrete( ";
    for (unsigned int i = 0; i < values.size(); ++i) {
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

void MultiConditionChecker::print(ostream& out) const {
    for (unsigned int i = 0; i < conditions.size(); ++i) {
        out << "case ";
        conditions[i]->print(out);
        out << " then ";
        effects[i]->print(out);
        out << endl;
    }
}
