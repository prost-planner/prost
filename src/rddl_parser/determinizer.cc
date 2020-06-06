#include "determinizer.h"

#include "evaluatables.h"
#include "rddl.h"

using namespace std;

void Determinizer::determinize() {
    // Calculate most likely determinzation of CPFs.
    std::map<ParametrizedVariable*, LogicalExpression*> dummy;
    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        if (cpf->isProbabilistic()) {
            LogicalExpression* det =
                cpf->formula->determinizeMostLikely(task->actionStates);
            cpf->determinization = det->simplify(dummy);
        }
    }
}