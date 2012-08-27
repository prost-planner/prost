#ifndef STATE_H
#define STATE_H

#include <map>
#include <set>
#include <vector>
#include <cassert>
#include <iostream>

#include "utils/math_utils.h"

class PlanningTask;

class State {
public:
    State() {} //TODO: This constructor is ugly and unsafe, but currently needed.
    State(std::vector<double> _state, int _remSteps) :
        state(_state), remSteps(_remSteps), stateFluentHashKeys(state.size()+1,0), hashKey(-1) {}

    State(State const& other) :
        state(other.state), remSteps(other.remSteps), stateFluentHashKeys(other.stateFluentHashKeys), hashKey(other.hashKey) {}

    State(int size) :
        state(size,0.0), remSteps(-1), stateFluentHashKeys(size+1,0), hashKey(-1) {}

    State(int size, int _remSteps) :
        state(size,0.0), remSteps(_remSteps), stateFluentHashKeys(size+1,0), hashKey(-1) {}

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

    bool isFinalState() const {
        assert(remSteps >= 0);
        return (remSteps == 0);
    }

    long const& stateFluentHashKey(int const& index) const {
        assert(index < stateFluentHashKeys.size());
        return stateFluentHashKeys[index];
    }

    long const& getHashKey() {
        return hashKey;
    }

    bool isEqualIgnoringRemainingStepsTo(State const& other) {
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

protected:
    std::vector<double> state;
    int remSteps;
    std::vector<long> stateFluentHashKeys;
    long hashKey;
};

#endif
