#include "states.h"

#include "evaluatables.h"
#include "rddl.h"

#include "utils/math.h"

#include <sstream>

using namespace std;

namespace prost::parser {
State::State(vector<ConditionalProbabilityFunction*> const& cpfs) {
    for (ConditionalProbabilityFunction const* cpf : cpfs) {
        state.push_back(cpf->getInitialValue());
    }
}

bool State::StateSort::operator()(State const& lhs, State const& rhs) const {
    assert(lhs.state.size() == rhs.state.size());

    for (int i = lhs.state.size() - 1; i >= 0; --i) {
        if (utils::doubleIsSmaller(lhs.state[i], rhs.state[i])) {
            return true;
        } else if (utils::doubleIsSmaller(rhs.state[i], lhs.state[i])) {
            return false;
        }
    }
    return false;
}

string State::toString(RDDLTask* task) const {
    stringstream ss;
    for (size_t i = 0; i < state.size(); ++i) {
        ss << task->stateFluents[i]->fullName << ": " << state[i] << endl;
    }
    return ss.str();
}

vector<string> ActionState::getScheduledActionFluentNames(
    RDDLTask* task) const {
    vector<string> varNames;
    for (size_t i = 0; i < state.size(); ++i) {
        int valueIndex = state[i];
        if (!task->actionFluents[i]->isFDR) {
            if (valueIndex) {
                varNames.push_back(task->actionFluents[i]->fullName);
            }
        } else if (task->actionFluents[i]->valueType->objects[valueIndex]->name
                   != "none-of-those") {
            varNames.push_back(
                task->actionFluents[i]->valueType->objects[valueIndex]->name);
        }
    }
    return varNames;
}

string ActionState::toString(RDDLTask* task) const {
    vector<string> varNames = getScheduledActionFluentNames(task);
    if (varNames.empty()) {
        return "noop()";
    }
    stringstream ss;
    for (string const& varName : varNames) {
        ss << varName << " ";
    }
    return ss.str();
}
} // namespace prost::parser
