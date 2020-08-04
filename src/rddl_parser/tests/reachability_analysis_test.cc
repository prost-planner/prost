#include "../../doctest/doctest.h"

#include "../evaluatables.h"
#include "../logical_expressions.h"
#include "../rddl.h"
#include "../reachability_analysis.h"

using namespace std;

namespace prost {
namespace parser {
namespace tests {
bool initStateValuesAreReachable(vector<set<double>> const& domains) {
    // All state values are 0 initially
    for (set<double> const& domain : domains)  {
        if(domain.find(0.0) == domain.end()) {
            return false;
        }
    }
    return true;
}

bool allValuesAreReachable(vector<set<double>> const& domains,
                           vector<StateFluent*> const& stateFluents) {
    for (StateFluent* sf : stateFluents) {
        set<double> const& domain = domains[sf->index];
        for (int val = 0; val < sf->domainSize; ++val) {
            if (domain.find(val) == domain.end()) {
                return false;
            }
        }
    }
    return true;
}

TEST_CASE("Reachability analysis where every fact is directly reachable") {
    RDDLTask* task = new RDDLTask();

    vector<Parameter*> params;
    string typeName = "fdr";
    Type* t = task->addType(typeName);
    task->addObject(typeName, "o0");
    task->addObject(typeName, "o1");
    task->addObject(typeName, "o2");
    auto fdrPVar = new ParametrizedVariable(
        "fdr", params, ParametrizedVariable::STATE_FLUENT, t, 0.0);
    task->addVariableSchematic(fdrPVar);
    auto pVar = new ParametrizedVariable(
        "pv", params, ParametrizedVariable::STATE_FLUENT,
        task->getType("bool"), 0.0);
    task->addVariableSchematic(pVar);

    auto s0 = new StateFluent(*fdrPVar, params, 0.0, 0);
    auto s1 = new StateFluent(*pVar, params, 0.0, 1);
    auto s2 = new StateFluent(*pVar, params, 0.0, 2);
    auto s3 = new StateFluent(*pVar, params, 0.0, 3);
    auto s4 = new StateFluent(*pVar, params, 0.0, 4);


    auto c0 = new ConditionalProbabilityFunction(s0, nullptr);
    c0->setDomain(2);
    auto c1 = new ConditionalProbabilityFunction(s1, nullptr);
    c1->setDomain(1);
    auto c2 = new ConditionalProbabilityFunction(s2, nullptr);
    c2->setDomain(1);
    auto c3 = new ConditionalProbabilityFunction(s3, nullptr);
    c3->setDomain(1);
    auto c4 = new ConditionalProbabilityFunction(s4, nullptr);
    c4->setDomain(1);

    auto a0 = new ActionFluent(*pVar, params, 0);
    auto a1 = new ActionFluent(*pVar, params, 1);
    task->actionFluents = {a0, a1};
    task->actionStates.push_back(ActionState({0,0}));

    SUBCASE("Reachability analysis where every fact is directly reachable") {
        task->horizon = 10;
        task->stateFluents = {s0, s1, s2, s3};
        task->CPFs = {c0, c1, c2, c3};
        vector<LogicalExpression*> values = {new NumericConstant(0.0),
                                             new NumericConstant(1.0),
                                             new NumericConstant(2.0)};
        vector<LogicalExpression*> probs = {new NumericConstant(0.3),
                                            new NumericConstant(0.3),
                                            new NumericConstant(0.4)};
        c0->formula = new DiscreteDistribution(values, probs);
        c1->formula = new BernoulliDistribution(new NumericConstant(0.5));
        c2->formula = new BernoulliDistribution(new NumericConstant(0.75));
        c3->formula = new BernoulliDistribution(new NumericConstant(0.25));

        MinkowskiReachabilityAnalyser r(task);
        vector<set<double>> domains = r.determineReachableFacts();
        CHECK(domains.size() == task->CPFs.size());
        CHECK(initStateValuesAreReachable(domains));
        CHECK(allValuesAreReachable(domains, task->stateFluents));
        CHECK(r.getNumberOfSimulationSteps() == 2);
    }

    SUBCASE("Reachability analysis where all facts are reachable after some iterations") {
        task->horizon = 10;
        task->stateFluents = {s0, s1, s2, s3, s4};
        task->CPFs = {c0, c1, c2, c3, c4};

        vector<LogicalExpression*> values = {new NumericConstant(0.0),
                                             new NumericConstant(1.0),
                                             new NumericConstant(2.0)};
        vector<LogicalExpression*> p1 = {s4, new NumericConstant(0.3)};
        vector<LogicalExpression*> p2 = {s4, new NumericConstant(0.4)};
        vector<LogicalExpression*> p3 =
            {new Multiplication(p2), new Negation(s4)};
        vector<LogicalExpression*> probs = {new Multiplication(p1),
                                            new Multiplication(p1),
                                            new Addition(p3)};
        c0->formula = new DiscreteDistribution(values, probs);
        c1->formula = new BernoulliDistribution(new NumericConstant(0.5));
        vector<LogicalExpression*> expr1 =
            {s1, new BernoulliDistribution(new NumericConstant(0.75))};
        c2->formula = new Conjunction(expr1);
        vector<LogicalExpression*> expr2 = {
            s2, new BernoulliDistribution(new NumericConstant(0.25))};
        c3->formula = new Conjunction(expr2);
        vector<LogicalExpression*> expr3 = {
            s3, new BernoulliDistribution(new NumericConstant(0.5))};
        c4->formula = new Conjunction(expr3);

        MinkowskiReachabilityAnalyser r(task);
        vector<set<double>> domains = r.determineReachableFacts();
        CHECK(domains.size() == task->CPFs.size());
        CHECK(initStateValuesAreReachable(domains));
        CHECK(allValuesAreReachable(domains, task->stateFluents));
        CHECK(r.getNumberOfSimulationSteps() == 6);
    }

    SUBCASE("Reachability analysis where facts are unreachable due to horizon") {
        task->horizon = 3;
        task->stateFluents = {s0, s1, s2, s3, s4};
        task->CPFs = {c0, c1, c2, c3, c4};

        vector<LogicalExpression*> values = {new NumericConstant(0.0),
                                             new NumericConstant(1.0),
                                             new NumericConstant(2.0)};
        vector<LogicalExpression*> p1 = {s4, new NumericConstant(0.3)};
        vector<LogicalExpression*> p2 = {s4, new NumericConstant(0.4)};
        vector<LogicalExpression*> p3 = {
            new Multiplication(p2), new Negation(s4)};
        vector<LogicalExpression*> probs = {new Multiplication(p1),
                                            new Multiplication(p1),
                                            new Addition(p3)};
        c0->formula = new DiscreteDistribution(values, probs);

        c1->formula = new BernoulliDistribution(new NumericConstant(0.5));
        vector<LogicalExpression*> expr1 = {
            s1, new BernoulliDistribution(new NumericConstant(0.75))};
        c2->formula = new Conjunction(expr1);
        vector<LogicalExpression*> expr2 = {
            s2, new BernoulliDistribution(new NumericConstant(0.25))};
        c3->formula = new Conjunction(expr2);
        vector<LogicalExpression*> expr3 = {
            s3, new BernoulliDistribution(new NumericConstant(0.5))};
        c4->formula = new Conjunction(expr3);

        MinkowskiReachabilityAnalyser r(task);
        vector<set<double>> domains = r.determineReachableFacts();
        CHECK(domains.size() == task->CPFs.size());
        CHECK(initStateValuesAreReachable(domains));
        CHECK(domains[0].size() == 2);
        CHECK(allValuesAreReachable(domains, {s1, s2, s3}));
        CHECK(domains[4].size() == 1);
        CHECK(r.getNumberOfSimulationSteps() == task->horizon);
    }
}
} // namespace tests
} // namespace parser
} // namespace prost
