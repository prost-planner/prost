#include "instantiator.h"

#include "evaluatables.h"
#include "rddl.h"

#include "utils/timer.h"

#include <iostream>

using namespace std;

void Instantiator::instantiate(bool const& output) {
    Timer t;
    if (output)
        cout << "    Instantiating variables..." << endl;
    instantiateVariables();
    if (output)
        cout << "    ...finished (" << t() << ")" << endl;
    t.reset();

    if (output)
        cout << "    Instantiating CPFs..." << endl;
    instantiateCPFs();
    if (output)
        cout << "    ...finished (" << t() << ")" << endl;
    t.reset();

    if (output)
        cout << "    Instantiating preconditions..." << endl;
    instantiateSACs();
    if (output)
        cout << "    ...finished (" << t() << ")" << endl;
    t.reset();
}

void Instantiator::instantiateVariables() {
    for (map<string, ParametrizedVariable*>::iterator it =
             task->variableDefinitions.begin();
         it != task->variableDefinitions.end(); ++it) {
        ParametrizedVariable*& var = it->second;
        if (!var->params.empty()) {
            vector<vector<Parameter*>> instantiatedParams;
            instantiateParams(var->params, instantiatedParams);
            for (unsigned int j = 0; j < instantiatedParams.size(); ++j) {
                task->addParametrizedVariable(var, instantiatedParams[j]);
            }
        } else {
            task->addParametrizedVariable(var, vector<Parameter*>());
        }
    }
}

void Instantiator::instantiateCPFs() {
    for (map<ParametrizedVariable*, LogicalExpression*>::iterator it =
             task->CPFDefinitions.begin();
         it != task->CPFDefinitions.end(); ++it) {
        instantiateCPF(it->first, it->second);
    }

    // Instantiate rewardCPF
    map<string, Object*> quantifierReplacements;
    task->rewardCPF->formula = task->rewardCPF->formula->replaceQuantifier(
        quantifierReplacements, this);
    map<string, Object*> replacements;
    task->rewardCPF->formula =
        task->rewardCPF->formula->instantiate(task, replacements);
}

void Instantiator::instantiateCPF(ParametrizedVariable* head,
                                  LogicalExpression* formula) {
    map<string, Object*> quantifierReplacements;
    formula = formula->replaceQuantifier(quantifierReplacements, this);

    vector<StateFluent*> instantiatedVars = task->getStateFluentsOfSchema(head);
    for (unsigned int i = 0; i < instantiatedVars.size(); ++i) {
        assert(head->params.size() == instantiatedVars[i]->params.size());

        map<string, Object*> replacements;
        for (unsigned int j = 0; j < head->params.size(); ++j) {
            assert(replacements.find(head->params[j]->name) ==
                   replacements.end());
            Object* obj = dynamic_cast<Object*>(instantiatedVars[i]->params[j]);
            assert(obj);
            replacements[head->params[j]->name] = obj;
        }
        LogicalExpression* instantiatedFormula =
            formula->instantiate(task, replacements);

        task->CPFs.push_back(new ConditionalProbabilityFunction(
            instantiatedVars[i], instantiatedFormula));
    }
}

void Instantiator::instantiateSACs() {
    for (unsigned int i = 0; i < task->SACs.size(); ++i) {
        map<string, Object*> quantifierReplacements;
        task->SACs[i] =
            task->SACs[i]->replaceQuantifier(quantifierReplacements, this);
        map<string, Object*> replacements;
        task->SACs[i] = task->SACs[i]->instantiate(task, replacements);
    }
}

void Instantiator::instantiateParams(vector<Parameter*> params,
                                     vector<vector<Parameter*>>& result,
                                     vector<Parameter*> addTo,
                                     int indexToProcess) {
    assert(indexToProcess < params.size());

    int nextIndex = indexToProcess + 1;

    assert(params[indexToProcess]->type);
    vector<Object*>& objs = params[indexToProcess]->type->objects;

    for (unsigned int i = 0; i < objs.size(); ++i) {
        vector<Parameter*> copy = addTo;
        copy.push_back(objs[i]);
        if (nextIndex < params.size()) {
            instantiateParams(params, result, copy, nextIndex);
        } else {
            result.push_back(copy);
        }
    }
}
