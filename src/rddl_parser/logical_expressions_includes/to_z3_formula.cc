z3::expr LogicalExpression::toZ3Formula(
    RDDLTaskCSP& csp, int /*actionIndex*/) const {
    utils::abort("toZ3Formula of expression not impemented!");
    // Just to make compiler happy
    return csp.createConstant("0");
}

/*****************************************************************
                           Atomics
*****************************************************************/

z3::expr StateFluent::toZ3Formula(RDDLTaskCSP& csp, int /*actionIndex*/) const {
    return csp.getStateVarSet()[index];
}

z3::expr ActionFluent::toZ3Formula(RDDLTaskCSP& csp, int actionIndex) const {
    return csp.getActionVarSet(actionIndex)[index];
}

z3::expr NumericConstant::toZ3Formula(
    RDDLTaskCSP& csp, int /*actionIndex*/) const {
    return csp.createConstant(to_string(value).c_str());
}

z3::expr Conjunction::toZ3Formula(RDDLTaskCSP& csp, int actionIndex) const {
    z3::expr res = (exprs[0]->toZ3Formula(csp, actionIndex) != 0);
    for (size_t i = 1; i < exprs.size(); ++i) {
        res = res && (exprs[i]->toZ3Formula(csp, actionIndex) != 0);
    }
    return z3::ite(res, csp.createConstant("1"), csp.createConstant("0"));
}

z3::expr Disjunction::toZ3Formula(RDDLTaskCSP& csp, int actionIndex) const {
    z3::expr res = (exprs[0]->toZ3Formula(csp, actionIndex) != 0);
    for (size_t i = 1; i < exprs.size(); ++i) {
        res = res || (exprs[i]->toZ3Formula(csp, actionIndex) != 0);
    }
    return z3::ite(res, csp.createConstant("1"), csp.createConstant("0"));
}


z3::expr EqualsExpression::toZ3Formula(
    RDDLTaskCSP& csp, int actionIndex) const {
    assert(exprs.size() == 2);
    z3::expr res =
        exprs[0]->toZ3Formula(csp, actionIndex) ==
        exprs[1]->toZ3Formula(csp, actionIndex);
    return z3::ite(res, csp.createConstant("1"), csp.createConstant("0"));
}

z3::expr GreaterExpression::toZ3Formula(
    RDDLTaskCSP& csp, int actionIndex) const {
    assert(exprs.size() == 2);
    z3::expr res =
            exprs[0]->toZ3Formula(csp, actionIndex) >
            exprs[1]->toZ3Formula(csp, actionIndex);
    return z3::ite(res, csp.createConstant("1"), csp.createConstant("0"));
}

z3::expr LowerExpression::toZ3Formula(RDDLTaskCSP& csp, int actionIndex) const {
    assert(exprs.size() == 2);
    z3::expr res =
            exprs[0]->toZ3Formula(csp, actionIndex) <
            exprs[1]->toZ3Formula(csp, actionIndex);
    return z3::ite(res, csp.createConstant("1"), csp.createConstant("0"));
}

z3::expr GreaterEqualsExpression::toZ3Formula(
    RDDLTaskCSP& csp, int actionIndex) const {
    assert(exprs.size() == 2);
    z3::expr res =
            exprs[0]->toZ3Formula(csp, actionIndex) >=
            exprs[1]->toZ3Formula(csp, actionIndex);
    return z3::ite(res, csp.createConstant("1"), csp.createConstant("0"));
}

z3::expr LowerEqualsExpression::toZ3Formula(
    RDDLTaskCSP& csp, int actionIndex) const {
    assert(exprs.size() == 2);
    z3::expr res =
            exprs[0]->toZ3Formula(csp, actionIndex) <=
            exprs[1]->toZ3Formula(csp, actionIndex);
    return z3::ite(res, csp.createConstant("1"), csp.createConstant("0"));
}

z3::expr Addition::toZ3Formula(RDDLTaskCSP& csp, int actionIndex) const {
    z3::expr res = exprs[0]->toZ3Formula(csp, actionIndex);
    for (size_t i = 1; i < exprs.size(); ++i) {
        res = res + exprs[i]->toZ3Formula(csp, actionIndex);
    }
    return res;
}

z3::expr Subtraction::toZ3Formula(RDDLTaskCSP& csp, int actionIndex) const {
    z3::expr res = exprs[0]->toZ3Formula(csp, actionIndex);
    for (size_t i = 1; i < exprs.size(); ++i) {
        res = res - exprs[i]->toZ3Formula(csp, actionIndex);
    }
    return res;
}

z3::expr Multiplication::toZ3Formula(RDDLTaskCSP& csp, int actionIndex) const {
    z3::expr res = exprs[0]->toZ3Formula(csp, actionIndex);
    for (size_t i = 1; i < exprs.size(); ++i) {
        res = res * exprs[i]->toZ3Formula(csp, actionIndex);
    }
    return res;
}

z3::expr Division::toZ3Formula(RDDLTaskCSP& csp, int actionIndex) const {
    z3::expr res = exprs[0]->toZ3Formula(csp, actionIndex);
    for (size_t i = 1; i < exprs.size(); ++i) {
        res = res / exprs[i]->toZ3Formula(csp, actionIndex);
    }
    return res;
}

z3::expr Negation::toZ3Formula(RDDLTaskCSP& csp, int actionIndex) const {
    z3::expr res = (expr->toZ3Formula(csp, actionIndex) != 0);
    return z3::ite(res, csp.createConstant("0"), csp.createConstant("1"));
}

z3::expr MultiConditionChecker::toZ3Formula(
    RDDLTaskCSP& csp, int actionIndex) const {
    return buildZ3Formula(csp, actionIndex, 0);
}

z3::expr MultiConditionChecker::buildZ3Formula(
    RDDLTaskCSP& csp, int actionIndex, int index) const {
    if (index == conditions.size() - 1) {
        return effects[index]->toZ3Formula(csp, actionIndex);
    }
    z3::expr ifCond = (conditions[index]->toZ3Formula(csp, actionIndex) != 0);
    z3::expr thenEff = effects[index]->toZ3Formula(csp, actionIndex);
    z3::expr elseEff = buildZ3Formula(csp, actionIndex, index + 1);
    return z3::ite(ifCond, thenEff, elseEff);
}