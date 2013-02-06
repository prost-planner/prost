#ifndef CONDITIONAL_PROBABILITY_FUNCTIONS_H
#define CONDITIONAL_PROBABILITY_FUNCTIONS_H

#include "unprocessed_planning_task.h"
#include "logical_expressions.h"
#include "typed_objects.h"

#include "utils/math_utils.h"

#include <limits>
#include <map>

class ProstPlanner;
class UninstantiatedVariable;
class RDDLParser;
class Instantiator;
class PlanningTask;

class ConditionalProbabilityFunction {
public:
    ConditionalProbabilityFunction(ProstPlanner* _planner, PlanningTask* _task, StateFluent* _head, LogicalExpression* _body) :
        planner(_planner), 
        task(_task),
        head(_head),
        body(_body),
        probDomainSize(0),
        probHashKeyBase(-1),
        hashKeyBase(-1),
        initialized(false), 
        domainCalculated(false),
        stateFluentHashKeysCalculated(false),
        kleeneStateFluentHashKeysCalculated(false),
        isProb(false),
        isActionIndep(false),
        doesNotDependPositivelyOnActs(false),
        cachingEnabled(true),
        cacheInVector(true),
        kleeneCachingEnabled(true),
        kleeneCacheInVector(true),
        minVal(0.0), 
        maxVal(0.0),
        stateHashKey(0) {}

    ConditionalProbabilityFunction(ConditionalProbabilityFunction const& other) :  
        planner(other.planner), 
        task(other.task),
        head(other.head),
        body(other.body),
        dependentStateFluents(other.dependentStateFluents),
        dependentActionFluents(other.dependentActionFluents),
        probDomainMap(other.probDomainMap),
        probDomainSize(other.probDomainSize),
        probHashKeyBase(other.probHashKeyBase),
        hashKeyBase(other.hashKeyBase),
        index(other.index),
        initialized(other.initialized), 
        domainCalculated(other.domainCalculated),
        stateFluentHashKeysCalculated(other.stateFluentHashKeysCalculated),
        kleeneStateFluentHashKeysCalculated(other.kleeneStateFluentHashKeysCalculated),
        isProb(other.isProb),
        isActionIndep(other.isActionIndep),
        doesNotDependPositivelyOnActs(other.doesNotDependPositivelyOnActs),
        cachingEnabled(other.cachingEnabled),
        cacheInVector(other.cacheInVector),
        kleeneCachingEnabled(other.kleeneCachingEnabled),
        kleeneCacheInVector(other.kleeneCacheInVector),
        minVal(other.minVal), 
        maxVal(other.maxVal),
        stateHashKey(0),
        actionHashKeyMap(other.actionHashKeyMap),
        evaluationCacheVector(other.evaluationCacheVector),
        kleeneEvaluationCacheVector(other.kleeneEvaluationCacheVector) {}

    //TODO: This is very very ugly, but cpfs, planning tasks and
    //states are very tightly coupled. Nevertheless, there must be a
    //way to get rid of this, even if it takes some work!
    friend class PlanningTask;

    bool simplify(UnprocessedPlanningTask* _task, std::map<StateFluent*, NumericConstant*>& replacements);
    void initialize();
    void calculateDomain(std::vector<ActionState>& actions);
    void disableCaching();
    ConditionalProbabilityFunction* determinizeMostLikely(NumericConstant* randomNumberReplacement, PlanningTask* detPlanningTask, UnprocessedPlanningTask* _task);

    void initializeStateFluentHashKeys();
    void initializeKleeneStateFluentHashKeys();

    void evaluate(double& res, State const& current, State const& next, ActionState const& actions) {
        stateHashKey = current.stateFluentHashKey(index) + actionHashKeyMap[actions.index];
        assert((current.stateFluentHashKey(index) >= 0) && (actionHashKeyMap[actions.index] >= 0) && (stateHashKey >= 0));

        if(cacheInVector) {
            if(MathUtils::doubleIsMinusInfinity(evaluationCacheVector[stateHashKey])) {
                body->evaluate(res, current, next, actions);
                evaluationCacheVector[stateHashKey] = res;
            } else {
                res = evaluationCacheVector[stateHashKey];
            }
        } else {
            if(evaluationCacheMap.find(stateHashKey) != evaluationCacheMap.end()) {
                res = evaluationCacheMap[stateHashKey];
            } else {
                body->evaluate(res, current, next, actions);
                if(cachingEnabled) {
                    evaluationCacheMap[stateHashKey] = res;
                }
            }
        }
    }

    void evaluateToKleeneOutcome(double& res, State const& current, State const& next, ActionState const& actions) {
        assert((head->parent->valueType == BoolType::instance()) || (head == StateFluent::rewardInstance()));

        stateHashKey = current.stateFluentHashKey(index) + actionHashKeyMap[actions.index];
        assert((current.stateFluentHashKey(index) >= 0) && (actionHashKeyMap[actions.index] >= 0) && (stateHashKey >= 0));

        if(kleeneCacheInVector) {
            if(!MathUtils::doubleIsMinusInfinity(kleeneEvaluationCacheVector[stateHashKey])) {
                res = kleeneEvaluationCacheVector[stateHashKey];
            } else {
                body->evaluateToKleeneOutcome(res, current, next, actions);
                kleeneEvaluationCacheVector[stateHashKey] = res;
            }
        } else {
            if(kleeneEvaluationCacheMap.find(stateHashKey) != kleeneEvaluationCacheMap.end()) {
                res = kleeneEvaluationCacheMap[stateHashKey];
            } else {
                body->evaluateToKleeneOutcome(res, current, next, actions);
                if(kleeneCachingEnabled) {
                    kleeneEvaluationCacheMap[stateHashKey] = res;
                }
            }
        }
    }


    bool isProbabilistic() {
        return isProb;
    }

    bool isActionIndependent() {
        return isActionIndep;
    }

    bool doesNotDependPositivelyOnActions() {
        return doesNotDependPositivelyOnActs;
    }

    bool isNextStateIndependent() {
        //TODO: We currently simply assume that this CPF is next state
        //independent. Instead, we should: 
        //1. Check if this is the rewardCPF. If not, return true (CPFs
        //never depend on the next state).
        //2.Otherwise, actully check if
        //body->isNextStateIndependent(), which is not implemented atm
        return true;
    }

    bool isExogenous() {
        return (isProb && isActionIndep);
    }

    bool isRewardCPF() {
        assert(head != NULL);
        return (head == StateFluent::rewardInstance());
    }

    StateFluent* getHead() {
        assert(head != NULL);
        return head;
    }

    void print(std::ostream& out);

private:
    void calculateActionHashKey(ActionState const& action, long& nextKey);
    long getActionHashKey(std::vector<ActionFluent*>& scheduledActions);

    ProstPlanner* planner;
    PlanningTask* task;

    StateFluent* head;
    LogicalExpression* body;

    std::vector<StateFluent*> dependentStateFluents;
    std::vector<ActionFluent*> dependentActionFluents;

    std::vector<ActionFluent*> positiveActionDependencies;
    std::vector<ActionFluent*> negativeActionDependencies;

    //hashing of states as probability distributions
    std::map<double, int> probDomainMap;
    int probDomainSize;

    //hashing of states
    long probHashKeyBase;
    long hashKeyBase;

    //index, used for hashing (this is equal to stateSize+1 for reward CPF, and to the heads index otherwise)
    int index;

    bool initialized;
    bool domainCalculated;
    bool stateFluentHashKeysCalculated;
    bool kleeneStateFluentHashKeysCalculated;
    bool isProb;
    bool isActionIndep;
    bool doesNotDependPositivelyOnActs;
    bool cachingEnabled;
    bool cacheInVector;
    bool kleeneCachingEnabled;
    bool kleeneCacheInVector;
    double minVal;
    double maxVal;

    long stateHashKey;

    std::vector<long> actionHashKeyMap;

    std::map<long, double> evaluationCacheMap;
    std::vector<double> evaluationCacheVector;

    std::map<long, double> kleeneEvaluationCacheMap;
    std::vector<double> kleeneEvaluationCacheVector;
};

class ConditionalProbabilityFunctionDefinition {
public:
    static void parse(std::string& desc, UnprocessedPlanningTask* task, RDDLParser* parser);

    UninstantiatedVariable* head;
    LogicalExpression* body;

    ConditionalProbabilityFunction* instantiate(ProstPlanner* planner, UnprocessedPlanningTask* task, 
                                                PlanningTask* preprocessedPlanningTask, AtomicLogicalExpression* variable);
    void replaceQuantifier(UnprocessedPlanningTask* task, Instantiator* instantiator);

    void print(std::ostream& out);

private:
    ConditionalProbabilityFunctionDefinition(UninstantiatedVariable* _head, LogicalExpression* _body) :
        head(_head), body(_body) {}
};

#endif
