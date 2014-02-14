#ifndef INSTANTIATOR_H
#define INSTANTIATOR_H

#include "planning_task.h"
#include "unprocessed_planning_task.h"

class ProstPlanner;

class Instantiator {
public:
    Instantiator(ProstPlanner* _planner, UnprocessedPlanningTask* _task) :
        planner(_planner), task(_task) {}

    void instantiate();

    void instantiateParams(UnprocessedPlanningTask* _task, std::vector<ObjectType*> params, std::vector<std::vector<Object*> >& result, 
                           std::vector<Object*> addTo = std::vector<Object*>(), int indexToProcess = 0);

private:
    ProstPlanner* planner;
    UnprocessedPlanningTask* task;

    void instantiateVariables();
    void instantiateCPFs();
    void instantiateCPF(std::pair<UninstantiatedVariable*, LogicalExpression*>& CPFDef);
    void instantiateSACs();
};

#endif
