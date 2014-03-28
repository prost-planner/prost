#ifndef EVALUATABLES_H
#define EVALUATABLES_H

#include "logical_expressions.h"

class Evaluatable {
public:
    enum CachingType {
        NONE, // too many variables influence formula
        MAP, // many variables influence formula
        DISABLED_MAP, // as MAP, but after disableCaching() has been called
        VECTOR // only few variables influence formula, so we use a vector for caching
    };

    Evaluatable(std::string _name, int _hashIndex) :
        name(_name),
        formula(NULL),
        determinizedFormula(NULL),
        hashIndex(_hashIndex),
        cachingType(NONE),//_cachingType),
        kleeneCachingType(NONE) {}//_kleeneCachingType) {}

    void setFormula(std::vector<StateFluent*> const& stateFluents, std::vector<ActionFluent*> const& actionFluents, std::string& desc, bool const& isDeterminization = false);

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
    // evaluate is called and the result is transformed to a pd.
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

    // Properties
    bool isProbabilistic() const {
        return (formula != determinizedFormula);
    }

    // Disable caching
    void disableCaching();

    // A unique string that describes this (only used for print)
    std::string name;

    // The formula that is evaluatable
    LogicalExpression* formula;

    // If formula is probabilistic, this is a determinized version of it
    LogicalExpression* determinizedFormula;

    // All evaluatables have a hash index that is used to quckly update the
    // state fluent hash key of this evaluatable
    int hashIndex;

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

    // This is a temporary variable used in evaluate. It is a member var as
    // initializing variables in case statements is only possible if additional
    // parentheses are used
    long stateHashKey;
};

class RewardFunction : public Evaluatable {
public:
    RewardFunction(int _hashIndex, double _minVal, double _maxVal, bool _actionIndependent) :
        Evaluatable("Reward", _hashIndex),
        minVal(_minVal),
        maxVal(_maxVal),
        actionIndependent(_actionIndependent) {}

    double const& getMinVal() const {
        return minVal;
    }

    double const& getMaxVal() const {
        return maxVal;
    }

    bool const& isActionIndependent() const {
        return actionIndependent;
    }

    double minVal;
    double maxVal;
    bool actionIndependent;
};

class ConditionalProbabilityFunction : public Evaluatable {
public:
    ConditionalProbabilityFunction(int _hashIndex, StateFluent* _head) :
        Evaluatable(_head->name, _hashIndex),
        head(_head) {}

    int getDomainSize() const {
        return head->values.size();
    }

    // The state fluent that is updated by evaluating this
    StateFluent* head;
};


#endif
