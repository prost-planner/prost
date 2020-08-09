#include "states.h"

#include "evaluatables.h"
#include "rddl.h"

#include <sstream>

using namespace std;

namespace prost {
namespace parser {
State::State(vector<ConditionalProbabilityFunction*> const& cpfs) {
    for (ConditionalProbabilityFunction const* cpf : cpfs) {
        state.push_back(cpf->getInitialValue());
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
