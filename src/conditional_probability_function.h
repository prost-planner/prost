#ifndef CONDITIONAL_PROBABILITY_FUNCTION_H
#define CONDITIONAL_PROBABILITY_FUNCTION_H

#include "evaluatable.h"
#include "unprocessed_planning_task.h"
#include "typed_objects.h"

#include "utils/math_utils.h"

#include <limits>
#include <map>

class UninstantiatedVariable;
class RDDLParser;
class Instantiator;
class PlanningTask;

class ConditionalProbabilityFunction : public Evaluatable {
public:
    //TODO: This is very very ugly, but cpfs, planning tasks and
    //states are very tightly coupled. Nevertheless, there must be a
    //way to get rid of this, even if it takes some work!
    friend class PlanningTask;

    ConditionalProbabilityFunction(StateFluent* _head, LogicalExpression* _formula) :
        Evaluatable("CPF " + _head->name, _formula),
        head(_head),
        probDomainSize(0),
        probHashKeyBase(0),
        hashKeyBase(0),
        minVal(0.0), 
        maxVal(0.0) {}

    ConditionalProbabilityFunction(ConditionalProbabilityFunction const& other, LogicalExpression* _formula) :
        Evaluatable(other, _formula),
        head(other.head),
        probDomainMap(other.probDomainMap),
        probDomainSize(other.probDomainSize),
        probHashKeyBase(other.probHashKeyBase),
        hashKeyBase(other.hashKeyBase),
        minVal(other.minVal), 
        maxVal(other.maxVal),
        domainSize(0) {}

    void initialize();
    bool simplify(UnprocessedPlanningTask* _task, std::map<StateFluent*, NumericConstant*>& replacements);
    ConditionalProbabilityFunction* determinizeMostLikely(NumericConstant* randomNumberReplacement, UnprocessedPlanningTask* _task);

    bool isRewardCPF() const {
        assert(head != NULL);
        return (head == StateFluent::rewardInstance());
    }

    StateFluent* getHead() const {
        assert(head != NULL);
        return head;
    }

    double const& getMinVal() const {
        return minVal;
    }

    double const& getMaxVal() const {
        return maxVal;
    }

    unsigned int const& getDomainSize() const {
        return domainSize;
    }

private:
    StateFluent* head;

    //hashing of states as probability distributions
    std::map<double, long> probDomainMap;
    int probDomainSize;

    //hashing of states
    long probHashKeyBase;
    long hashKeyBase;

    double minVal;
    double maxVal;

    unsigned int domainSize;
};

class ConditionalProbabilityFunctionDefinition {
public:
    static void parse(std::string& desc, UnprocessedPlanningTask* task, RDDLParser* parser);

    UninstantiatedVariable* head;
    LogicalExpression* formula;

    ConditionalProbabilityFunction* instantiate(UnprocessedPlanningTask* task, AtomicLogicalExpression* variable);
    void replaceQuantifier(UnprocessedPlanningTask* task, Instantiator* instantiator);

    void print(std::ostream& out);

private:
    ConditionalProbabilityFunctionDefinition(UninstantiatedVariable* _head, LogicalExpression* _formula) :
        head(_head), formula(_formula) {}
};

#endif
