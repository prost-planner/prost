#ifndef UNPROCESSED_PLANNING_TASK_H
#define UNPROCESSED_PLANNING_TASK_H

#include <string>
#include <vector>
#include <map>
#include <cassert>

class ObjectType;
class Object;
class VariableDefinition;
class LogicalExpression;
class UninstantiatedVariable;
class AtomicLogicalExpression;
class NonFluent;
class StateFluent;
class ActionFluent;
class ConditionalProbabilityFunction;

class UnprocessedPlanningTask {
public:
    UnprocessedPlanningTask(std::string _domainDesc, std::string _problemDesc);

    void preprocessInput(std::string& problemDesc);

    void addObjectType(ObjectType* objectType);
    ObjectType* getObjectType(std::string& name);

    void addObject(Object* object);
    Object* getObject(std::string& name);
    void getObjectsOfType(ObjectType* objectType, std::vector<Object*>& res);

    void addVariableDefinition(VariableDefinition* varDef);
    VariableDefinition* getVariableDefinition(std::string& name);
    void getVariableDefinitions(std::vector<VariableDefinition*>& result, bool includeRewardDef = false);
    bool isAVariableDefinition(std::string& name);

    void addCPFDefinition(std::pair<UninstantiatedVariable*, LogicalExpression*> const& CPFDef);

    void addStateActionConstraint(LogicalExpression* sac);
    void removeStateActionConstraint(LogicalExpression* sac);

    void addStateFluent(StateFluent* var);
    StateFluent* getStateFluent(UninstantiatedVariable* var);

    void addNonFluent(NonFluent* var);
    NonFluent* getNonFluent(UninstantiatedVariable* var);

    void addActionFluent(ActionFluent* var);
    ActionFluent* getActionFluent(UninstantiatedVariable* var);
    void getActionFluents(std::vector<ActionFluent*>& result);

    void getVariablesOfSchema(VariableDefinition* schema, std::vector<AtomicLogicalExpression*>& result);
    void getInitialState(std::vector<AtomicLogicalExpression*>& result);

    void addCPF(std::pair<StateFluent*, LogicalExpression*> const& cpf);
    void setRewardCPF(LogicalExpression* const& _rewardCPF);

    //string description
    std::string domainDesc;
    std::string nonFluentsDesc;
    std::string instanceDesc;

    //general info
    std::string domainName;
    std::string nonFluentsName;
    std::string instanceName;

    //requirements
    std::map<std::string, bool> requirements;

    //numbers
    int numberOfConcurrentActions;
    int horizon;
    double discountFactor;

    //objects
    std::map<std::string, ObjectType*> objectTypes;
    std::map<std::string, Object*> objects;
    std::map<ObjectType*,std::vector<Object*> > objectsByType;

    //schematic variables and cpfs
    std::map<std::string, VariableDefinition*> variableDefinitions;
    VariableDefinition* rewardVariableDefinition;
    std::vector<VariableDefinition*> variableDefinitionsVec;

    std::vector<std::pair<UninstantiatedVariable*, LogicalExpression*> > CPFDefs;

    //state action constraints
    std::vector<LogicalExpression*> SACs;

    //variables
    std::map<std::string, StateFluent*> variables;
    std::map<std::string, ActionFluent*> actions;
    std::map<std::string, NonFluent*> nonFluents;
    StateFluent* rewardVariable;
    std::map<VariableDefinition*, std::vector<AtomicLogicalExpression*> > variablesBySchema;

    //instantiated CPFs
    LogicalExpression* rewardCPF;
    std::vector<std::pair<StateFluent*, LogicalExpression*> > CPFs;
};

#endif
