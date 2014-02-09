#ifndef KLEENE_STATE_H
#define KLEENE_STATE_H

#include "state.h"

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
