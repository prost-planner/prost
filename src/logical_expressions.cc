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

UninstantiatedVariable::UninstantiatedVariable(VariableDefinition* _parent, vector<Parameter*> _params) :
    parent(_parent), params(_params) {
    assert(params.size() == parent->params.size());
    name = parent->name + "(";
    for(unsigned int i = 0; i < params.size(); ++i) {
        name += params[i]->name;
        if(i != params.size()-1) {
            name += ", ";
        }
    }
    name += ")";
}

AtomicLogicalExpression::AtomicLogicalExpression(VariableDefinition* _parent, vector<Object*> _params, double _initialValue) :
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

/*****************************************************************
                             Object
*****************************************************************/

void Object::getObjectTypes(vector<ObjectType*>& objectTypes) {
    ObjectType* objType = type;
    while(objType != NULL) {
        objectTypes.push_back(objType);
        objType = objType->superType;
    }
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
