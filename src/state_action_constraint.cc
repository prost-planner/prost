#include "state_action_constraint.h"

#include "rddl_parser.h"

using namespace std;

void StateActionConstraint::parse(string& desc, UnprocessedPlanningTask* task, RDDLParser* parser) {
    LogicalExpression* formula = parser->parseRDDLFormula(desc,task, "SAC");
    task->addStateActionConstraint(new StateActionConstraint("SAC ", formula));
}

void StateActionConstraint::replaceQuantifier(UnprocessedPlanningTask* task, Instantiator* instantiator) {
    map<string, string> replacements;
    formula = formula->replaceQuantifier(task, replacements, instantiator);
}

void StateActionConstraint::instantiate(UnprocessedPlanningTask* task) {
    map<string, Object*> replacements;
    formula = formula->instantiate(task, replacements);
}

bool StateActionConstraint::simplify(UnprocessedPlanningTask* task, map<StateFluent*, NumericConstant*>& replacements) {
    formula = formula->simplify(task, replacements);
    NumericConstant* nc = dynamic_cast<NumericConstant*>(formula);
    if(nc) {
        assert(nc != NumericConstant::falsity());
        return true;
    }
    return false;
}


