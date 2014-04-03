#include "states.h"

#include "evaluatables.h"

#include <sstream>

using namespace std;

State::State(vector<ConditionalProbabilityFunction*> const& cpfs) {
    for(unsigned int i = 0; i < cpfs.size(); ++i) {
        state.push_back(cpfs[i]->getInitialValue());
    }
}

string ActionState::getName() const {
    if(scheduledActionFluents.empty()) {
        return "noop";
    }
    stringstream name;
    for(unsigned int i = 0; i < scheduledActionFluents.size(); ++i) {
        name << scheduledActionFluents[i]->fullName << " ";
    }
    return name.str();
}
