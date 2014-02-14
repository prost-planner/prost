#include "logical_expressions.h"

#include "rddl_parser.h"
#include "instantiator.h"
#include "search_engine.h"
#include "typed_objects.h"

#include "utils/string_utils.h"
#include "utils/math_utils.h"
#include "utils/system_utils.h"

#include <iostream>

using namespace std;

/*****************************************************************
                        Constructors
*****************************************************************/

UninstantiatedVariable::UninstantiatedVariable(VariableDefinition* _parent, std::vector<std::string> _params) :
    parent(_parent), params(_params) {
    assert(params.size() == parent->params.size());
    name = parent->name + "(";
    for(unsigned int i = 0; i < params.size(); ++i) {
        name += params[i];
        if(i != params.size()-1) {
            name += ", ";
        }
    }
    name += ")";
}

AtomicLogicalExpression::AtomicLogicalExpression(VariableDefinition* _parent, std::vector<Object*> _params, double _initialValue) :
    parent(_parent), params(_params), initialValue(_initialValue), index(-1) {

    assert(_parent->params.size() == params.size());
    name = _parent->name + "(";
    for(unsigned int i = 0; i < params.size(); ++i) {
        name += params[i]->name;
        if(i != params.size()-1) {
            name += ", ";
        }
    }
    name += ")";
}

Quantifier::Quantifier(std::vector<LogicalExpression*>& _exprs) {
    assert(_exprs.size() == 2);
    parameterDefsSet = dynamic_cast<ParameterDefinitionSet*>(_exprs[0]);
    assert(parameterDefsSet);
    expr = _exprs[1];
}

#include "logical_expressions_includes/instantiate.cc"
#include "logical_expressions_includes/replace_quantifier.cc"
#include "logical_expressions_includes/simplify.cc"
#include "logical_expressions_includes/collect_initial_info.cc"
#include "logical_expressions_includes/calculate_domain.cc"
#include "logical_expressions_includes/calculate_pd_domain.cc"
#include "logical_expressions_includes/determinization.cc"
#include "logical_expressions_includes/evaluate.cc"
#include "logical_expressions_includes/evaluate_to_pd.cc"
#include "logical_expressions_includes/evaluate_to_kleene.cc"
#include "logical_expressions_includes/print.cc"
