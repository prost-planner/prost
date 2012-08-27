#include "actions.h"

#include "conditional_probability_functions.h"
#include "prost_planner.h"

#include "utils/math_utils.h"

#include <iostream>
#include <limits>

using namespace std;

void ActionState::getActions(vector<string>& result) const {
    for(unsigned int i = 0; i < scheduledActionFluents.size(); ++i) {
        result.push_back(scheduledActionFluents[i]->name);
    }
}


void ActionState::calculateProperties(vector<ActionFluent*>& actionFluents) {
    for(unsigned int i = 0; i < state.size(); ++i) {
        if(state[i]) {
            scheduledActionFluents.push_back(actionFluents[i]);
        }
    }
}
