#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <map>
#include <string>

class PlanningTask;
class ActionFluent;
class StateFluent;
class Evaluatable;
class ConditionalProbabilityFunction;
class RewardFunction;
class ActionState;
class State;

class Parser {
public:
    Parser(std::string _problemFileName) :
        problemFileName(_problemFileName) {}

    void parseTask(std::map<std::string,int>& stateVariableIndices,
                   std::vector<std::vector<std::string> >& stateVariableValues);

private:
    std::string problemFileName;

    inline void parseActionFluent(std::stringstream& desc) const;
    inline void parseCPF(std::stringstream& desc,
                         std::vector<std::string>& formulas,
                         std::vector<std::string>& detFormulas,
                         bool const& isProbabilistic) const;
    inline void parseRewardFunction(std::stringstream& desc) const;
    inline void parseActionPrecondition(std::stringstream& desc) const;
    inline void parseCachingType(std::stringstream& desc,
                                 Evaluatable* probEval,
                                 Evaluatable* detEval) const;
    inline void parseActionHashKeyMap(std::stringstream& desc,
                                      Evaluatable* probEval,
                                      Evaluatable* detEval) const;
    inline void parseActionState(std::stringstream& desc) const;
    inline void parseHashKeys(std::stringstream& desc) const;
    inline void parseTrainingSet(std::stringstream& desc) const;
};

#endif
