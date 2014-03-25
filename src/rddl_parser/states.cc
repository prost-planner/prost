#include "states.h"

#include "evaluatables.h"

using namespace std;

State::State(vector<ConditionalProbabilityFunction*> const& cpfs) {
    for(unsigned int i = 0; i < cpfs.size(); ++i) {
        state.push_back(cpfs[i]->getInitialValue());
    }
}
