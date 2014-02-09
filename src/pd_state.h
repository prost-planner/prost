#ifndef PD_STATE_H
#define PD_STATE_H

#include <vector>

#include "probability_distribution.h"

class PDState {
public:
    PDState() {} //TODO: This constructor is ugly and unsafe, but currently needed.
    PDState(std::vector<DiscretePD> _state, int const& _remSteps) :
        state(_state), remSteps(_remSteps) {}
    PDState(PDState const& other) :
        state(other.state), remSteps(other.remSteps) {}
    PDState(int const& size, int const& _remSteps) :
        state(size, DiscretePD()), remSteps(_remSteps) {}

    DiscretePD& operator[](int const& index) {
        assert(index < state.size());
        return state[index];
    }

    DiscretePD const& operator[](int const& index) const {
        assert(index < state.size());
        return state[index];
    }

    int const& remainingSteps() const {
        return remSteps;
    }

    int& remainingSteps() {
        return remSteps;
    }

    void reset(int _remSteps) {
        for(unsigned int i = 0; i < state.size(); ++i) {
            state[i].reset();
        }
        remSteps = _remSteps;
    }

    void transferDeterministicPart(State& detState) {
        assert(remSteps == detState.remainingSteps());
        assert(detState.state.size() == state.size());

        for(unsigned int i = 0; i < state.size(); ++i) {
            if(state[i].isDeterministic()) {
                detState.state[i] = state[i].values[0];
            }
        }
    }

    struct CompareIgnoringRemainingSteps {
        bool operator() (PDState const& lhs, PDState const& rhs) const {
            assert(lhs.state.size() == rhs.state.size());

            for(unsigned int i = 0; i < lhs.state.size(); ++i) {
                if(rhs.state[i] < lhs.state[i]) {
                    return false;
                } else if(lhs.state[i] < rhs.state[i]) {
                    return true;
                }
            }
            return false;
        }
    };

    //TODO: This is very very ugly, but cpfs, planning tasks and
    //states are very tightly coupled. Nevertheless, there must be a
    //way to get rid of this, even if it takes some work!
    friend class PlanningTask;

protected:
    std::vector<DiscretePD> state;
    int remSteps;
};


#endif
