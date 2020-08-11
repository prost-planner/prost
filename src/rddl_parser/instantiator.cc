#include "instantiator.h"

#include "evaluatables.h"
#include "rddl.h"

#include "utils/timer.h"

using namespace std;

namespace prost::parser {
void Instantiator::instantiate(bool const& output) {
    utils::Timer t;
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
    instantiatePreconds();
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
            auto obj = dynamic_cast<Object*>(instantiatedVars[i]->params[j]);
            assert(obj);
            replacements[head->params[j]->name] = obj;
        }
        LogicalExpression* instantiatedFormula =
            formula->instantiate(task, replacements);

        task->CPFs.push_back(new ConditionalProbabilityFunction(
            instantiatedVars[i], instantiatedFormula));
    }
}

bool isSumOverAllActionFluents(Addition const* add, size_t numActionFluents) {
    if (!add || (add->exprs.size() != numActionFluents)) {
        return false;
    }
    vector<bool> afIsUsed(numActionFluents, false);
    for (LogicalExpression* expr : add->exprs) {
        auto af = dynamic_cast<ActionFluent*>(expr);
        if (expr && !afIsUsed[af->index]) {
            afIsUsed[af->index] = true;
        } else {
            return false;
        }
    }
    return true;
}

void Instantiator::instantiatePreconds() {
    for (auto it = task->preconds.begin(); it != task->preconds.end(); ++it) {
        map<string, Object*> replacements;
        (*it)->formula = (*it)->formula->replaceQuantifier(replacements, this);
        replacements.clear();
        (*it)->formula = (*it)->formula->instantiate(task, replacements);

        // Check if this formula encodes a constraint on the number of
        // concurrently applicable actions
        auto lee = dynamic_cast<LowerEqualsExpression*>((*it));
        auto gee = dynamic_cast<GreaterEqualsExpression*>((*it));
        auto le = dynamic_cast<LowerExpression*>((*it));
        auto ge = dynamic_cast<GreaterExpression*>((*it));
        Addition* add = nullptr;
        int value = -1;

        if (lee && lee->exprs.size() == 2) {
            add = dynamic_cast<Addition*>(lee->exprs[0]);
            auto nc = dynamic_cast<NumericConstant*>(lee->exprs[1]);
            if (nc) {
                value = static_cast<int>(nc->value);
            }
        } else if(gee && gee->exprs.size() == 2) {
            add = dynamic_cast<Addition *>(gee->exprs[1]);
            auto nc = dynamic_cast<NumericConstant*>(gee->exprs[0]);
            if (nc) {
                value = static_cast<int>(nc->value);
            }
        } else if(le && le->exprs.size() == 2) {
            add = dynamic_cast<Addition*>(le->exprs[0]);
            auto nc = dynamic_cast<NumericConstant*>(le->exprs[1]);
            if (nc) {
                value = static_cast<int>(nc->value) + 1;
            }
        } else if(ge && ge->exprs.size() == 2) {
            add = dynamic_cast<Addition*>(ge->exprs[1]);
            auto nc = dynamic_cast<NumericConstant*>(ge->exprs[0]);
            if (nc) {
                value = static_cast<int>(nc->value) - 1;
            }
        }

        if ((value >= 1) &&
            isSumOverAllActionFluents(add, task->actionFluents.size())) {
            task->numberOfConcurrentActions = value;
            it = task->preconds.erase(it) - 1;
        }
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
} // namespace prost::parser
