#ifndef UNPROCESSED_PLANNING_TASK_H
#define UNPROCESSED_PLANNING_TASK_H

#include <string>
#include <vector>
#include <map>
#include <cassert>

class Type;
class Parameter;
class Object;
class ParametrizedVariable;
class LogicalExpression;
class NonFluent;
class StateFluent;
class ActionFluent;

struct UnprocessedPlanningTask {
    UnprocessedPlanningTask(std::string _domainDesc, std::string _problemDesc);

    void preprocessInput(std::string& problemDesc);

    void addType(std::string const& name, std::string const& superType = "");
    void addObject(std::string const& typeName, std::string const& objectName);

    void addVariableDefinition(ParametrizedVariable* varDef);

    void addParametrizedVariable(ParametrizedVariable* parent, std::vector<Parameter*> const& params);
    void addParametrizedVariable(ParametrizedVariable* parent, std::vector<Parameter*> const& params, double initialValue);

    StateFluent* getStateFluent(std::string const& name);
    ActionFluent* getActionFluent(std::string const& name);
    NonFluent* getNonFluent(std::string const& name);

    std::vector<StateFluent*> getVariablesOfSchema(ParametrizedVariable* schema);

    void addStateActionConstraint(LogicalExpression* sac);
    void addCPF(std::pair<StateFluent*, LogicalExpression*> const& cpf);
    void setRewardCPF(LogicalExpression* const& _rewardCPF);

    // Task description as string
    std::string domainDesc;
    std::string nonFluentsDesc;
    std::string instanceDesc;

    // Names
    std::string domainName;
    std::string nonFluentsName;
    std::string instanceName;

    // Simple numeric properties
    int numberOfConcurrentActions;
    int horizon;
    double discountFactor;

    // Object types
    std::map<std::string, Type*> types;
    std::map<std::string, Object*> objects;

    // Schematic variables and CPFs
    std::map<std::string, ParametrizedVariable*> variableDefinitions;
    std::map<ParametrizedVariable*, LogicalExpression*> CPFDefinitions;

    // Instantiated Variables
    std::map<std::string, StateFluent*> stateFluents;
    std::map<std::string, ActionFluent*> actionFluents;
    std::map<std::string, NonFluent*> nonFluents;
    std::map<ParametrizedVariable*, std::vector<StateFluent*> > variablesBySchema;

    // State action constraints
    std::vector<LogicalExpression*> SACs;

    // Instantiated CPFs
    LogicalExpression* rewardCPF;
    std::vector<std::pair<StateFluent*, LogicalExpression*> > CPFs;
};

#endif

