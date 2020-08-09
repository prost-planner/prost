#include "../../doctest/doctest.h"

#include "../evaluatables.h"
#include "../logical_expressions.h"
#include "../rddl.h"

#include "../determinize/determinize.h"
#include "../utils/math_utils.h"

using namespace std;

namespace prost {
namespace parser {
namespace determinize {
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
    SUBCASE("Static Discrete distribution") {
        task->stateFluents = {s0};
        task->CPFs = {c0};

        vector<LogicalExpression*> conditions = {new NumericConstant(0.2),
                                                 new NumericConstant(0.6),
                                                 new NumericConstant(1.0)};
        vector<LogicalExpression*> effects = {new NumericConstant(0.0),
                                              new NumericConstant(1.0),
                                              new NumericConstant(2.0)};
        c0->formula = new DiscreteDistribution(conditions, effects);
        c0->initialize();

        MostLikelyDeterminizer det(task);
        det.determinize();

        // The probability for the first effect (0.0) is 0.2; the probability
        // for the second effect (1.0) is 0.8 * 0.6 = 0.48; and the probability
        // for the third effect is 0.32. Therefore, the result of the
        // determinization must be "1.0"
        auto nc = dynamic_cast<NumericConstant*>(c0->determinization);
        CHECK(nc);
        CHECK(utils::MathUtils::doubleIsEqual(nc->value, 1.0));
    }
}
} // namespace determinize
} // namespace parser
} // namespace prost
