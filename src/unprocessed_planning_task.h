#ifndef UNPROCESSED_PLANNING_TASK_H
#define UNPROCESSED_PLANNING_TASK_H

#include <string>
#include <vector>
#include <map>
#include <cassert>

class ObjectType;
class Object;
class VariableDefinition;
class ConditionalProbabilityFunctionDefinition;
class StateActionConstraint;
class UninstantiatedVariable;
class NumericConstant;
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

    void addCPFDefinition(ConditionalProbabilityFunctionDefinition* cpf);
    void getCPFDefinitions(std::vector<ConditionalProbabilityFunctionDefinition*>& result, bool includeRewardCPF = true);

    void addStateActionConstraint(StateActionConstraint* sac);
    void getStateActionConstraints(std::vector<StateActionConstraint*>& result);
    void removeStateActionConstraint(StateActionConstraint* sac);

    void addStateFluent(StateFluent* var);
    StateFluent* getStateFluent(UninstantiatedVariable* var);

    void addNonFluent(NonFluent* var);
    NonFluent* getNonFluent(UninstantiatedVariable* var);

    void addActionFluent(ActionFluent* var);
    ActionFluent* getActionFluent(UninstantiatedVariable* var);
    void getActionFluents(std::vector<ActionFluent*>& result);

    void getVariablesOfSchema(VariableDefinition* schema, std::vector<AtomicLogicalExpression*>& result);
    void getInitialState(std::vector<AtomicLogicalExpression*>& result);

    NumericConstant* getConstant(double val);
    NumericConstant* getConstant(UninstantiatedVariable* var);

    void addCPF(ConditionalProbabilityFunction* cpf);
    void getCPFs(std::vector<ConditionalProbabilityFunction*>& result, bool includeRewardCPF = true);
    void removeCPF(ConditionalProbabilityFunction* cpf);

    void print(std::ostream& out);

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

private:
    //objects
    std::map<std::string, ObjectType*> objectTypes;
    std::map<std::string, Object*> objects;
    std::map<ObjectType*,std::vector<Object*> > objectsByType;

    //schematic variables and cpfs
    std::map<std::string, VariableDefinition*> variableDefinitions;
    VariableDefinition* rewardVariableDefinition;
    std::vector<VariableDefinition*> variableDefinitionsVec;

    std::map<std::string, ConditionalProbabilityFunctionDefinition*> CPFDefs;
    std::vector<ConditionalProbabilityFunctionDefinition*> CPFDefsVec;
    ConditionalProbabilityFunctionDefinition* rewardCPFDef;

    //state action constraints
    std::vector<StateActionConstraint*> SACs;

    //variables
    std::map<std::string, StateFluent*> variables;
    std::map<std::string, ActionFluent*> actions;
    std::map<std::string, NonFluent*> nonFluents;
    StateFluent* rewardVariable;
    std::map<VariableDefinition*, std::vector<AtomicLogicalExpression*> > variablesBySchema;

    //constants
    std::map<double, NumericConstant*> constants;

    //instantiated CPFs
    std::map<StateFluent*, ConditionalProbabilityFunction*> CPFs;
    ConditionalProbabilityFunction* rewardCPF;
    std::vector<ConditionalProbabilityFunction*> CPFsVec;
};

#endif
