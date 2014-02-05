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

void ConditionalProbabilityFunction::initialize() {
    Evaluatable::initialize();
    if(head->parent->valueType == BoolType::instance()) {
        // this is a boolean variable
        domainSize = 2;
    } else if(!head->parent->valueType->domain.empty()) {
        // this variable has a custom domain (enum or object fluent)
        domainSize = head->parent->valueType->domain.size();
    }
}

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
    res->probDomainSize = 2;
    res->probDomainMap.clear();
    res->probDomainMap[0.0] = 0;
    res->probDomainMap[1.0] = 1;

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
