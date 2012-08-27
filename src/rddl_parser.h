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
    LogicalExpression* parseRDDLFormula(std::string& desc, UnprocessedPlanningTask* _task);

private:
    UnprocessedPlanningTask* task;

    void parseDomain();
    void parseNonFluents();
    void parseInstance();

    void getTokenName(std::string& token, std::string& name, int startPos);
    void splitToken(std::string& desc, std::vector<std::string>& result);

    void tokenizeFormula(std::string& text, std::vector<std::string>& tokens);
    bool isParameterDefinition(std::vector<std::string>& tokens);
    bool isNumericConstant(std::string& token);
};

#endif
