#include "evaluatables.h"

#include "rddl.h"

#include <numeric>

using namespace std;

namespace prost::parser {
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

void ConditionalProbabilityFunction::setDomainSize(int numVals) {
    domain.resize(numVals);
    iota(domain.begin(), domain.end(), 0);
    head->domainSize = numVals;
}
} // namespace prost::parser
