#ifndef STATES_H
#define STATES_H

#include <vector>
#include <set>
#include <cassert>

#include "probability_distribution.h"

#include "utils/math_utils.h"

class ActionFluent;
class Evaluatable;
class ConditionalProbabilityFunction;

/*****************************************************************
                               State
*****************************************************************/

class State {
public:
    State() {} //TODO: This constructor is ugly and unsafe, but currently needed.
    State(std::vector<double> _state, int const& _remSteps, int const& stateFluentHashKeySize) :
        state(_state), remSteps(_remSteps), stateFluentHashKeys(stateFluentHashKeySize,0), hashKey(-1) {}

    State(State const& other) :
        state(other.state), remSteps(other.remSteps), stateFluentHashKeys(other.stateFluentHashKeys), hashKey(other.hashKey) {}

    State(int const& size, int const& _remSteps, int const& stateFluentHashKeySize) :
        state(size, 0.0), remSteps(_remSteps), stateFluentHashKeys(stateFluentHashKeySize, 0), hashKey(-1) {}

    void setTo(State const& other) {
        for(unsigned int i = 0; i < state.size(); ++i) {
            state[i] = other.state[i];
        }
        remSteps = other.remSteps;

        for(unsigned int i = 0; i < stateFluentHashKeys.size(); ++i) {
            stateFluentHashKeys[i] = other.stateFluentHashKeys[i];
        }

        hashKey = other.hashKey;
    }

    void reset(int _remSteps) {
        for(unsigned int i = 0; i < state.size(); ++i) {
            state[i] = 0.0;
        }
        remSteps = _remSteps;

        for(unsigned int i = 0; i < stateFluentHashKeys.size(); ++i) {
            stateFluentHashKeys[i] = 0;
        }

        hashKey = -1;
    }

    void swap(State& other) {
        state.swap(other.state);
        std::swap(remSteps, other.remSteps);
        std::swap(hashKey, other.hashKey);
        stateFluentHashKeys.swap(other.stateFluentHashKeys);
    }

    double& operator[](int const& index) {
        assert(index < state.size());
        return state[index];
    }

    double const& operator[](int const& index) const {
        assert(index < state.size());
        return state[index];
    }

    int const& remainingSteps() const {
        return remSteps;
    }

    int& remainingSteps() {
        return remSteps;
    }

    bool isTerminal() const {
        assert(remSteps >= 0);
        return (remSteps == 0);
    }

    long const& stateFluentHashKey(int const& index) const {
        assert(index < stateFluentHashKeys.size());
        return stateFluentHashKeys[index];
    }

    long const& getHashKey() const {
        return hashKey;
    }

    bool isEqualIgnoringRemainingStepsTo(State const& other) const {
        assert(state.size() == other.state.size());
        if((hashKey >= 0) && (other.hashKey >= 0)) {
            return (hashKey == other.hashKey);
        }

        for(unsigned int i = 0; i < state.size(); ++i) {
            if(!MathUtils::doubleIsEqual(state[i],other.state[i])) {
                return false;
            }
        }
        return true;
    }

    struct CompareIgnoringRemainingSteps {
        bool operator() (State const& lhs, State const& rhs) const {
            assert(lhs.state.size() == rhs.state.size());
            if((lhs.hashKey >= 0) && (rhs.hashKey >= 0)) {
                return (lhs.hashKey < rhs.hashKey);
            }

            for(unsigned int i = 0; i < lhs.state.size(); ++i) {
                if(MathUtils::doubleIsSmaller(rhs.state[i],lhs.state[i])) {
                    return false;
                } else if(MathUtils::doubleIsSmaller(lhs.state[i],rhs.state[i])) {
                    return true;
                }
            }
            return false;
        }
    };

    struct CompareConsideringRemainingSteps {
        bool operator() (State const& lhs, State const& rhs) const {
            if(lhs.remSteps < rhs.remSteps) {
                return true;
            } else if(lhs.remSteps > rhs.remSteps) {
                return false;
            }

            assert(lhs.state.size() == rhs.state.size());
            if((lhs.hashKey >= 0) && (rhs.hashKey >= 0)) {
                return (lhs.hashKey < rhs.hashKey);
            }

            for(unsigned int i = 0; i < lhs.state.size(); ++i) {
                if(MathUtils::doubleIsSmaller(rhs.state[i],lhs.state[i])) {
                    return false;
                } else if(MathUtils::doubleIsSmaller(lhs.state[i],rhs.state[i])) {
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
    friend class KleeneState;
    friend class PDState;

protected:
    std::vector<double> state;
    int remSteps;
    std::vector<long> stateFluentHashKeys;
    long hashKey;
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
    std::vector<Evaluatable*> relevantSACs;
    int index;
};

/*****************************************************************
                              PDState
*****************************************************************/

class PDState {
public:
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

/*****************************************************************
                            KleeneState
*****************************************************************/

class KleeneState {
public:
    KleeneState(int const& size, int const& stateFluentHashKeySize) :
        state(size), stateFluentHashKeys(stateFluentHashKeySize, 0), hashKey(-1) {}

    KleeneState(State const& origin) :
        state(origin.state.size()), stateFluentHashKeys(origin.stateFluentHashKeys.size(), 0), hashKey(-1) {

        for(unsigned int index = 0; index < state.size(); ++index) {
            state[index].insert(origin[index]);
        }
    }

    KleeneState(KleeneState const& other) :
        state(other.state), stateFluentHashKeys(other.stateFluentHashKeys), hashKey(other.hashKey) {}

    std::set<double>& operator[](int const& index) {
        assert(index < state.size());
        return state[index];
    }

    std::set<double> const& operator[](int const& index) const {
        assert(index < state.size());
        return state[index];
    }

    long const& stateFluentHashKey(int const& index) const {
        assert(index < stateFluentHashKeys.size());
        return stateFluentHashKeys[index];
    }

    long const& getHashKey() const {
        return hashKey;
    }

    bool operator==(KleeneState const& other) const {
        assert(state.size() == other.state.size());
        if((hashKey >= 0) && other.hashKey >= 0) {
            return (hashKey == other.hashKey);
        }

        for(unsigned int index = 0; index < state.size(); ++index) {
            if(!std::equal(state[index].begin(), state[index].end(), other.state[index].begin())) {
                return false;
            }
        }
        return true;
    }

    // This is used to merge two KleeneStates
    KleeneState operator|=(KleeneState const& other) {
        assert(state.size() == other.state.size());

        for(unsigned int i = 0; i < state.size(); ++i) {
            state[i].insert(other.state[i].begin(), other.state[i].end());
        }

        hashKey = -1;
        return *this;
    }

    KleeneState const operator||(KleeneState const& other) {
        assert(state.size() == other.state.size());
        return KleeneState(*this) |= other;
    }

    //TODO: This is very very ugly, but cpfs, planning tasks and
    //states are very tightly coupled. Nevertheless, there must be a
    //way to get rid of this, even if it takes some work!
    friend class PlanningTask;

protected:
    std::vector<std::set<double> > state;
    std::vector<long> stateFluentHashKeys;
    long hashKey;
};


#endif
