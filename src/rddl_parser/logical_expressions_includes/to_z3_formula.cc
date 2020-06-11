z3::expr LogicalExpression::toZ3Formula(
        z3::context& c, Z3Expressions const& /*sf_exprs*/,
        Z3Expressions const& /*af_exprs*/) const {
    SystemUtils::abort("toZ3Formula not impemented!");
    // Just to make compiler happy
    return c.real_val("0");
}

/*****************************************************************
                           Atomics
*****************************************************************/

z3::expr StateFluent::toZ3Formula(
        z3::context& /*c*/, Z3Expressions const& sf_exprs,
        Z3Expressions const& /*af_exprs*/) const {
    return sf_exprs[index];
}

z3::expr ActionFluent::toZ3Formula(
        z3::context& /*c*/, Z3Expressions const& /*sf_exprs*/,
        Z3Expressions const& af_exprs) const {
    return af_exprs[index];
}

z3::expr NumericConstant::toZ3Formula(
        z3::context& c, Z3Expressions const& /*sf_exprs*/,
        Z3Expressions const& /*af_exprs*/) const {
    return c.real_val(to_string(value).c_str());
}

z3::expr Conjunction::toZ3Formula(
        z3::context& c, Z3Expressions const& sf_exprs,
        Z3Expressions const& af_exprs) const {
    z3::expr res = exprs[0]->toZ3Formula(c, sf_exprs, af_exprs);
    for (size_t i = 1; i < exprs.size(); ++i) {
        res = res && exprs[i]->toZ3Formula(c, sf_exprs, af_exprs);
    }
    return z3::ite(res, c.real_val("1"), c.real_val("0"));
}

z3::expr Disjunction::toZ3Formula(
        z3::context& c, Z3Expressions const& sf_exprs,
        Z3Expressions const& af_exprs) const {
    z3::expr res = exprs[0]->toZ3Formula(c, sf_exprs, af_exprs);
    for (size_t i = 1; i < exprs.size(); ++i) {
        res = res || exprs[i]->toZ3Formula(c, sf_exprs, af_exprs);
    }
    return z3::ite(res, c.real_val("1"), c.real_val("0"));
}


z3::expr EqualsExpression::toZ3Formula(
        z3::context& c, Z3Expressions const& sf_exprs,
        Z3Expressions const& af_exprs) const {
    assert(exprs.size() == 2);
    z3::expr res =
        exprs[0]->toZ3Formula(c, sf_exprs, af_exprs) ==
        exprs[1]->toZ3Formula(c, sf_exprs, af_exprs);
    return z3::ite(res, c.real_val("1"), c.real_val("0"));
}

z3::expr GreaterExpression::toZ3Formula(
        z3::context& c, Z3Expressions const& sf_exprs,
        Z3Expressions const& af_exprs) const {
    assert(exprs.size() == 2);
    z3::expr res =
            exprs[0]->toZ3Formula(c, sf_exprs, af_exprs) >
            exprs[1]->toZ3Formula(c, sf_exprs, af_exprs);
    return z3::ite(res, c.real_val("1"), c.real_val("0"));
}

z3::expr LowerExpression::toZ3Formula(
        z3::context& c, Z3Expressions const& sf_exprs,
        Z3Expressions const& af_exprs) const {
    assert(exprs.size() == 2);
    z3::expr res =
            exprs[0]->toZ3Formula(c, sf_exprs, af_exprs) <
            exprs[1]->toZ3Formula(c, sf_exprs, af_exprs);
    return z3::ite(res, c.real_val("1"), c.real_val("0"));
}

z3::expr GreaterEqualsExpression::toZ3Formula(
        z3::context& c, Z3Expressions const& sf_exprs,
        Z3Expressions const& af_exprs) const {
    assert(exprs.size() == 2);
    z3::expr res =
            exprs[0]->toZ3Formula(c, sf_exprs, af_exprs) >=
            exprs[1]->toZ3Formula(c, sf_exprs, af_exprs);
    return z3::ite(res, c.real_val("1"), c.real_val("0"));
}

z3::expr LowerEqualsExpression::toZ3Formula(
        z3::context& c, Z3Expressions const& sf_exprs,
        Z3Expressions const& af_exprs) const {
    assert(exprs.size() == 2);
    z3::expr res =
            exprs[0]->toZ3Formula(c, sf_exprs, af_exprs) <=
            exprs[1]->toZ3Formula(c, sf_exprs, af_exprs);
    return z3::ite(res, c.real_val("1"), c.real_val("0"));
}

z3::expr Addition::toZ3Formula(
        z3::context& c, Z3Expressions const& sf_exprs,
        Z3Expressions const& af_exprs) const {
    z3::expr res = exprs[0]->toZ3Formula(c, sf_exprs, af_exprs);
    for (size_t i = 1; i < exprs.size(); ++i) {
        res = res + exprs[i]->toZ3Formula(c, sf_exprs, af_exprs);
    }
    return res;
}

z3::expr Subtraction::toZ3Formula(
        z3::context& c, Z3Expressions const& sf_exprs,
        Z3Expressions const& af_exprs) const {
    z3::expr res = exprs[0]->toZ3Formula(c, sf_exprs, af_exprs);
    for (size_t i = 1; i < exprs.size(); ++i) {
        res = res - exprs[i]->toZ3Formula(c, sf_exprs, af_exprs);
    }
    return res;
}

z3::expr Multiplication::toZ3Formula(
        z3::context& c, Z3Expressions const& sf_exprs,
        Z3Expressions const& af_exprs) const {
    z3::expr res = exprs[0]->toZ3Formula(c, sf_exprs, af_exprs);
    for (size_t i = 1; i < exprs.size(); ++i) {
        res = res * exprs[i]->toZ3Formula(c, sf_exprs, af_exprs);
    }
    return res;
}

z3::expr Division::toZ3Formula(
        z3::context& c, Z3Expressions const& sf_exprs,
        Z3Expressions const& af_exprs) const {
    z3::expr res = exprs[0]->toZ3Formula(c, sf_exprs, af_exprs);
    for (size_t i = 1; i < exprs.size(); ++i) {
        res = res / exprs[i]->toZ3Formula(c, sf_exprs, af_exprs);
    }
    return res;
}

z3::expr Negation::toZ3Formula(
        z3::context& c, Z3Expressions const& sf_exprs,
        Z3Expressions const& af_exprs) const {
    z3::expr res = !expr->toZ3Formula(c, sf_exprs, af_exprs);
    return z3::ite(res, c.real_val("1"), c.real_val("0"));
}