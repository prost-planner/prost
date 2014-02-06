#include "conditional_probability_function.h"

#include "prost_planner.h"
#include "actions.h"
#include "rddl_parser.h"

#include "utils/string_utils.h"

#include <iostream>
#include <cassert>

using namespace std;

/*****************************************************************
                 ConditionalProbabilityFunction
*****************************************************************/

bool ConditionalProbabilityFunction::simplify(UnprocessedPlanningTask* _task, map<StateFluent*, NumericConstant*>& replacements) {
    formula = formula->simplify(_task, replacements);
    NumericConstant* nc = dynamic_cast<NumericConstant*>(formula);
    if(nc && MathUtils::doubleIsEqual(head->initialValue, nc->value)) {
        assert(replacements.find(head) == replacements.end());
        replacements[head] = nc;
        return true;
    }
    return false;
}

ConditionalProbabilityFunction* ConditionalProbabilityFunction::determinizeMostLikely(NumericConstant* randomNumberReplacement, 
                                                                                      UnprocessedPlanningTask* _task) {
    // If this is not probabilistic, we return this to reuse the
    // caches as often as possible. This is nevertheless a potential
    // error source that should be kept in mind!
    if(!isProbabilistic()) {
        return this;
    }

    LogicalExpression* detFormula = formula->determinizeMostLikely(randomNumberReplacement);
    ConditionalProbabilityFunction* res = new ConditionalProbabilityFunction(*this, detFormula);

    // We know these because detFormula must be deterministic, and
    // therefore this has a domain of 2 with values 0 and 1.
    res->isProb = false;
    // res->probDomainMap.clear();
    // res->probDomainMap[0.0] = 0;
    // res->probDomainMap[1.0] = res->hashKeyBase;

    // We run initialization again, as things might have changed
    // compared to the probabilistic task. Therefore, we need to reset
    // some member variables to their initial value
    res->dependentStateFluents.clear();
    res->initialize();

    // The same is true for simplification. Rather than calling
    // simplify(), though, we call formula->simplify. This is because
    // the function also checks if this CPF can be omitted entirely,
    // which is never true in a determinization
    map<StateFluent*,NumericConstant*> replacements;
    res->formula = res->formula->simplify(_task, replacements);

    return res;
}

void ConditionalProbabilityFunction::initializeDomains(vector<ActionState> const& actionStates) {
    assert(domain.empty());

    for(unsigned int actionIndex = 0; actionIndex < actionStates.size(); ++actionIndex) {
        // Calculate the values that can be achieved if the action
        // actionStates[actionIndex] is applied and add it to the domain
        set<double> actionDependentValues;
        formula->calculateDomain(actionStates[actionIndex], actionDependentValues);
        domain.insert(actionDependentValues.begin(), actionDependentValues.end());
    }

    // The number of possible values of this variable in Kleene states is 2^0 +
    // 2^1 + ... + 2^{n-1} = 2^n -1 with n = CPFs[i]->domain.size()
    kleeneDomainSize = 2;
    if(!MathUtils::toThePowerOfWithOverflowCheck(kleeneDomainSize, domain.size())) {
        // TODO: Check if this variable has an infinte domain (e.g. reals or
        // ints in inifinte horizon). A domain size of 0 represents infinite
        // domains or domains that are too large to be considered finite.
        kleeneDomainSize = 0;
    } else {
        --kleeneDomainSize;
    }

    if(isBoolean() && isProbabilistic()) {
        set<double> actionIndependentValues;

        for(unsigned int actionIndex = 0; actionIndex < actionStates.size(); ++actionIndex) {
            // Calculate the values that can be achieved if the action
            // with actionIndex is applied and add it to the values of
            // all other actions
            set<double> actionDependentValues;
            formula->calculateProbDomain(actionStates[actionIndex], actionDependentValues);
            actionIndependentValues.insert(actionDependentValues.begin(), actionDependentValues.end());
        }

        // Enumerate all values with 0...domainSize. We will later
        // multiply these with the hash key base once it's determined
        // (this happens in planning_task.cc)
        // int counter = 0;
        // for(set<double>::iterator it = actionIndependentValues.begin(); it != actionIndependentValues.end(); ++it) {
        //     probDomainMap[*it] = counter;
        //     ++counter;
        // }
    } // else {
    //     probDomainMap[0.0] = 0;
    //     probDomainMap[1.0] = 1;
    // }
}


/*****************************************************************
           ConditionalProbabilityFunctionDefinition
*****************************************************************/

void ConditionalProbabilityFunctionDefinition::parse(string& desc, UnprocessedPlanningTask* task, RDDLParser* parser) {
    size_t cutPos = desc.find("=");
    assert(cutPos != string::npos);

    string nameAndParams = desc.substr(0,cutPos);
    StringUtils::trim(nameAndParams);
    string rest = desc.substr(cutPos+1);
    StringUtils::trim(rest);

    string name;
    vector<string> params;
    StringUtils::trim(nameAndParams);

    if(nameAndParams.compare("reward") != 0) {
        StringUtils::removeFirstAndLastCharacter(nameAndParams);
    }

    vector<string> nameAndParamsVec;
    StringUtils::split(nameAndParams,nameAndParamsVec," ");

    assert(nameAndParamsVec.size() > 0);
    name = nameAndParamsVec[0];
    StringUtils::trim(name);

    if(name[name.length()-1] == '\'') {
        name = name.substr(0,name.length()-1);
    }

    VariableDefinition* headParent = task->getVariableDefinition(name);

    vector<string> headParams;
    for(unsigned int i = 1; i < nameAndParamsVec.size(); ++i) {
        headParams.push_back(nameAndParamsVec[i]);
    }
    UninstantiatedVariable* head = new UninstantiatedVariable(headParent, headParams);
    LogicalExpression* formula = parser->parseRDDLFormula(rest,task, head->parent->valueType->name);

    task->addCPFDefinition(new ConditionalProbabilityFunctionDefinition(head,formula));
}

void ConditionalProbabilityFunctionDefinition::replaceQuantifier(UnprocessedPlanningTask* task, Instantiator* instantiator) {
    map<string, string> replacements;
    formula = formula->replaceQuantifier(task, replacements, instantiator);
}

ConditionalProbabilityFunction* ConditionalProbabilityFunctionDefinition::instantiate(UnprocessedPlanningTask* task, 
                                                                                      AtomicLogicalExpression* variable) {
    assert(head->params.size() == variable->params.size());

    map<string, Object*> replacements;
    for(unsigned int i = 0; i < head->params.size(); ++i) {
        assert(replacements.find(head->params[i]) == replacements.end());
        replacements[head->params[i]] = variable->params[i];
    }
    LogicalExpression* newFormula = formula->instantiate(task,replacements);
    StateFluent* tmp = dynamic_cast<StateFluent*>(variable);
    assert(tmp != NULL);
    return new ConditionalProbabilityFunction(tmp, newFormula);
}

void ConditionalProbabilityFunctionDefinition::print(ostream& out) {
    head->print(out);
    out << " = ";
    formula->print(out);
    out << endl;
}
