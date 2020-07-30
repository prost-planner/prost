#include "../../doctest/doctest.h"

#include "../evaluatables.h"
#include "../logical_expressions.h"
#include "../rddl.h"
#include "../reachability_analysis.h"

using namespace std;

RDDLTask* createTask(int numStateVars, int numActionVars) {
    RDDLTask* task = new RDDLTask();
    task->horizon = 10;
    vector<Parameter*> params;
    auto pVar = new ParametrizedVariable("pv", params,
                                         ParametrizedVariable::STATE_FLUENT,
                                         task->getType("bool"), 0.0);
    task->addVariableSchematic(pVar);
    for (int i = 0; i < numStateVars; ++i) {
        auto sf = new StateFluent(*pVar, params, 0.0, i);
        auto cpf = new ConditionalProbabilityFunction(sf, nullptr);
        cpf->setDomain(1);
        task->stateFluents.push_back(sf);
        task->CPFs.push_back(cpf);
    }
    task->actionStates.push_back(ActionState(vector<int>(numActionVars, 0)));

    for (int i = 0; i < numActionVars; ++i) {
        task->actionFluents.push_back(new ActionFluent(*pVar, params, i));
    }
    return task;
}

void addFDRStateFluent(RDDLTask* task, int numVals = 3) {
    static int counter = 0;
    string typeName = "fdr" + to_string(counter);
    Type* t = task->addType(typeName);
    for (int i = 0; i < numVals; ++i) {
        string objectName = "o" + to_string(counter) + "-" + to_string(i);
        task->addObject(typeName, objectName);
    }
    vector<Parameter*> params;
    auto pVar = new ParametrizedVariable("fdr", params,
                                         ParametrizedVariable::STATE_FLUENT,
                                         t, 0.0);
    auto sf = new StateFluent(*pVar, params, 0.0, task->stateFluents.size());
    auto cpf = new ConditionalProbabilityFunction(sf, nullptr);
    cpf->setDomain(numVals-1);
    task->stateFluents.push_back(sf);
    task->CPFs.push_back(cpf);
}

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
        if (domain.size() != sf->domainSize) {
            cout << sf->fullName << endl;
            return false;
        }
        for (int val = 0; val < sf->domainSize; ++val) {
            if (domain.find(val) == domain.end()) {
                return false;
            }
        }
    }
    return true;
}

TEST_CASE("Reachability analysis where every fact is directly reachable") {
    RDDLTask* task = createTask(3, 2);
    addFDRStateFluent(task);
    task->CPFs[0]->formula =
        new BernoulliDistribution(new NumericConstant(0.5));
    task->CPFs[1]->formula =
        new BernoulliDistribution(new NumericConstant(0.75));
    task->CPFs[2]->formula =
        new BernoulliDistribution(new NumericConstant(0.25));
    vector<LogicalExpression*> values = {new NumericConstant(0.0),
                                         new NumericConstant(1.0),
                                         new NumericConstant(2.0)};
    vector<LogicalExpression*> probs = {new NumericConstant(0.3),
                                        new NumericConstant(0.3),
                                        new NumericConstant(0.4)};
    task->CPFs[3]->formula = new DiscreteDistribution(values, probs);

    MinkowskiReachabilityAnalyser r(task);
    vector<set<double>> domains = r.determineReachableFacts();
    CHECK(domains.size() == task->CPFs.size());
    CHECK(initStateValuesAreReachable(domains));
    CHECK(allValuesAreReachable(domains, task->stateFluents));
    CHECK(r.getNumberOfSimulationSteps() == 2);
}

TEST_CASE("Reachability analysis where all facts are reachable after some iterations") {
    RDDLTask* task = createTask(4, 2);
    addFDRStateFluent(task);
    task->CPFs[0]->formula =
        new BernoulliDistribution(new NumericConstant(0.5));
    vector<LogicalExpression*> expr1 = {task->stateFluents[0],
                                       new BernoulliDistribution(new NumericConstant(0.75))};
    task->CPFs[1]->formula = new Conjunction(expr1);
    vector<LogicalExpression*> expr2 = {task->stateFluents[1],
                                       new BernoulliDistribution(new NumericConstant(0.25))};
    task->CPFs[2]->formula = new Conjunction(expr2);
    vector<LogicalExpression*> expr3 = {task->stateFluents[2],
                                        new BernoulliDistribution(new NumericConstant(0.5))};
    task->CPFs[3]->formula = new Conjunction(expr3);
    vector<LogicalExpression*> values = {new NumericConstant(0.0),
                                         new NumericConstant(1.0),
                                         new NumericConstant(2.0)};
    vector<LogicalExpression*> p1 = {task->stateFluents[3],
                                     new NumericConstant(0.3)};
    vector<LogicalExpression*> p2 = {task->stateFluents[3],
                                     new NumericConstant(0.4)};
    vector<LogicalExpression*> p3 = {new Multiplication(p2),
                                     new Negation(task->stateFluents[3])};
    vector<LogicalExpression*> probs = {new Multiplication(p1),
                                        new Multiplication(p1),
                                        new Addition(p3)};
    task->CPFs[4]->formula = new DiscreteDistribution(values, probs);


    MinkowskiReachabilityAnalyser r(task);
    vector<set<double>> domains = r.determineReachableFacts();
    CHECK(domains.size() == task->CPFs.size());
    CHECK(initStateValuesAreReachable(domains));
    CHECK(allValuesAreReachable(domains, task->stateFluents));
    CHECK(r.getNumberOfSimulationSteps() == 6);
}

TEST_CASE("Reachability analysis where facts are unreachable due to horizon") {
    RDDLTask* task = createTask(4, 2);
    addFDRStateFluent(task);
    task->horizon = 3;
    task->CPFs[0]->formula =
        new BernoulliDistribution(new NumericConstant(0.5));
    vector<LogicalExpression*> expr1 = {task->stateFluents[0],
                                        new BernoulliDistribution(new NumericConstant(0.75))};
    task->CPFs[1]->formula = new Conjunction(expr1);
    vector<LogicalExpression*> expr2 = {task->stateFluents[1],
                                        new BernoulliDistribution(new NumericConstant(0.25))};
    task->CPFs[2]->formula = new Conjunction(expr2);
    vector<LogicalExpression*> expr3 = {task->stateFluents[2],
                                        new BernoulliDistribution(new NumericConstant(0.5))};
    task->CPFs[3]->formula = new Conjunction(expr3);
    vector<LogicalExpression*> values = {new NumericConstant(0.0),
                                         new NumericConstant(1.0),
                                         new NumericConstant(2.0)};
    vector<LogicalExpression*> p1 = {task->stateFluents[3],
                                     new NumericConstant(0.3)};
    vector<LogicalExpression*> p2 = {task->stateFluents[3],
                                     new NumericConstant(0.4)};
    vector<LogicalExpression*> p3 = {new Multiplication(p2),
                                     new Negation(task->stateFluents[3])};
    vector<LogicalExpression*> probs = {new Multiplication(p1),
                                        new Multiplication(p1),
                                        new Addition(p3)};
    task->CPFs[4]->formula = new DiscreteDistribution(values, probs);

    MinkowskiReachabilityAnalyser r(task);
    vector<set<double>> domains = r.determineReachableFacts();
    CHECK(domains.size() == task->CPFs.size());
    CHECK(initStateValuesAreReachable(domains));
    vector<StateFluent*>& sfs = task->stateFluents;
    CHECK(allValuesAreReachable(domains, {sfs[0], sfs[1], sfs[2]}));
    CHECK(domains[3].size() == 1);
    CHECK(domains[4].size() == 2);
    CHECK(r.getNumberOfSimulationSteps() == task->horizon);
}