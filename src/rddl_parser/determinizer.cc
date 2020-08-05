#include "determinizer.h"

#include "evaluatables.h"
#include "rddl.h"

using namespace std;

namespace prost {
namespace parser {
void Determinizer::determinize() {
    // Calculate most likely determinzation of CPFs.
    map<ParametrizedVariable*, LogicalExpression*> dummy;
    for (ConditionalProbabilityFunction* cpf : task->CPFs) {
        if (cpf->isProbabilistic()) {
            LogicalExpression* det =
                cpf->formula->determinizeMostLikely(task->actionStates);
            cpf->determinization = det->simplify(dummy);
        }
    }
}
} // namespace parser
} // namespace prost
