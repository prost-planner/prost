#ifndef RDDL_PARSER_H
#define RDDL_PARSER_H

#include "unprocessed_planning_task.h"

class ProstPlanner;
class LogicalExpression;

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
    LogicalExpression* parseRDDLFormula(std::string& desc, std::string enumContext);

    void getTokenName(std::string& token, std::string& name, int startPos);
    void splitToken(std::string& desc, std::vector<std::string>& result);

    void tokenizeFormula(std::string& text, std::vector<std::string>& tokens);
    bool isParameterDefinition(std::vector<std::string>& tokens);
    bool isNumericConstant(std::string& token);
};

#endif
