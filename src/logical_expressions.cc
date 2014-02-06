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

#include "logical_expressions_includes/initialization.cc"
#include "logical_expressions_includes/instantiate.cc"
#include "logical_expressions_includes/replace_quantifier.cc"
#include "logical_expressions_includes/simplify.cc"
#include "logical_expressions_includes/collect_initial_info.cc"
#include "logical_expressions_includes/calculate_domain.cc"
#include "logical_expressions_includes/calculate_prob_domain.cc"
#include "logical_expressions_includes/determinization.cc"
#include "logical_expressions_includes/evaluate.cc"
#include "logical_expressions_includes/evaluate_to_kleene_outcome.cc"
#include "logical_expressions_includes/print.cc"
