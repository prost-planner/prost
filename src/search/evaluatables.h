#ifndef EVALUATABLES_H
#define EVALUATABLES_H

#include "logical_expressions.h"

#include <unordered_map>

class Evaluatable {
public:
    enum CachingType {
        NONE,         // too many variables influence formula
        MAP,          // many variables influence formula
        DISABLED_MAP, // as MAP, but after disableCaching() has been called
        VECTOR // only few variables influence formula, so we use a vector for
               // caching
    };

    // This function is called for state transitions with KleeneStates. The
    // result of the evaluation is a set of values, i.e., a subset of the domain
    // of this Evaluatable
    void evaluateToKleene(std::set<double>& res, KleeneState const& current,
                          ActionState const& actions) {
        assert(res.empty());
        switch (kleeneCachingType) {
        case NONE:
            formula->evaluateToKleene(res, current, actions);
            break;
        case MAP:
            stateHashKey = current.stateFluentHashKey(hashIndex) +
                           actionHashKeyMap[actions.index];
            assert((current.stateFluentHashKey(hashIndex) >= 0) &&
                   (actionHashKeyMap[actions.index] >= 0) &&
                   (stateHashKey >= 0));

            if (kleeneEvaluationCacheMap.find(stateHashKey) !=
                kleeneEvaluationCacheMap.end()) {
                res = kleeneEvaluationCacheMap[stateHashKey];
            } else {
                formula->evaluateToKleene(res, current, actions);
                kleeneEvaluationCacheMap[stateHashKey] = res;
            }
            break;
        case DISABLED_MAP:
            stateHashKey = current.stateFluentHashKey(hashIndex) +
                           actionHashKeyMap[actions.index];
            assert((current.stateFluentHashKey(hashIndex) >= 0) &&
                   (actionHashKeyMap[actions.index] >= 0) &&
                   (stateHashKey >= 0));

            if (kleeneEvaluationCacheMap.find(stateHashKey) !=
                kleeneEvaluationCacheMap.end()) {
                res = kleeneEvaluationCacheMap[stateHashKey];
            } else {
                formula->evaluateToKleene(res, current, actions);
            }

            break;
        case VECTOR:
            stateHashKey = current.stateFluentHashKey(hashIndex) +
                           actionHashKeyMap[actions.index];
            assert((current.stateFluentHashKey(hashIndex) >= 0) &&
                   (actionHashKeyMap[actions.index] >= 0) &&
                   (stateHashKey >= 0));
            assert(stateHashKey < kleeneEvaluationCacheVector.size());

            if (kleeneEvaluationCacheVector[stateHashKey].empty()) {
                formula->evaluateToKleene(res, current, actions);
                kleeneEvaluationCacheVector[stateHashKey] = res;
            } else {
                res = kleeneEvaluationCacheVector[stateHashKey];
            }
            break;
        }
    }

    // Properties
    virtual bool isProbabilistic() const = 0;

    // Disable caching
    void disableCaching();

    // This only matters for CPFs (where it is overwritten)
    virtual int getDomainSize() const {
        return 0;
    }

    // A unique string that describes this (only used for print)
    std::string name;

    // The formula that is evaluatable
    LogicalExpression* formula;

    // All evaluatables have a hash index that is used to quickly update the
    // state fluent hash key of this evaluatable
    int hashIndex;

    // CachingType describes which datastructure is used to cache computed
    // values (the datastructures are defined in DeterministicEvaluatable and
    // ProbabilisticEvaluatable).
    CachingType cachingType;

    // KleeneCachingType describes which of the two (if any) datastructures is
    // used to cache computed values on Kleene states
    CachingType kleeneCachingType;
    std::unordered_map<long, std::set<double>> kleeneEvaluationCacheMap;
    std::vector<std::set<double>> kleeneEvaluationCacheVector;

    // ActionHashKeyMap contains the hash keys of the actions that influence
    // this Evaluatable (these are added to the state fluent hash keys of a
    // state)
    std::vector<long> actionHashKeyMap;

    // This is a temporary variable used in evaluate. It is a member var as
    // initializing variables in case statements is only possible if additional
    // parentheses are used
    long stateHashKey;

protected:
    Evaluatable(std::string _name, int _hashIndex)
        : name(_name),
          formula(nullptr),
          hashIndex(_hashIndex),
          cachingType(NONE),
          kleeneCachingType(NONE) {}

    Evaluatable(std::string _name, LogicalExpression* _formula, int _hashIndex)
        : name(_name),
          formula(_formula),
          hashIndex(_hashIndex),
          cachingType(NONE),
          kleeneCachingType(NONE) {}
};

class DeterministicEvaluatable : public Evaluatable {
public:
    DeterministicEvaluatable(std::string _name, LogicalExpression* _formula,
                             int _hashIndex)
        : Evaluatable(_name, _formula, _hashIndex) {}

    DeterministicEvaluatable(std::string _name, int _hashIndex)
        : Evaluatable(_name, _hashIndex) {}

    // Evaluates the formula (deterministically) to a double
    void evaluate(double& res, State const& current,
                  ActionState const& actions) {
        switch (cachingType) {
        case NONE:
            formula->evaluate(res, current, actions);
            break;
        case MAP:
            stateHashKey = current.stateFluentHashKey(hashIndex) +
                           actionHashKeyMap[actions.index];
            assert((current.stateFluentHashKey(hashIndex) >= 0) &&
                   (actionHashKeyMap[actions.index] >= 0) &&
                   (stateHashKey >= 0));

            if (evaluationCacheMap.find(stateHashKey) !=
                evaluationCacheMap.end()) {
                res = evaluationCacheMap[stateHashKey];
            } else {
                formula->evaluate(res, current, actions);
                evaluationCacheMap[stateHashKey] = res;
            }
            break;
        case DISABLED_MAP:
            stateHashKey = current.stateFluentHashKey(hashIndex) +
                           actionHashKeyMap[actions.index];
            assert((current.stateFluentHashKey(hashIndex) >= 0) &&
                   (actionHashKeyMap[actions.index] >= 0) &&
                   (stateHashKey >= 0));

            if (evaluationCacheMap.find(stateHashKey) !=
                evaluationCacheMap.end()) {
                res = evaluationCacheMap[stateHashKey];
            } else {
                formula->evaluate(res, current, actions);
            }

            break;
        case VECTOR:
            stateHashKey = current.stateFluentHashKey(hashIndex) +
                           actionHashKeyMap[actions.index];

            assert((current.stateFluentHashKey(hashIndex) >= 0) &&
                   (actionHashKeyMap[actions.index] >= 0) &&
                   (stateHashKey >= 0));
            assert(stateHashKey < evaluationCacheVector.size());
            assert(!MathUtils::doubleIsMinusInfinity(
                evaluationCacheVector[stateHashKey]));

            res = evaluationCacheVector[stateHashKey];
            break;
        }
    }

    bool isProbabilistic() const {
        return false;
    }

    std::unordered_map<long, double> evaluationCacheMap;
    std::vector<double> evaluationCacheVector;
};

class ProbabilisticEvaluatable : public Evaluatable {
public:
    ProbabilisticEvaluatable(std::string _name, int _hashIndex)
        : Evaluatable(_name, _hashIndex) {}

    // Evaluates the formula to a discrete probability distribution
    void evaluate(DiscretePD& res, State const& current,
                  ActionState const& actions) {
        assert(res.isUndefined());

        switch (cachingType) {
        case NONE:
            formula->evaluateToPD(res, current, actions);
            break;
        case MAP:
            stateHashKey = current.stateFluentHashKey(hashIndex) +
                           actionHashKeyMap[actions.index];
            assert((current.stateFluentHashKey(hashIndex) >= 0) &&
                   (actionHashKeyMap[actions.index] >= 0) &&
                   (stateHashKey >= 0));

            if (evaluationCacheMap.find(stateHashKey) !=
                evaluationCacheMap.end()) {
                res = evaluationCacheMap[stateHashKey];
            } else {
                formula->evaluateToPD(res, current, actions);
                evaluationCacheMap[stateHashKey] = res;
            }
            break;
        case DISABLED_MAP:
            stateHashKey = current.stateFluentHashKey(hashIndex) +
                           actionHashKeyMap[actions.index];
            assert((current.stateFluentHashKey(hashIndex) >= 0) &&
                   (actionHashKeyMap[actions.index] >= 0) &&
                   (stateHashKey >= 0));

            if (evaluationCacheMap.find(stateHashKey) !=
                evaluationCacheMap.end()) {
                res = evaluationCacheMap[stateHashKey];
            } else {
                formula->evaluateToPD(res, current, actions);
            }
            break;
        case VECTOR:
            stateHashKey = current.stateFluentHashKey(hashIndex) +
                           actionHashKeyMap[actions.index];

            assert((current.stateFluentHashKey(hashIndex) >= 0) &&
                   (actionHashKeyMap[actions.index] >= 0) &&
                   (stateHashKey >= 0));
            assert(stateHashKey < evaluationCacheVector.size());
            assert(!evaluationCacheVector[stateHashKey].isUndefined());

            res = evaluationCacheVector[stateHashKey];
            break;
        }
    }

    bool isProbabilistic() const {
        return true;
    }

    std::unordered_map<long, DiscretePD> evaluationCacheMap;
    std::vector<DiscretePD> evaluationCacheVector;
};

class RewardFunction : public DeterministicEvaluatable {
public:
    RewardFunction(LogicalExpression* _formula, int _hashIndex, double _minVal,
                   double _maxVal, bool _actionIndependent)
        : DeterministicEvaluatable("Reward", _formula, _hashIndex),
          minVal(_minVal),
          maxVal(_maxVal),
          actionIndependent(_actionIndependent) {}

    double const& getMinVal() const {
        return minVal;
    }

    double const& getMaxVal() const {
        return maxVal;
    }

    bool isActionIndependent() const {
        return actionIndependent;
    }

private:
    double minVal;
    double maxVal;
    bool actionIndependent;
};

class DeterministicCPF : public DeterministicEvaluatable {
public:
    DeterministicCPF(int _hashIndex, StateFluent* _head)
        : DeterministicEvaluatable(_head->name, _hashIndex), head(_head) {}

    int getDomainSize() const {
        return head->values.size();
    }

    // The state fluent that is updated by evaluating this
    StateFluent* head;
};

class ProbabilisticCPF : public ProbabilisticEvaluatable {
public:
    ProbabilisticCPF(int _hashIndex, StateFluent* _head)
        : ProbabilisticEvaluatable(_head->name, _hashIndex), head(_head) {}

    int getDomainSize() const {
        return head->values.size();
    }

    // The state fluent that is updated by evaluating this
    StateFluent* head;
};

#endif
