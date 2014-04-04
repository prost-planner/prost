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

    PlanningTask* parseTask(std::map<std::string,int>& stateVariableIndices,
                            std::vector<std::vector<std::string> >& stateVariableValues);

private:
    std::string problemFileName;

    inline void parseActionFluent(std::stringstream& desc,
                                  std::vector<ActionFluent*>& actionFluents) const;
    inline ConditionalProbabilityFunction* parseCPF(std::stringstream& desc,
                                                    std::vector<StateFluent*>& stateFluents,
                                                    std::vector<ConditionalProbabilityFunction*>& CPFs,
                                                    std::vector<std::string>& formulas,
                                                    std::vector<std::string>& detFormulas,
                                                    bool const& isProbabilistic) const;
    inline RewardFunction* parseRewardFunction(std::stringstream& desc,
                                               std::vector<StateFluent*> const& stateFluents,
                                               std::vector<ActionFluent*> const& actionFluents) const;
    inline Evaluatable* parseActionPrecondition(std::stringstream& desc,
                                                std::vector<Evaluatable*>& actionPreconditions,
                                                std::vector<StateFluent*> const& stateFluents,
                                                std::vector<ActionFluent*> const& actionFluents) const;
    inline void parseCachingType(std::stringstream& desc,
                                 Evaluatable* eval,
                                 bool const& isProbabilistic) const;
    inline void parseActionHashKeyMap(std::stringstream& desc,
                                      Evaluatable* eval,
                                      int const& numberOfActionStates) const;
    inline void parseActionState(std::stringstream& desc,
                                 std::vector<ActionState>& actionStates,
                                 std::vector<ActionFluent*> const& actionFluents,
                                 std::vector<Evaluatable*> const& actionPreconditions) const;
    inline void parseHashKeys(std::stringstream& desc,
                              std::vector<std::vector<long> >& hashKeyBases,
                              std::vector<long>& kleeneHashKeyBases,
                              std::vector<std::vector<std::pair<int, long> > >& indexToStateFluentHashKeyMap,
                              std::vector<std::vector<std::pair<int, long> > >& indexToKleeneStateFluentHashKeyMap,
                              std::vector<ConditionalProbabilityFunction*> const& CPFs,
                              bool const& stateHashingPossible,
                              bool const& kleeneStateHashingPossible) const;
    inline void parseTrainingSet(std::stringstream& desc,
                                 int const& stateSize,
                                 int const& horizon,
                                 int const& numberOfStateFluentHashKeys,
                                 std::vector<State>& trainingSet) const;
};

#endif
