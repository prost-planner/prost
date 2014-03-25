#ifndef EVALUATABLES_H
#define EVALUATABLES_H

#include <cassert>

#include "logical_expressions.h"

class ConditionalProbabilityFunction;

struct Evaluatable {
    Evaluatable(std::string _name, LogicalExpression* _formula) :
        name(_name),
        formula(_formula),
        cachingVectorSize(0),
        isProb(false),
        hasArithmeticFunction(false) {}

    // Initialization
    void initialize();

    std::set<ActionFluent*> const& getPositiveDependentActionFluents() {
        return positiveActionDependencies;
    }

    std::set<ActionFluent*>const& getNegativeDependentActionFluents() {
        return negativeActionDependencies;
    }

    // A unique string that describes this (only used for print)
    std::string name;

    // The formula that is evaluatable
    LogicalExpression* formula;

    // All evaluatables have a hash index that is used to quckly update the
    // state fluent hash key of this evaluatable
    int hashIndex;

    // The caching type that will be used (initially) for this evaluatable and
    // the size of the vector if cachingType is VECTOR
    std::string cachingType;
    int cachingVectorSize;

    // The kleene caching type that will be used (initially) for this
    // evaluatable and the size of the vector if kleeneCachingType is VECTOR
    std::string kleeneCachingType;
    int kleeneCachingVectorSize;

    // Properties of this Evaluatable
    std::set<StateFluent*> dependentStateFluents;
    std::set<ActionFluent*> positiveActionDependencies;
    std::set<ActionFluent*> negativeActionDependencies;
    bool isProb;
    bool hasArithmeticFunction;

    // ActionHashKeyMap contains the hash keys of the actions that influence
    // this Evaluatable (these are added to the state fluent hash keys of a
    // state)
    std::vector<long> actionHashKeyMap;

    // These function are used to calculate the two parts of state fluent hash
    // keys: the action part (that is stored in the actionHashKeyMap of
    // Evaluatable), and the state fluent part (that is stored in PlanningTask
    // and computed within states).
    void initializeHashKeys(PlanningTask* task);
    long initializeActionHashKeys(std::vector<ActionState> const& actionStates);
    void calculateActionHashKey(std::vector<ActionState> const& actionStates, ActionState const& action, long& nextKey);
    long getActionHashKey(std::vector<ActionState> const& actionStates, std::vector<ActionFluent*>& scheduledActions);

    void initializeStateFluentHashKeys(std::vector<ConditionalProbabilityFunction*> const& CPFs,
                                       std::vector<std::vector<std::pair<int,long> > >& indexToStateFluentHashKeyMap,
                                       long const& firstStateFluentHashKeyBase);
    void initializeKleeneStateFluentHashKeys(std::vector<ConditionalProbabilityFunction*> const& CPFs,
                                             std::vector<std::vector<std::pair<int,long> > >& indexToKleeneStateFluentHashKeyMap,
                                             long const& firstStateFluentHashKeyBase);
    bool dependsOnActionFluent(ActionFluent* fluent) {
        return (positiveActionDependencies.find(fluent) != positiveActionDependencies.end() ||
                negativeActionDependencies.find(fluent) != negativeActionDependencies.end());
    }
};

struct ActionPrecondition : public Evaluatable {
    ActionPrecondition(std::string _name, LogicalExpression* _formula) :
        Evaluatable(_name , _formula) {}

    bool containsArithmeticFunction() const {
        return hasArithmeticFunction;
    }

    bool containsStateFluent() const {
        return !dependentStateFluents.empty();
    }

    bool containsActionFluent() const {
        return !(positiveActionDependencies.empty() && negativeActionDependencies.empty());
    }

    int index;
};

struct RewardFunction : public Evaluatable {
    RewardFunction(LogicalExpression* _formula) :
        Evaluatable("Reward" , _formula) {}

    bool isActionIndependent() const {
        return (positiveActionDependencies.empty() && negativeActionDependencies.empty());
    }

    double const& getMinVal() const {
        assert(!domain.empty());
        return *domain.begin();
    }

    double const& getMaxVal() const {
        assert(!domain.empty());
        return *domain.rbegin();
    }

    std::set<double> domain;
};

struct ConditionalProbabilityFunction : public Evaluatable {
    // This is used to sort transition functions by their name to ensure
    // deterministic behaviour
    struct TransitionFunctionSort {
        bool operator() (ConditionalProbabilityFunction* const& lhs, ConditionalProbabilityFunction* const& rhs) const {
            return lhs->name < rhs->name;
        }
    };

    ConditionalProbabilityFunction(StateFluent* _head, LogicalExpression* _formula) :
        Evaluatable(_head->fullName, _formula),
        head(_head),
        determinization(NULL),
        kleeneDomainSize(0) {}

    bool const& isProbabilistic() const {
        return isProb;
    }

    int getDomainSize() const {
        return domain.size();
    }

    bool hasFiniteDomain() const {
        return (!domain.empty());
    }

    void setIndex(int _index) {
        head->index = _index;
    }

    double getInitialValue() const {
        return head->initialValue;
    }

    StateFluent* head;

    // The determinized version of formula
    LogicalExpression* determinization;

    // The values this CPF can take
    std::set<double> domain;

    // Hashing of KleeneStates
    long kleeneDomainSize;
};


#endif
