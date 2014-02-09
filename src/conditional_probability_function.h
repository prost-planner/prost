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
        kleeneDomainSize(0) {}

    ConditionalProbabilityFunction(ConditionalProbabilityFunction const& other, LogicalExpression* _formula) :
        Evaluatable(other, _formula),
        head(other.head),
        domain(other.domain),
        kleeneDomainSize(other.kleeneDomainSize) {}

    bool simplify(UnprocessedPlanningTask* _task, std::map<StateFluent*, NumericConstant*>& replacements);
    ConditionalProbabilityFunction* determinizeMostLikely(NumericConstant* randomNumberReplacement, UnprocessedPlanningTask* _task);

    void initializeDomains(std::vector<ActionState> const& actionStates);

    bool isRewardCPF() const {
        assert(head != NULL);
        return (head == StateFluent::rewardInstance());
    }

    StateFluent* getHead() const {
        assert(head != NULL);
        return head;
    }

    bool isBoolean() const {
        return head->parent->valueType->type == Type::BOOL;
    }

    std::set<double> const& getDomain() const {
        return domain;
    }

    bool hasFiniteDomain() const {
        return (!domain.empty());
    }

    double const& getMinVal() const {
        assert(!domain.empty());
        return *domain.begin();
    }

    double const& getMaxVal() const {
        assert(!domain.empty());
        return *domain.rbegin();
    }

    bool hasFiniteKleeneDomain() const {
        return (kleeneDomainSize > 0);
    }

    long getKleeneDomainSize() const {
        return kleeneDomainSize;
    }

private:
    StateFluent* head;

    // The values this CPF can take
    std::set<double> domain;

    // Hashing of KleeneStates
    long kleeneDomainSize;
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
