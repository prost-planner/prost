#ifndef INSTANTIATOR_H
#define INSTANTIATOR_H

#include <vector>

class UnprocessedPlanningTask;
class Parameter;
class ParametrizedVariable;
class LogicalExpression;

class Instantiator {
public:
    Instantiator(UnprocessedPlanningTask* _task) :
        task(_task) {}

    void instantiate();
    void instantiateParams(std::vector<Parameter*> params, std::vector<std::vector<Parameter*> >& result, 
                           std::vector<Parameter*> addTo = std::vector<Parameter*>(), int indexToProcess = 0);

private:
    UnprocessedPlanningTask* task;

    void instantiateVariables();
    void instantiateCPFs();
    void instantiateCPF(ParametrizedVariable* head, LogicalExpression* formula);
    void instantiateSACs();
};

#endif
