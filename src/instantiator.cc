#include "instantiator.h"

#include "typed_objects.h"
#include "logical_expressions.h"
#include "conditional_probability_function.h"

using namespace std;

void Instantiator::instantiate() {
    instantiateVariables();
    instantiateCPFs();
    instantiateSACs();
}

void Instantiator::instantiateSACs() {
    for(unsigned int i = 0; i < task->SACs.size(); ++i) {
        map<string, Object*> quantifierReplacements;
        task->SACs[i] = task->SACs[i]->replaceQuantifier(quantifierReplacements, this);
        map<string, Object*> replacements;
        task->SACs[i] = task->SACs[i]->instantiate(task, replacements);
    }
}

void Instantiator::instantiateCPFs() {
    for(unsigned int i = 0; i < task->CPFDefs.size(); ++i) {
        instantiateCPF(task->CPFDefs[i]);
    }
    map<string, Object*> quantifierReplacements;
    task->rewardCPF = task->rewardCPF->replaceQuantifier(quantifierReplacements, this);
    map<string, Object*> replacements;
    task->rewardCPF = task->rewardCPF->instantiate(task,replacements);
}

void Instantiator::instantiateCPF(pair<UninstantiatedVariable*, LogicalExpression*>& CPFDef) {
    map<string, Object*> quantifierReplacements;
    CPFDef.second = CPFDef.second->replaceQuantifier(quantifierReplacements, this);

    vector<AtomicLogicalExpression*> instantiatedVars;
    task->getVariablesOfSchema(CPFDef.first->parent, instantiatedVars);
    for(unsigned int i = 0; i < instantiatedVars.size(); ++i) {
        assert(CPFDef.first->params.size() == instantiatedVars[i]->params.size());

        map<string, Object*> replacements;
        for(unsigned int j = 0; j < CPFDef.first->params.size(); ++j) {
            assert(replacements.find(CPFDef.first->params[j]->name) == replacements.end());
            replacements[CPFDef.first->params[j]->name] = instantiatedVars[i]->params[j];
        }
        LogicalExpression* instantiatedFormula = CPFDef.second->instantiate(task,replacements);
        StateFluent* instantiatedHead = dynamic_cast<StateFluent*>(instantiatedVars[i]);
        assert(instantiatedHead);

        task->addCPF(make_pair(instantiatedHead, instantiatedFormula));
    }
}

void Instantiator::instantiateVariables() {
    vector<VariableDefinition*> variableDefinitions;
    task->getVariableDefinitions(variableDefinitions, false);
    for(unsigned int i = 0; i < variableDefinitions.size(); ++i) {
        if(!variableDefinitions[i]->params.empty()) {
            vector<vector<Object*> > instantiatedParams;
            instantiateParams(variableDefinitions[i]->params, instantiatedParams);
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

void Instantiator::instantiateParams(vector<ObjectType*> params, vector<vector<Object*> >& result, vector<Object*> addTo, int indexToProcess) {
    assert(indexToProcess < params.size());

    int nextIndex = indexToProcess + 1;

    vector<Object*> objs;
    task->getObjectsOfType(params[indexToProcess], objs);
    assert(!objs.empty());

    for(unsigned int i = 0; i < objs.size(); ++i) {
        vector<Object*> copy = addTo;
        copy.push_back(objs[i]);
        if(nextIndex < params.size()) {
            instantiateParams(params, result, copy, nextIndex);
        } else {
            result.push_back(copy);
        }
    }
}
