#include "../../doctest/doctest.h"

#include "../evaluatables.h"
#include "../logical_expressions.h"
#include "../rddl.h"

#include "../determinize/determinize.h"
#include "../utils/math_utils.h"

using namespace std;

namespace prost::parser::determinize {
TEST_CASE("Determinization with binary variables") {
    auto task = new RDDLTask();
    vector<Parameter*> params;
    auto pVar = new ParametrizedVariable(
        "pv", params, ParametrizedVariable::STATE_FLUENT,
        task->getType("bool"), 0.0);
    task->addVariableSchematic(pVar);

    auto s0 = new StateFluent(*pVar, params, 0.0, 0);
    auto s1 = new StateFluent(*pVar, params, 0.0, 1);
    auto s2 = new StateFluent(*pVar, params, 0.0, 2);

    auto c0 = new ConditionalProbabilityFunction(s0, nullptr);
    c0->setDomainSize(2);
    auto c1 = new ConditionalProbabilityFunction(s1, nullptr);
    c1->setDomainSize(2);
    auto c2 = new ConditionalProbabilityFunction(s2, nullptr);
    c2->setDomainSize(2);

    SUBCASE("Static Bernoulli distribution") {
        // The determinization of a constant expression c is "1" (i.e., true) if
        // c >= 0.5, and "0" (i.e., false) otherwise
        task->stateFluents = {s0, s1, s2};
        task->CPFs = {c0, c1, c2};
        c0->formula = new BernoulliDistribution(new NumericConstant(0.3));
        c0->initialize();
        c1->formula = new BernoulliDistribution(new NumericConstant(0.5));
        c1->initialize();
        c2->formula = new BernoulliDistribution(new NumericConstant(0.7));
        c2->initialize();

        MostLikelyDeterminizer det(task);
        det.determinize();

        auto nc0 = dynamic_cast<NumericConstant*>(c0->determinization);
        auto nc1 = dynamic_cast<NumericConstant*>(c1->determinization);
        auto nc2 = dynamic_cast<NumericConstant*>(c2->determinization);
            CHECK(nc0);
            CHECK(nc1);
            CHECK(nc2);
            CHECK(utils::MathUtils::doubleIsEqual(nc0->value, 0.0));
            CHECK(utils::MathUtils::doubleIsEqual(nc1->value, 1.0));
            CHECK(utils::MathUtils::doubleIsEqual(nc2->value, 1.0));
    }

    SUBCASE("Bernoulli distribution that depends on variable") {
        task->stateFluents = {s0};
        task->CPFs = {c0};
        vector<LogicalExpression*> mult = {s0, new NumericConstant(0.4)};
        c0->formula = new BernoulliDistribution(new Multiplication(mult));
        c0->initialize();

        MostLikelyDeterminizer det(task);
        det.determinize();

        // The result of the determinization of the formula "s0 * 0.4" is the
        // expression "0.5 <= s0 * 0.4"
        auto leq = dynamic_cast<LowerEqualsExpression*>(c0->determinization);
            CHECK(leq);
        auto nc1 = dynamic_cast<NumericConstant*>(leq->exprs[0]);
            CHECK(nc1);
            CHECK(utils::MathUtils::doubleIsEqual(nc1->value, 0.5));
        auto mpl = dynamic_cast<Multiplication*>(leq->exprs[1]);
            CHECK(mpl);
        auto var = dynamic_cast<StateFluent*>(mpl->exprs[0]);
            CHECK(var);
            CHECK(var == s0);
        auto nc2 = dynamic_cast<NumericConstant*>(mpl->exprs[1]);
            CHECK(nc2);
            CHECK(utils::MathUtils::doubleIsEqual(nc2->value, 0.4));
    }
}

TEST_CASE("Determinization with FDR variables") {
    auto task = new RDDLTask();
    vector<Parameter*> params;
    string typeName = "fdr";
    Type* t = task->addType(typeName);
    task->addObject(typeName, "o0");
    task->addObject(typeName, "o1");
    task->addObject(typeName, "o2");
    auto fdrPVar = new ParametrizedVariable(
        "fdr", params, ParametrizedVariable::STATE_FLUENT, t, 0.0);
    task->addVariableSchematic(fdrPVar);

    auto s0 = new StateFluent(*fdrPVar, params, 0.0, 0);
    auto c0 = new ConditionalProbabilityFunction(s0, nullptr);
    c0->setDomainSize(3);
    SUBCASE("Static Discrete distribution with unique max") {
        task->stateFluents = {s0};
        task->CPFs = {c0};

        vector<LogicalExpression*> values = {new NumericConstant(0.0),
                                             new NumericConstant(1.0),
                                             new NumericConstant(2.0)};
        vector<LogicalExpression*> probs = {new NumericConstant(0.2),
                                            new NumericConstant(0.6),
                                            new NumericConstant(0.2)};
        c0->formula = new DiscreteDistribution(values, probs);
        c0->initialize();

        MostLikelyDeterminizer det(task);
        det.determinize();

        // The probability for value "1" is the highest
        auto nc = dynamic_cast<NumericConstant*>(c0->determinization);
        CHECK(nc);
        CHECK(utils::MathUtils::doubleIsEqual(nc->value, 1.0));
    }

    SUBCASE("Static Discrete distribution with non-unique max") {
        task->stateFluents = {s0};
        task->CPFs = {c0};

        vector<LogicalExpression*> values = {new NumericConstant(0.0),
                                             new NumericConstant(1.0),
                                             new NumericConstant(2.0)};
        vector<LogicalExpression*> probs = {new NumericConstant(0.4),
                                            new NumericConstant(0.2),
                                            new NumericConstant(0.4)};
        c0->formula = new DiscreteDistribution(values, probs);
        c0->initialize();

        MostLikelyDeterminizer det(task);
        det.determinize();

        // The probability for values "0" and "2" are equal, so the one that
        // occurs first in the values vector is selected
        auto nc = dynamic_cast<NumericConstant*>(c0->determinization);
        CHECK(nc);
        CHECK(utils::MathUtils::doubleIsEqual(nc->value, 0.0));
    }

    SUBCASE("Discrete distribution with state-dependent probability") {
        // If the probabilities depend on the state, the discrete distribution
        // is determinized into a MultiConditionChecker.
        task->stateFluents = {s0};
        task->CPFs = {c0};

        vector<LogicalExpression*> values = {new NumericConstant(0.0),
                                             new NumericConstant(1.0),
                                             new NumericConstant(2.0)};
        vector<LogicalExpression*> p1 = {new NumericConstant(0.3), s0};
        auto m1 = new Multiplication(p1);
        vector<LogicalExpression*> p2 = {new NumericConstant(1), m1};
        auto s1 = new Subtraction(p2);
        vector<LogicalExpression*> probs = {m1, s1, new NumericConstant(0)};
        c0->formula = new DiscreteDistribution(values, probs);
        c0->initialize();

        MostLikelyDeterminizer det(task);
        det.determinize();

        // The probability for values "0" and "2" are equal, so the one that
        // occurs first in the values vector is selected
        auto mcc = dynamic_cast<MultiConditionChecker*>(c0->determinization);
        CHECK(mcc);
        CHECK(mcc->effects.size() == 3);
        auto nc1 = dynamic_cast<NumericConstant*>(mcc->effects[0]);
        auto nc2 = dynamic_cast<NumericConstant*>(mcc->effects[1]);
        auto nc3 = dynamic_cast<NumericConstant*>(mcc->effects[2]);
        CHECK(nc1);
        CHECK(nc2);
        CHECK(nc3);
        CHECK(utils::MathUtils::doubleIsEqual(nc1->value, 0.0));
        CHECK(utils::MathUtils::doubleIsEqual(nc2->value, 1.0));
        CHECK(utils::MathUtils::doubleIsEqual(nc3->value, 2.0));
        CHECK(mcc->conditions.size() == 3);
        auto conj1 = dynamic_cast<Conjunction*>(mcc->conditions[0]);
        auto conj2 = dynamic_cast<Conjunction*>(mcc->conditions[1]);
        auto conj3 = dynamic_cast<Conjunction*>(mcc->conditions[2]);
        CHECK(conj1);
        CHECK(conj2);
        CHECK(conj3);
        CHECK(conj1->exprs.size() == 2);
        CHECK(conj2->exprs.size() == 2);
        CHECK(conj3->exprs.size() == 2);
        auto geq0 = dynamic_cast<GreaterEqualsExpression*>(conj1->exprs[0]);
        auto geq1 = dynamic_cast<GreaterEqualsExpression*>(conj1->exprs[1]);
        CHECK(geq0);
        CHECK(geq1);
    }
}
} // namespace prost::parser::determinize
