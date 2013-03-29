#include "instantiator.h"

#include "typed_objects.h"
#include "logical_expressions.h"
#include "conditional_probability_function.h"
#include "state_action_constraint.h"

using namespace std;

void Instantiator::instantiate() {
    instantiateVariables();
    instantiateCPFs();
    instantiateSACs();
}

void Instantiator::instantiateSACs() {
    vector<StateActionConstraint*> sacs;
    task->getStateActionConstraints(sacs);
    for(unsigned int i = 0; i < sacs.size(); ++i) {
        sacs[i]->replaceQuantifier(task, this);
        sacs[i]->instantiate(task);
    }
}

void Instantiator::instantiateCPFs() {
    vector<ConditionalProbabilityFunction*> result;
    vector<ConditionalProbabilityFunctionDefinition*> CPFDefs;
    task->getCPFDefinitions(CPFDefs, true);
    for(unsigned int i = 0; i < CPFDefs.size(); ++i) {
        CPFDefs[i]->replaceQuantifier(task, this);
        vector<AtomicLogicalExpression*> instantiatedVars;
        task->getVariablesOfSchema(CPFDefs[i]->head->parent, instantiatedVars);
        for(unsigned int j = 0; j < instantiatedVars.size(); ++j) {
            task->addCPF(CPFDefs[i]->instantiate(task, instantiatedVars[j]));
        }
    }
}

void Instantiator::instantiateVariables() {
    vector<VariableDefinition*> variableDefinitions;
    task->getVariableDefinitions(variableDefinitions, false);
    for(unsigned int i = 0; i < variableDefinitions.size(); ++i) {
        if(!variableDefinitions[i]->params.empty()) {
            vector<vector<Object*> > instantiatedParams;
            instantiateParams(task, variableDefinitions[i]->params, instantiatedParams);
            for(unsigned int j = 0; j < instantiatedParams.size(); ++j) {
                switch(variableDefinitions[i]->variableType) {
                case VariableDefinition::STATE_FLUENT:
                case VariableDefinition::INTERM_FLUENT:
                    task->addStateFluent(new StateFluent(variableDefinitions[i], instantiatedParams[j]));
                    break;
                case VariableDefinition::ACTION_FLUENT:
                    task->addActionFluent(new ActionFluent(variableDefinitions[i], instantiatedParams[j]));
                    break;
                case VariableDefinition::NON_FLUENT:
                    task->addNonFluent(new NonFluent(variableDefinitions[i], instantiatedParams[j]));
                    break;
                }
            }
        } else {
            switch(variableDefinitions[i]->variableType) {
            case VariableDefinition::STATE_FLUENT:
            case VariableDefinition::INTERM_FLUENT:
                task->addStateFluent(new StateFluent(variableDefinitions[i], vector<Object*>()));
                    break;
            case VariableDefinition::ACTION_FLUENT:
                task->addActionFluent(new ActionFluent(variableDefinitions[i], vector<Object*>()));
                break;
            case VariableDefinition::NON_FLUENT:
                task->addNonFluent(new NonFluent(variableDefinitions[i], vector<Object*>()));
                break;
            }
        }
    }
}

void Instantiator::instantiateParams(UnprocessedPlanningTask* _task, vector<ObjectType*> params, vector<vector<Object*> >& result, vector<Object*> addTo, int indexToProcess) {
    assert(indexToProcess < params.size());

    int nextIndex = indexToProcess + 1;

    vector<Object*> objs;
    _task->getObjectsOfType(params[indexToProcess], objs);
    assert(!objs.empty());

    for(unsigned int i = 0; i < objs.size(); ++i) {
        vector<Object*> copy = addTo;
        copy.push_back(objs[i]);
        if(nextIndex < params.size()) {
            instantiateParams(_task, params, result, copy, nextIndex);
        } else {
            result.push_back(copy);
        }
    }
}
