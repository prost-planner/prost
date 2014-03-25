#ifndef STATES_H
#define STATES_H

#include <vector>
#include <set>
#include <cassert>

#include "utils/math_utils.h"

class ActionFluent;
class ActionPrecondition;
class ConditionalProbabilityFunction;

/*****************************************************************
                               State
*****************************************************************/

struct State {
    State(std::vector<ConditionalProbabilityFunction*> const& cpfs);

    double& operator[](int const& index) {
        assert(index < state.size());
        return state[index];
    }

    double const& operator[](int const& index) const {
        assert(index < state.size());
        return state[index];
    }

    std::vector<double> state;
};

/*****************************************************************
                            ActionState
*****************************************************************/

class ActionState {
public:
    ActionState(int size) :
        state(size, 0), index(-1) {}

    // This is used to sort action states by the number of true fluents and the
    // position of the true fluents to ensure deterministic behaviour
    struct ActionStateSort {
        bool operator() (ActionState const& lhs, ActionState const& rhs) const {
            int lhsNum = 0;
            int rhsNum = 0;
            for(unsigned int i = 0; i < lhs.state.size(); ++i) {
                lhsNum += lhs.state[i];
                rhsNum += rhs.state[i];
            }

            if(lhsNum < rhsNum) {
                return true;
            } else if(rhsNum < lhsNum) {
                return false;
            }

            return (lhs.state < rhs.state);
        }
    };

    int& operator[](int const& index) {
        return state[index];
    }

    const int& operator[](int const& index) const {
        return state[index];
    }

    std::vector<int> state;
    std::vector<ActionFluent*> scheduledActionFluents;
    std::vector<ActionPrecondition*> relevantSACs;
    int index;
};

#endif
