#ifndef EVALUATABLES_H
#define EVALUATABLES_H

#include "logical_expressions.h"

class UnprocessedPlanningTask;

class Evaluatable {
public:
    Evaluatable(std::string _name, LogicalExpression* _formula) :
        name(_name),
        formula(_formula),
        determinizedFormula(_formula),
        isProb(false),
        hasArithmeticFunction(false),
        hashIndex(-1),
        cachingType(NONE),
        kleeneCachingType(NONE) {}

    // Evaluates determinizedFormula (which is equivalent to formula if this
    // evaluatable is deterministic)
    void evaluate(double& res, State const& current, ActionState const& actions) {
        switch(cachingType) {
        case NONE:
            determinizedFormula->evaluate(res, current, actions);
            break;
        case MAP:
            stateHashKey = current.stateFluentHashKey(hashIndex) + actionHashKeyMap[actions.index];
            assert((current.stateFluentHashKey(hashIndex) >= 0) && (actionHashKeyMap[actions.index] >= 0) && (stateHashKey >= 0));

            if(evaluationCacheMap.find(stateHashKey) != evaluationCacheMap.end()) {
                res = evaluationCacheMap[stateHashKey];
            } else {
                determinizedFormula->evaluate(res, current, actions);
                evaluationCacheMap[stateHashKey] = res;
            }
            break;
        case DISABLED_MAP:
            stateHashKey = current.stateFluentHashKey(hashIndex) + actionHashKeyMap[actions.index];
            assert((current.stateFluentHashKey(hashIndex) >= 0) && (actionHashKeyMap[actions.index] >= 0) && (stateHashKey >= 0));

            if(evaluationCacheMap.find(stateHashKey) != evaluationCacheMap.end()) {
                res = evaluationCacheMap[stateHashKey];
            } else {
                determinizedFormula->evaluate(res, current, actions);
            }

            break;
        case VECTOR:
            stateHashKey = current.stateFluentHashKey(hashIndex) + actionHashKeyMap[actions.index];
            assert((current.stateFluentHashKey(hashIndex) >= 0) && (actionHashKeyMap[actions.index] >= 0) && (stateHashKey >= 0));
            assert(stateHashKey < evaluationCacheVector.size());

            if(MathUtils::doubleIsMinusInfinity(evaluationCacheVector[stateHashKey])) {
                determinizedFormula->evaluate(res, current, actions);
                evaluationCacheVector[stateHashKey] = res;
            } else {
                res = evaluationCacheVector[stateHashKey];
            }
            break;
        }
    }

    // Evaluates the probabilistic (original) formula. If this is deterministic,
    // evaluate is called and the result is transformed in a pd.
    void evaluateToPD(DiscretePD& res, State const& current, ActionState const& actions) {
        assert(res.isUndefined());

        if(isProbabilistic()) {
            switch(cachingType) {
            case NONE:
                formula->evaluateToPD(res, current, actions);
                break;
            case MAP:
                stateHashKey = current.stateFluentHashKey(hashIndex) + actionHashKeyMap[actions.index];
                assert((current.stateFluentHashKey(hashIndex) >= 0) && (actionHashKeyMap[actions.index] >= 0) && (stateHashKey >= 0));

                if(pdEvaluationCacheMap.find(stateHashKey) != pdEvaluationCacheMap.end()) {
                    res = pdEvaluationCacheMap[stateHashKey];
                } else {
                    formula->evaluateToPD(res, current, actions);
                    pdEvaluationCacheMap[stateHashKey] = res;
                }
                break;
            case DISABLED_MAP:
                stateHashKey = current.stateFluentHashKey(hashIndex) + actionHashKeyMap[actions.index];
                assert((current.stateFluentHashKey(hashIndex) >= 0) && (actionHashKeyMap[actions.index] >= 0) && (stateHashKey >= 0));

                if(pdEvaluationCacheMap.find(stateHashKey) != pdEvaluationCacheMap.end()) {
                    res = pdEvaluationCacheMap[stateHashKey];
                } else {
                    formula->evaluateToPD(res, current, actions);
                }
                break;
            case VECTOR:
                stateHashKey = current.stateFluentHashKey(hashIndex) + actionHashKeyMap[actions.index];
                assert((current.stateFluentHashKey(hashIndex) >= 0) && (actionHashKeyMap[actions.index] >= 0) && (stateHashKey >= 0));
                assert(stateHashKey < pdEvaluationCacheVector.size());

                if(pdEvaluationCacheVector[stateHashKey].isUndefined()) {
                    formula->evaluateToPD(res, current, actions);
                    pdEvaluationCacheVector[stateHashKey] = res;
                } else {
                    res = pdEvaluationCacheVector[stateHashKey];
                }
                break;
            }
        } else {
            double tmp;
            evaluate(tmp, current, actions);
            res.assignDiracDelta(tmp);
        }
    }

    // This function is called for state transitions with KleeneStates, which
    // works on the probabilistic (original) formula. TODO: it might be
    // necessary to distinguish between original and determinized formula
    void evaluateToKleene(std::set<double>& res, KleeneState const& current, ActionState const& actions) {
        assert(res.empty());
        switch(kleeneCachingType) {
        case NONE:
            formula->evaluateToKleene(res, current, actions);
            break;
        case MAP:
            stateHashKey = current.stateFluentHashKey(hashIndex) + actionHashKeyMap[actions.index];
            assert((current.stateFluentHashKey(hashIndex) >= 0) && (actionHashKeyMap[actions.index] >= 0) && (stateHashKey >= 0));

            if(kleeneEvaluationCacheMap.find(stateHashKey) != kleeneEvaluationCacheMap.end()) {
                res = kleeneEvaluationCacheMap[stateHashKey];
            } else {
                formula->evaluateToKleene(res, current, actions);
                kleeneEvaluationCacheMap[stateHashKey] = res;
            }
            break;
        case DISABLED_MAP:
            stateHashKey = current.stateFluentHashKey(hashIndex) + actionHashKeyMap[actions.index];
            assert((current.stateFluentHashKey(hashIndex) >= 0) && (actionHashKeyMap[actions.index] >= 0) && (stateHashKey >= 0));

            if(kleeneEvaluationCacheMap.find(stateHashKey) != kleeneEvaluationCacheMap.end()) {
                res = kleeneEvaluationCacheMap[stateHashKey];
            } else {
                formula->evaluateToKleene(res, current, actions);
            }

            break;
        case VECTOR:
            stateHashKey = current.stateFluentHashKey(hashIndex) + actionHashKeyMap[actions.index];
            assert((current.stateFluentHashKey(hashIndex) >= 0) && (actionHashKeyMap[actions.index] >= 0) && (stateHashKey >= 0));
            assert(stateHashKey < kleeneEvaluationCacheVector.size());

            if(kleeneEvaluationCacheVector[stateHashKey].empty()) {
                formula->evaluateToKleene(res, current, actions);
                kleeneEvaluationCacheVector[stateHashKey] = res;
            } else {
                res = kleeneEvaluationCacheVector[stateHashKey];
            }
            break;
        }
    }

    // Initialization
    void initialize();
    void initializeHashKeys(UnprocessedPlanningTask* task);

    // Disable caching
    void disableCaching();

    // Properties
    bool isProbabilistic() const {
        return isProb;
    }

    // SAC only
    bool containsArithmeticFunction() const {
        return hasArithmeticFunction;
    }

    // SAC only
    bool containsStateFluent() const {
        return !dependentStateFluents.empty();
    }

    // SAC only
    bool containsActionFluent() const {
        return !(positiveActionDependencies.empty() && negativeActionDependencies.empty());
    }

    // SAC & Reward
    std::set<ActionFluent*> const& getPositiveDependentActionFluents() {
        return positiveActionDependencies;
    }

    // SAC only
    std::set<ActionFluent*>const& getNegativeDependentActionFluents() {
        return negativeActionDependencies;
    }

    // Reward only (apart from print in PlanningTask)
    bool isActionIndependent() const {
        return (positiveActionDependencies.empty() && negativeActionDependencies.empty());
    }

    // A unique string that describes this (only used for print)
    std::string name;

    // The formula that is evaluatable
    LogicalExpression* formula;

    // If formula is probabilistic, this is a determinized version of it
    LogicalExpression* determinizedFormula;

    // Properties of this Evaluatable
    std::set<StateFluent*> dependentStateFluents;
    std::set<ActionFluent*> positiveActionDependencies;
    std::set<ActionFluent*> negativeActionDependencies;
    bool isProb;
    bool hasArithmeticFunction;

    // All evaluatables have a hash index that is used to quckly update the
    // state fluent hash key of this evaluatable
    int hashIndex;

    enum CachingType {
        NONE, // too many variables influence formula
        MAP, // many variables influence formula, but it's still possible with a map
        DISABLED_MAP, // as MAP, but after disableCaching() has been called
        VECTOR // only few variables influence formula, so we use a vector for caching
    };

    // CachingType describes which of the two (if any) datastructures is used to
    // cache computed values. If this Evaluatable is probabilistic, the
    // datastructures that start with 'pd' are used and the other ones if the
    // Evaluatable is deterministic
    CachingType cachingType;
    std::map<long, double> evaluationCacheMap;
    std::map<long, DiscretePD> pdEvaluationCacheMap;
    std::vector<double> evaluationCacheVector;
    std::vector<DiscretePD> pdEvaluationCacheVector;

    // KleeneCachingType describes which of the two (if any) datastructures is
    // used to cache computed values on Kleene states
    CachingType kleeneCachingType;
    std::map<long, std::set<double> > kleeneEvaluationCacheMap;
    std::vector<std::set<double> > kleeneEvaluationCacheVector;

    // ActionHashKeyMap contains the hash keys of the actions that influence
    // this Evaluatable (these are added to the state fluent hash keys of a
    // state)
    std::vector<long> actionHashKeyMap;

private:
    // These function are used to calculate the two parts of state fluent hash
    // keys: the action part (that is stored in the actionHashKeyMap of
    // Evaluatable), and the state fluent part (that is stored in PlanningTask
    // and computed within states).
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

    // This is a temporary variable used in evaluate. It is a member var as
    // initializing variables in case statements is only possible if additional
    // parentheses are used
    long stateHashKey;
};

class RewardFunction : public Evaluatable {
public:
    RewardFunction(LogicalExpression* _formula) :
        Evaluatable("Reward" , _formula) {}

    void setDomain(std::set<double> _domain) {
        domain = _domain;
    }

    double const& getMinVal() const {
        assert(!domain.empty());
        return *domain.begin();
    }

    double const& getMaxVal() const {
        assert(!domain.empty());
        return *domain.rbegin();
    }

private:
    std::set<double> domain;
};

class ConditionalProbabilityFunction : public Evaluatable {
public:
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
        kleeneDomainSize(0) {}

    StateFluent* getHead() const {
        assert(head);
        return head;
    }

    void setDomain(std::set<double> _domain) {
        domain = _domain;
    }

    int getDomainSize() const {
        return domain.size();
    }

    bool hasFiniteDomain() const {
        return (!domain.empty());
    }

    void setKleeneDomainSize(long _kleeneDomainSize) {
        kleeneDomainSize = _kleeneDomainSize;
    }

    long getKleeneDomainSize() const {
        return kleeneDomainSize;
    }

    bool hasFiniteKleeneDomain() const {
        return (kleeneDomainSize > 0);
    }

    void setIndex(int _index) {
        head->index = _index;
    }

    double getInitialValue() const {
        return head->initialValue;
    }

    StateFluent* head;

    // The values this CPF can take
    std::set<double> domain;

    // Hashing of KleeneStates
    long kleeneDomainSize;
};


#endif
