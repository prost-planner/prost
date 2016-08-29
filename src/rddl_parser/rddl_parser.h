#ifndef RDDL_PARSER_H
#define RDDL_PARSER_H

#include <string>
#include <vector>

class PlanningTask;
class LogicalExpression;
class ParameterList;
class Parameter;

class RDDLParser {
public:
    RDDLParser();
    ~RDDLParser() {}

    PlanningTask* parse(std::string& domainFile, std::string& problemFile);

private:
    PlanningTask* task;

    void readFiles(std::string& domainFile, std::string& problemFile);

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

    // Task description as string
    std::string domainDesc;
    std::string nonFluentsDesc;
    std::string instanceDesc;

    // Names
    std::string domainName;
    std::string nonFluentsName;
    std::string instanceName;

    // tests which access private members
    friend class SimplifyTest;
};

#endif
