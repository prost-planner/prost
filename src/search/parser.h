#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <map>
#include <string>

class PlanningTask;

class Parser {
public:
    Parser(std::string _problemFileName) :
        problemFileName(_problemFileName) {}

    PlanningTask* parseTask(std::map<std::string,int>& stateVariableIndices, std::vector<std::vector<std::string> >& stateVariableValues);

private:
    std::string problemFileName;
};

#endif /* PARSER_H */
