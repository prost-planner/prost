#include "logical_expressions.h"

#include "csp.h"
#include "instantiator.h"
#include "probability_distribution.h"
#include "rddl.h"

#include "utils/math.h"
#include "utils/system.h"

#include <cmath>
#include <iostream>
#include <limits>

using namespace std;

namespace prost::parser {
Object::Object(std::string _name, Type* _type)
    : Parameter(_name, _type), types() {
    Type* tmpType = type;

    while (tmpType) {
        types.push_back(tmpType);
        value = tmpType->objects.size();
        tmpType->objects.push_back(this);
        tmpType = tmpType->superType;
    }
}

// This constructor is used for instantiation
ParametrizedVariable::ParametrizedVariable(ParametrizedVariable const& source,
                                           std::vector<Parameter*> _params)
    : LogicalExpression(),
      variableName(source.variableName),
      fullName(source.variableName),
      params(_params),
      variableType(source.variableType),
      valueType(source.valueType),
      initialValue(source.initialValue) {
    assert(params.size() == source.params.size());
    for (unsigned int i = 0; i < params.size(); ++i) {
        if (!params[i]->type) {
            params[i]->type = source.params[i]->type;
        } else {
            assert(params[i]->type->isSubtypeOf(source.params[i]->type));
        }
    }

    if (!params.empty()) {
        fullName += "(";
        for (unsigned int i = 0; i < params.size(); ++i) {
            fullName += params[i]->name;
            if (i != params.size() - 1) {
                fullName += ", ";
            }
        }
        fullName += ")";
    }
}

ParametrizedVariable::ParametrizedVariable(ParametrizedVariable const& source,
                                           std::vector<Parameter*> _params,
                                           double _initialValue)
    : ParametrizedVariable(source, _params) {
    initialValue = _initialValue;
}

ActionFluent::ActionFluent(ParametrizedVariable const& source,
                           std::vector<Parameter*> _params, int _index)
    : ParametrizedVariable(source, _params, 0.0),
      index(_index),
      isFDR(source.valueType->name != "bool") {
    // To support action fluent with a different initial value than 0, we have
    // to adapt the simplification
    assert(utils::doubleIsEqual(initialValue, 0.0));
}

ActionFluent::ActionFluent(std::string name, Type* _valueType, int _index)
    : ParametrizedVariable(name, {}, VariableType::ACTION_FLUENT, _valueType,
                           0.0),
      index(_index),
      isFDR(_valueType->name != "bool") {
    // To support action fluent with a different initial value than 0, we have
    // to adapt the simplification
    assert(utils::doubleIsEqual(initialValue, 0.0));
}

bool Type::isSubtypeOf(Type* const& other) const {
    Type const* comp = this;
    while (comp) {
        if (other == comp) {
            return true;
        }
        comp = comp->superType;
    }
    return false;
}

#include "logical_expressions_includes/calculate_domain.cc"
#include "logical_expressions_includes/calculate_domain_as_interval.cc"
#include "logical_expressions_includes/collect_initial_info.cc"
#include "logical_expressions_includes/evaluate.cc"
#include "logical_expressions_includes/evaluate_to_kleene.cc"
#include "logical_expressions_includes/evaluate_to_pd.cc"
#include "logical_expressions_includes/instantiate.cc"
#include "logical_expressions_includes/pretty_print.cc"
#include "logical_expressions_includes/print.cc"
#include "logical_expressions_includes/replace_quantifier.cc"
#include "logical_expressions_includes/simplify.cc"
#include "logical_expressions_includes/to_z3_formula.cc"
} // namespace prost::parser
