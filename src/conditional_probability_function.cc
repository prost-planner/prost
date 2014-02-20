#include "conditional_probability_function.h"

#include "prost_planner.h"
#include "actions.h"
#include "rddl_parser.h"

#include "utils/string_utils.h"

#include <iostream>
#include <cassert>

using namespace std;

ConditionalProbabilityFunction* ConditionalProbabilityFunction::determinizeMostLikely(NumericConstant* randomNumberReplacement) {
    // If this is not probabilistic, we return this to reuse the caches as often
    // as possible. This is nevertheless a potential error source that should be
    // kept in mind!
    if(!isProbabilistic()) {
        return this;
    }

    LogicalExpression* detFormula = formula->determinizeMostLikely(randomNumberReplacement);
    ConditionalProbabilityFunction* res = new ConditionalProbabilityFunction(*this, detFormula);

    // We know these because detFormula must be deterministic, and therefore
    // this has a domain of 2 with values 0 and 1.
    res->isProb = false;
    // res->probDomainMap.clear();
    // res->probDomainMap[0.0] = 0;
    // res->probDomainMap[1.0] = res->hashKeyBase;

    // We run initialization again, as things might have changed compared to the
    // probabilistic task. Therefore, we need to reset some member variables to
    // their initial value
    res->dependentStateFluents.clear();
    res->initialize();

    // The same is true for simplification. Rather than calling simplify(),
    // though, we call formula->simplify. This is because the function also
    // checks if this CPF can be omitted entirely, which is never true in a
    // determinization
    map<StateFluent*, double> replacements;
    res->formula = res->formula->simplify(replacements);

    return res;
}

void ConditionalProbabilityFunction::initializeDomains(vector<ActionState> const& /*actionStates*/) {
    // assert(domain.empty());

    // // for(unsigned int actionIndex = 0; actionIndex < actionStates.size(); ++actionIndex) {
    // //     // Calculate the values that can be achieved if the action
    // //     // actionStates[actionIndex] is applied and add it to the domain
    // //     set<double> actionDependentValues;
    // //     formula->calculateDomain(actionStates[actionIndex], actionDependentValues);
    // //     domain.insert(actionDependentValues.begin(), actionDependentValues.end());
    // // }

    // // The number of possible values of this variable in Kleene states is 2^0 +
    // // 2^1 + ... + 2^{n-1} = 2^n -1 with n = CPFs[i]->domain.size()
    // kleeneDomainSize = 2;
    // if(!MathUtils::toThePowerOfWithOverflowCheck(kleeneDomainSize, domain.size())) {
    //     // TODO: Check if this variable has an infinte domain (e.g. reals or
    //     // ints in inifinte horizon). A domain size of 0 represents infinite
    //     // domains or domains that are too large to be considered finite.
    //     kleeneDomainSize = 0;
    // } else {
    //     --kleeneDomainSize;
    // }

    // if(isBoolean() && isProbabilistic()) {
    //     set<double> actionIndependentValues;

    //     for(unsigned int actionIndex = 0; actionIndex < actionStates.size(); ++actionIndex) {
    //         // Calculate the values that can be achieved if the action
    //         // with actionIndex is applied and add it to the values of
    //         // all other actions
    //         set<double> actionDependentValues;
    //         formula->calculateProbDomain(actionStates[actionIndex], actionDependentValues);
    //         actionIndependentValues.insert(actionDependentValues.begin(), actionDependentValues.end());
    //     }

    //     // Enumerate all values with 0...domainSize. We will later
    //     // multiply these with the hash key base once it's determined
    //     // (this happens in planning_task.cc)
    //     // int counter = 0;
    //     // for(set<double>::iterator it = actionIndependentValues.begin(); it != actionIndependentValues.end(); ++it) {
    //     //     probDomainMap[*it] = counter;
    //     //     ++counter;
    //     // }
    // } // else {
    // //     probDomainMap[0.0] = 0;
    // //     probDomainMap[1.0] = 1;
    // // }
}

