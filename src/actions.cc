#include "actions.h"

#include "conditional_probability_function.h"
#include "state_action_constraint.h"

#include "utils/math_utils.h"

#include <iostream>
#include <limits>

using namespace std;

// Writes the name of this action to the result vector (used only for
// communication with the environment)
void ActionState::getActions(vector<string>& result) const {
    for(unsigned int i = 0; i < scheduledActionFluents.size(); ++i) {
        result.push_back(scheduledActionFluents[i]->name);
    }
}

void ActionState::calculateProperties(vector<ActionFluent*> const& actionFluents, vector<StateActionConstraint*> const& dynamicSACs) {
    // Determine which action fluents are scheduled in this action
    for(unsigned int i = 0; i < state.size(); ++i) {
        if(state[i]) {
            scheduledActionFluents.push_back(actionFluents[i]);
        }
    }

    // Determine which dynamic SACs might make this action state
    // inapplicable
    for(unsigned int i = 0; i < dynamicSACs.size(); ++i) {
        if(dynamicSACs[i]->containsArithmeticFunction()) {
            // If the SAC contains an arithmetic function we treat it
            // as if it influenced this action. TODO: Implement a
            // function that checks if it does influence this!
            relevantSACs.push_back(dynamicSACs[i]);
        } else if(containsNegativeActionFluent(dynamicSACs[i])) {
            // If the SAC contains one of this ActionStates' action
            // fluents negatively it might forbid this action.
            relevantSACs.push_back(dynamicSACs[i]);
        } else if(containsAdditionalPositiveActionFluent(dynamicSACs[i])) {
            // If the SAC contains action fluents positively that are
            // not in this ActionStates' action fluents it might
            // enforce that action fluent (and thereby forbid this
            // action)
            relevantSACs.push_back(dynamicSACs[i]);
        }
    }
}

inline bool ActionState::containsNegativeActionFluent(StateActionConstraint* sac) const {
    set<ActionFluent*> const& actionFluents = sac->getNegativeDependentActionFluents();

    for(unsigned int index = 0; index < scheduledActionFluents.size(); ++index) {
        if(actionFluents.find(scheduledActionFluents[index]) != actionFluents.end()) {
            return true;
        }
    }
    return false;
}

bool ActionState::containsAdditionalPositiveActionFluent(StateActionConstraint* sac) const {
    set<ActionFluent*> const& actionFluents = sac->getPositiveDependentActionFluents();

    for(set<ActionFluent*>::iterator it = actionFluents.begin(); it != actionFluents.end(); ++it) {
        bool isScheduledActionFluent = false;
        for(unsigned int index = 0; index < scheduledActionFluents.size(); ++index) {
            if((*it) == scheduledActionFluents[index]) {
                isScheduledActionFluent = true;
                break;
            }
        }
        if(!isScheduledActionFluent) {
            return true;
        }
    }

    return false;
}

bool ActionState::isApplicable(State const& current) const {
    DiscretePD res;
    for(unsigned int sacIndex = 0; sacIndex < relevantSACs.size(); ++sacIndex) {
        relevantSACs[sacIndex]->evaluate(res, current, *this);
        if(res.isFalsity()) {
            return false;
        }
        assert(res.isTruth());
    }
    return true;
}

