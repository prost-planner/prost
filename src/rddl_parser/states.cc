#include "states.h"

#include "evaluatables.h"
#include "rddl.h"

#include <sstream>

using namespace std;

State::State(vector<ConditionalProbabilityFunction*> const& cpfs) {
    for (unsigned int i = 0; i < cpfs.size(); ++i) {
        state.push_back(cpfs[i]->getInitialValue());
    }
}

void State::print(ostream& out) const {
    for (unsigned int index = 0; index < state.size(); ++index) {
        out << state[index] << " ";
    }
    out << endl;
}

bool ActionState::isNOOP(RDDLTask* task) const {
    for (ActionFluent* af : task->actionFluents) {
        if (state[af->index] != 0) {
            return false;
        }
        vector<Object*> values = af->valueType->objects;
        if ((values.size() > 2) && values[0]->name != "none-of-those") {
            return false;
        }
    }
    return true;
}

bool ActionState::sharesActiveActionFluent(
    set<ActionFluent*> const& actionFluents) const {
    for (ActionFluent const* af : actionFluents) {
        if (state[af->index]) {
            return true;
        }
    }
    return false;
}

string ActionState::getName() const {
    if (scheduledActionFluents.empty()) {
        return "noop";
    }
    stringstream name;
    for (unsigned int i = 0; i < scheduledActionFluents.size(); ++i) {
        name << scheduledActionFluents[i]->fullName << " ";
    }
    return name.str();
}

void ActionState::print(ostream& out) const {
    for (unsigned int index = 0; index < state.size(); ++index) {
        out << state[index] << " ";
    }
    out << endl;
}
