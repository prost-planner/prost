#ifndef EVALUATABLES_H
#define EVALUATABLES_H

#include <cassert>

#include "logical_expressions.h"
#include "probability_distribution.h"

struct ConditionalProbabilityFunction;

struct Evaluatable {
    Evaluatable(std::string _name, LogicalExpression* _formula)
        : name(_name),
          formula(_formula),
          determinization(nullptr),
          isProb(false),
          hasArithmeticFunction(false) {}

    // Initialization
    virtual void initialize();

    // Simplification
    virtual void simplify(Simplifications& replace);

    bool isActionIndependent() const {
        return dependentActionFluents.empty();
    }

    bool const& isProbabilistic() const {
        return isProb;
    }

    // A unique string that describes this (only used for print)
    std::string name;

    // The formula that is evaluatable
    LogicalExpression* formula;

    // The determinized version of formula
    // except probabilistic CPFs)
    LogicalExpression* determinization;

    // All evaluatables have a hash index that is used to quckly update the
    // state fluent hash key of this evaluatable
    int hashIndex;

    // The caching type that will be used (initially) for this evaluatable
    std::string cachingType;

    // If the caching type is VECTOR, this contains all possible results of
    // evaluating this (the first of the determinization, and if this is
    // probabilistic the second of formula)
    std::vector<double> precomputedResults;
    std::vector<DiscretePD> precomputedPDResults;

    // The kleene caching type that will be used (initially) for this
    // evaluatable and the size of the vector if kleeneCachingType is VECTOR
    std::string kleeneCachingType;
    int kleeneCachingVectorSize;

    // Properties of this Evaluatable
    std::set<StateFluent*> dependentStateFluents;
    std::set<ActionFluent*> dependentActionFluents;
    bool isProb;
    bool hasArithmeticFunction;

    // The actionHashKeyMap contains the hash keys of the actions that influence
    // this Evaluatable (these are added to the state fluent hash keys of a
    // state)
    std::vector<long> actionHashKeyMap;

    // The stateFluentHashKeyMap contains the state fluent hash key (base) of
    // each of the dependent state fluents
    std::vector<std::pair<int, long>> stateFluentHashKeyBases;

    // These function are used to calculate the two parts of state fluent hash
    // keys: the action part (that is stored in the actionHashKeyMap of
    // Evaluatable), and the state fluent part (that is stored in RDDLTask
    // and computed within states).
    void initializeHashKeys(RDDLTask* task);
    long initializeActionHashKeys(RDDLTask* task);
    long getActionHashKey(
        std::vector<ActionState> const& actionStates, int actionIndex);

    void initializeStateFluentHashKeys(RDDLTask* task, long nextHashKeyBase);
    void initializeKleeneStateFluentHashKeys(
        RDDLTask* task, long nextHashKeyBase);
};

struct ActionPrecondition : public Evaluatable {
    ActionPrecondition(std::string _name, LogicalExpression* _formula)
        : Evaluatable(_name, _formula) {}

    bool containsStateFluent() const {
        return !dependentStateFluents.empty();
    }

    // Checks if this is a static action constraint of the form "not a" for some
    // action fluent "a"
    int triviallyForbidsActionFluent() const;

    int index;
};

struct RewardFunction : public Evaluatable {
    RewardFunction(LogicalExpression* _formula)
        : Evaluatable("Reward", _formula) {}

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
    ConditionalProbabilityFunction(StateFluent* _head,
                                   LogicalExpression* _formula)
        : Evaluatable(_head->fullName, _formula),
          head(_head),
          kleeneDomainSize(0) {}

    int getDomainSize() const {
        return domain.size();
    }

    bool hasFiniteDomain() const {
        return !domain.empty();
    }

    void setDomain(int numVals);

    void setIndex(int _index) {
        head->index = _index;
    }

    double getInitialValue() const {
        return head->initialValue;
    }

    StateFluent* head;

    // The values this CPF can take
    std::vector<int> domain;

    // Hashing of KleeneStates
    long kleeneDomainSize;
};

#endif
