void LogicalExpression::print(ostream& /*out*/) {
    assert(false);
}

/*****************************************************************
                         Schematics
*****************************************************************/

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

void ActionFluent::print(ostream& out) {
    out << "$a(" << index << ")";
}

void StateFluent::print(ostream& out) {
    out << "$s(" << index << ")";
}

/*****************************************************************
                           Atomics
*****************************************************************/

void NumericConstant::print(ostream& out) {
    out << "$c(" << value << ")";
}

void Object::print(ostream& out) {
    if(types.size() != 1) {
        SystemUtils::abort("Implement object fluents in print.cc");
    }
    assert(values[0] < types[0]->objects.size());
    assert(name == types[0]->objects[values[0]]->name);
    out << name << "(" << values[0] << ") ";
}

/*****************************************************************
                           Connectives
*****************************************************************/

void Connective::print(ostream& out) {
    for(unsigned int i = 0; i < exprs.size(); ++i) {
        exprs[i]->print(out);
        if(i != exprs.size()-1) {
            out << " ";
        }
    }
}

void Conjunction::print(ostream& out) {
    out << "and(";
    Connective::print(out);
    out << ")";
}

void Disjunction::print(ostream& out) {
    out << "or(";
    Connective::print(out);
    out << ")";
}

void EqualsExpression::print(ostream& out) {
    out << "==(";
    Connective::print(out);
    out << ")";
}

void GreaterExpression::print(ostream& out) {
    assert(exprs.size() == 2);
    out << ">(";
    Connective::print(out);
    out << ")";
}

void LowerExpression::print(ostream& out) {
    assert(exprs.size() == 2);
    out << "<(";
    Connective::print(out);
    out << ")";
}

void GreaterEqualsExpression::print(ostream& out) {
    assert(exprs.size() == 2);
    out << ">=(";
    Connective::print(out);
    out << ")";
}

void LowerEqualsExpression::print(ostream& out) {
    assert(exprs.size() == 2);
    out << "<=(";
    Connective::print(out);
    out << ")";
}

void Addition::print(ostream& out) {
    out << "+(";
    Connective::print(out);
    out << ")";
}

void Subtraction::print(ostream& out) {
    out << "-(";
    Connective::print(out);
    out << ")";
}

void Multiplication::print(ostream& out) {
    out << "*(";
    Connective::print(out);
    out << ")";
}

void Division::print(ostream& out) {
    out << "/(";
    Connective::print(out);
    out << ")";
}

/*****************************************************************
                          Unaries
*****************************************************************/

void Negation::print(ostream& out) {
    out << "~(";
    expr->print(out);
    out << ")";
}

/*****************************************************************
                   Probability Distributions
*****************************************************************/

void BernoulliDistribution::print(ostream& out) {
    out << "Bernoulli(";
    expr->print(out);
    out << ")";
}

void DiscreteDistribution::print(ostream& out) {
    out << "Discrete(";
    for(unsigned int i = 0; i < values.size(); ++i) {
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

void IfThenElseExpression::print(ostream& out) {
    out << "if(";
    condition->print(out);
    out << ") then(";
    valueIfTrue->print(out);
    out << ") elif($c(1)) then(";
    valueIfFalse->print(out);
    out << ")";
}

void MultiConditionChecker::print(ostream& out) {
    for(unsigned int i = 0; i < conditions.size(); ++i) {
        if(i == 0) {
            out << "if(";
        } else {
            out << " elif(";
        }
        conditions[i]->print(out);
        out << ") then(";
        effects[i]->print(out);
        out << ")";
    }
}
