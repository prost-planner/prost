#ifndef CONDITIONAL_PROBABILITY_FUNCTION_H
#define CONDITIONAL_PROBABILITY_FUNCTION_H

#include "evaluatable.h"
#include "unprocessed_planning_task.h"
#include "typed_objects.h"

#include "utils/math_utils.h"

#include <map>

class UninstantiatedVariable;

class ConditionalProbabilityFunction : public Evaluatable {
public:
    //TODO: This is very very ugly, but cpfs, planning tasks and
    //states are very tightly coupled. Nevertheless, there must be a
    //way to get rid of this, even if it takes some work!
    friend class PlanningTask;

    // This is used to sort transition functions by their name to ensure
    // deterministic behaviour
    struct TransitionFunctionSort {
        bool operator() (ConditionalProbabilityFunction* const& lhs, ConditionalProbabilityFunction* const& rhs) const {
            return lhs->head->name < rhs->head->name;
        }
    };

    ConditionalProbabilityFunction(StateFluent* _head, LogicalExpression* _formula) :
        Evaluatable("CPF " + _head->name, _formula),
        head(_head),
        kleeneDomainSize(0) {}

    ConditionalProbabilityFunction(ConditionalProbabilityFunction const& other, LogicalExpression* _formula) :
        Evaluatable(other, _formula),
        head(other.head),
        domain(other.domain),
        kleeneDomainSize(other.kleeneDomainSize) {}

    ConditionalProbabilityFunction* determinizeMostLikely(NumericConstant* randomNumberReplacement);

    void calculateDomain(std::vector<std::set<double> > const& domains, ActionState const& actionState, std::set<double>& res) {
        formula->calculateDomain(domains, actionState, res);
    }

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

    void setDomain(std::set<double> _domain) {
        domain = _domain;
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

    void setIndex(int _index) {
        head->index = _index;
    }

    double getInitialValue() const {
        return head->initialValue;
    }

    void setKleeneDomainSize(long _kleeneDomainSize) {
        kleeneDomainSize = _kleeneDomainSize;
    }

private:
    StateFluent* head;

    // The values this CPF can take
    std::set<double> domain;

    // Hashing of KleeneStates
    long kleeneDomainSize;
};

#endif
