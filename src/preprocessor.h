#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <list>
#include <map>
#include <string>
#include <vector>

class ProstPlanner;
class UnprocessedPlanningTask;
class PlanningTask;

class Preprocessor {
public:
    Preprocessor(ProstPlanner* _planner, UnprocessedPlanningTask* _task, PlanningTask* _probPlanningTask) :
        planner(_planner), task(_task), probPlanningTask(_probPlanningTask) {}

    PlanningTask* preprocess(std::map<std::string,int>& stateVariableIndices, std::vector<std::vector<std::string> >& stateVariableValues);

private:
    ProstPlanner* planner;
    UnprocessedPlanningTask* task;
    PlanningTask* probPlanningTask;

    void simplifyFormulas();
    void createProbabilisticPlanningTaskRepresentation(std::map<std::string,int>& stateVariableIndices, std::vector<std::vector<std::string> >& stateVariableValues);
};

#endif
