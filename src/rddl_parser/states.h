#ifndef STATES_H
#define STATES_H

#include <cassert>
#include <set>
#include <string>
#include <vector>

#include "probability_distribution.h"
#include "utils/math_utils.h"

class ActionFluent;
class ActionPrecondition;
class ConditionalProbabilityFunction;

/*****************************************************************
                               State
*****************************************************************/

struct State {
    State(std::vector<ConditionalProbabilityFunction*> const& cpfs);
    State(State const& other) : state(other.state) {}
    State(int stateSize) : state(stateSize, 0.0) {}

    double& operator[](int const& index) {
        assert(index < state.size());
        return state[index];
    }

    double const& operator[](int const& index) const {
        assert(index < state.size());
        return state[index];
    }

    void print(std::ostream& out) const;

    struct StateSort {
        bool operator()(State const& lhs, State const& rhs) const {
            assert(lhs.state.size() == rhs.state.size());

            for (int i = lhs.state.size() - 1; i >= 0; --i) {
                if (MathUtils::doubleIsSmaller(lhs.state[i], rhs.state[i])) {
                    return true;
                } else if (MathUtils::doubleIsSmaller(rhs.state[i],
                                                      lhs.state[i])) {
                    return false;
                }
            }
            return false;
        }
    };

    std::vector<double> state;
};

/*****************************************************************
                            PDState
*****************************************************************/

struct PDState {
    PDState(int stateSize) : state(stateSize, DiscretePD()) {}

    DiscretePD& operator[](int const& index) {
        assert(index < state.size());
        return state[index];
    }

    DiscretePD const& operator[](int const& index) const {
        assert(index < state.size());
        return state[index];
    }

    struct PDStateSort {
        bool operator()(PDState const& lhs, PDState const& rhs) const {
            for (unsigned int i = 0; i < lhs.state.size(); ++i) {
                if (rhs.state[i] < lhs.state[i]) {
                    return false;
                } else if (lhs.state[i] < rhs.state[i]) {
                    return true;
                }
            }
            return false;
        }
    };

    std::vector<DiscretePD> state;
};

/*****************************************************************
                          KleeneState
*****************************************************************/

class KleeneState {
public:
    KleeneState(int stateSize) : state(stateSize) {}

    KleeneState(State const& origin) : state(origin.state.size()) {
        for (unsigned int index = 0; index < state.size(); ++index) {
            state[index].insert(origin[index]);
        }
    }

    std::set<double>& operator[](int const& index) {
        assert(index < state.size());
        return state[index];
    }

    std::set<double> const& operator[](int const& index) const {
        assert(index < state.size());
        return state[index];
    }

    bool operator==(KleeneState const& other) const {
        assert(state.size() == other.state.size());

        for (unsigned int index = 0; index < state.size(); ++index) {
            if (!std::equal(state[index].begin(), state[index].end(),
                            other.state[index].begin())) {
                return false;
            }
        }
        return true;
    }

    // This is used to merge two KleeneStates
    KleeneState operator|=(KleeneState const& other) {
        assert(state.size() == other.state.size());

        for (unsigned int i = 0; i < state.size(); ++i) {
            state[i].insert(other.state[i].begin(), other.state[i].end());
        }
        return *this;
    }

    KleeneState const operator||(KleeneState const& other) {
        assert(state.size() == other.state.size());
        return KleeneState(*this) |= other;
    }

    void print(std::ostream& out) const;

protected:
    std::vector<std::set<double>> state;

private:
    KleeneState(KleeneState const& other) : state(other.state) {}
};

/*****************************************************************
                            ActionState
*****************************************************************/

class ActionState {
public:
    ActionState(int size) : state(size, 0), index(-1) {}

    ActionState(ActionState const& other)
        : state(other.state), index(other.index) {}

    // This is used to sort action states by the number of true fluents and the
    // position of the true fluents to ensure deterministic behaviour
    struct ActionStateSort {
        bool operator()(ActionState const& lhs, ActionState const& rhs) const {
            int lhsNum = 0;
            int rhsNum = 0;
            for (unsigned int i = 0; i < lhs.state.size(); ++i) {
                lhsNum += lhs.state[i];
                rhsNum += rhs.state[i];
            }

            if (lhsNum < rhsNum) {
                return true;
            } else if (rhsNum < lhsNum) {
                return false;
            }

            return lhs.state < rhs.state;
        }
    };

    int& operator[](int const& index) {
        return state[index];
    }

    const int& operator[](int const& index) const {
        return state[index];
    }

    bool operator<(ActionState const& other) const {
        if (state.size() < other.state.size()) {
            return true;
        } else if (state.size() > other.state.size()) {
            return false;
        }

        for (unsigned int i = 0; i < state.size(); ++i) {
            if (state[i] < other.state[i]) {
                return true;
            } else if (state[i] > other.state[i]) {
                return false;
            }
        }

        return false;
    }

    void print(std::ostream& out) const;

    std::string getName() const;

    std::vector<int> state;
    std::vector<ActionFluent*> scheduledActionFluents;
    std::vector<ActionPrecondition*> relevantSACs;
    int index;
};

#endif
