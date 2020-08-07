#include "evaluatables.h"

#include "rddl.h"

#include "utils/system_utils.h"

#include <iostream>
#include <numeric>

using namespace std;

namespace prost {
namespace parser {
void Evaluatable::initialize() {
    isProb = false;
    hasArithmeticFunction = false;
    dependentStateFluents.clear();
    dependentActionFluents.clear();
    formula->collectInitialInfo(isProb, hasArithmeticFunction,
                                dependentStateFluents, dependentActionFluents);
}

void Evaluatable::simplify(Simplifications& replace) {
    formula = formula->simplify(replace);
    initialize();
}

void ConditionalProbabilityFunction::setDomain(int maxVal) {
    domain.resize(maxVal + 1);
    iota(domain.begin(), domain.end(), 0);
    head->domainSize = maxVal + 1;
}
} // namespace parser
} // namespace prost
