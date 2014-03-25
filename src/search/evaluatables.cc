#include "evaluatables.h"

#include "planning_task.h"

#include "utils/system_utils.h"

using namespace std;

/*****************************************************************
                           Evaluatable
*****************************************************************/

void Evaluatable::setFormula(vector<StateFluent*> const& stateFluents, vector<ActionFluent*> const& actionFluents, string& desc, bool const& isDeterminization) {
    if(isDeterminization) {
        assert(formula);
        determinizedFormula = LogicalExpression::createFromString(stateFluents, actionFluents, desc);
    } else {
        assert(!formula);
        formula = LogicalExpression::createFromString(stateFluents, actionFluents, desc);
        determinizedFormula = formula;
    }
}

void Evaluatable::disableCaching() {
    //We only disable caching if it is done in maps as the 
    //space for vectors is already reserved and thus not growing.
    if(cachingType == MAP) {
        cachingType = DISABLED_MAP;
    }

    if(kleeneCachingType == MAP) {
        kleeneCachingType = DISABLED_MAP;
    }
}
