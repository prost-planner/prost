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

void State::print(ostream& out) const {
    for (unsigned int index = 0; index < state.size(); ++index) {
        out << state[index] << " ";
    }
    out << endl;
}

void ActionState::print(ostream& out) const {
    for (unsigned int index = 0; index < state.size(); ++index) {
        out << state[index] << " ";
    }
    out << endl;
}
} // namespace prost::parser
