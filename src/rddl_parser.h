#ifndef RDDL_PARSER_H
#define RDDL_PARSER_H

#include "unprocessed_planning_task.h"

class ProstPlanner;
class LogicalExpression;
class ParameterList;
class Parameter;

class RDDLParser {
public:
    RDDLParser(UnprocessedPlanningTask* _task) :
        task(_task) {}

    void parse();

private:
    UnprocessedPlanningTask* task;

    void parseDomain();
    void parseNonFluents();
    void parseInstance();

    void parseType(std::string& desc);
    void parseObject(std::string& desc);
    void parseVariableDefinition(std::string& desc);
    void parseCPFDefinition(std::string& desc);
    void parseAtomicLogicalExpression(std::string& desc);
    LogicalExpression* parseRDDLFormula(std::string desc);
    ParameterList* parseParameterList(std::string& desc);
    Parameter* parseParameter(std::string& desc);

    void getTokenName(std::string& token, std::string& name, int startPos);
    void splitToken(std::string& desc, std::vector<std::string>& result);

    std::vector<std::string> tokenizeFormula(std::string& text);
    bool isNumericConstant(std::string& token);
};

#endif
