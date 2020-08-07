#include "states.h"

#include "evaluatables.h"
#include "rddl.h"

#include <sstream>

using namespace std;

namespace prost {
namespace parser {
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

void ActionState::print(ostream& out) const {
    for (unsigned int index = 0; index < state.size(); ++index) {
        out << state[index] << " ";
    }
    out << endl;
}
} // namespace parser
} // namespace prost
