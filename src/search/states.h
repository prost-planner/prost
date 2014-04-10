#ifndef STATES_H
#define STATES_H

#include <vector>
#include <set>
#include <cassert>

#include "probability_distribution.h"

#include "utils/math_utils.h"

class ActionFluent;
class Evaluatable;

/*****************************************************************
                               STATE
*****************************************************************/

class State {
public:
/*****************************************************************
                State creation and adoption
*****************************************************************/

    State(int const& _remSteps = -1) :
        state(State::stateSize, 0.0), remSteps(_remSteps), stateFluentHashKeys(State::numberOfStateFluentHashKeys, 0), hashKey(-1) {}

    State(std::vector<double> _state, int const& _remSteps) :
        state(_state), remSteps(_remSteps), stateFluentHashKeys(State::numberOfStateFluentHashKeys, 0), hashKey(-1) {
        assert(state.size() == State::stateSize);
    }

    State(State const& other) :
        state(other.state), remSteps(other.remSteps), stateFluentHashKeys(other.stateFluentHashKeys), hashKey(other.hashKey) {}

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

/*****************************************************************
        Calculation of state and state fluent hash keys
*****************************************************************/

    // Calculate the hash key of a State
    static void calcStateHashKey(State& state) {
        if(State::stateHashingPossible) {
            state.hashKey = 0;
            for(unsigned int index = 0; index < State::stateSize; ++index) {
                state.hashKey += State::stateHashKeys[index][(int)state[index]];
            }
        } else {
            assert(state.hashKey == -1);
        }
    }

    // Calculate the hash key for each state fluent in a State
    static void calcStateFluentHashKeys(State& state) {
        for(unsigned int i = 0; i < State::stateSize; ++i) {
            if(MathUtils::doubleIsGreater(state[i], 0.0)) {
                for(unsigned int j = 0; j < State::indexToStateFluentHashKeyMap[i].size(); ++j) {
                    assert(state.stateFluentHashKeys.size() > State::indexToStateFluentHashKeyMap[i][j].first);
                    state.stateFluentHashKeys[State::indexToStateFluentHashKeyMap[i][j].first] +=
                        ((int)state[i]) * State::indexToStateFluentHashKeyMap[i][j].second;
                    //state.stateFluentHashKeys[indexToStateFluentHashKeyMap[i][j].first] += indexToStateFluentHashKeyMap[i][j].second[(int)state[i]];
                }                
            }
        }
    }

    // Calculate the hash key of a PDState
    // void calcPDStateHashKey(State& /*state*/) const {
    //     REPAIR AND MOVE TO PDSTATE
    // }

    // Calculate the hash key for each state fluent in a PDState
    // void calcPDStateFluentHashKeys(PDState& state) const {
    //     REPAIR AND MOVE TO PDSTATE
    // }

/*****************************************************************
               Get and modify member variables
*****************************************************************/

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

/*****************************************************************
                       State comparison
*****************************************************************/

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

/*****************************************************************
                              Printer
*****************************************************************/

    void printCompact(std::ostream& out) const;
    void print(std::ostream& out) const;

/*****************************************************************
                    Static member variables
*****************************************************************/

    // The number of state fluents (this is equal to CPFs.size())
    static int stateSize;

    // The number of variables that have a state fluent hash key
    static int numberOfStateFluentHashKeys;

    // Is true if hashing of states (not state fluent hashing) is possible
    static bool stateHashingPossible;

    // These are used to calculate hash keys
    static std::vector<std::vector<long> > stateHashKeys;

    // The Evaluatable with index indexToStateFluentHashKeyMap[i][j].first
    // depends on the CPF with index i, and is updated by multiplication with
    // indexToStateFluentHashKeyMap[i][j].second
    static std::vector<std::vector<std::pair<int, long> > > indexToStateFluentHashKeyMap;

    friend class KleeneState;
    friend class PDState;

private:
/*****************************************************************
                     Member variables
*****************************************************************/

    std::vector<double> state;
    int remSteps;
    std::vector<long> stateFluentHashKeys;
    long hashKey;
};

/*****************************************************************
                           ACTION STATE
*****************************************************************/

struct ActionState {
    ActionState(int _index, std::vector<int> _state, std::vector<ActionFluent*> _scheduledActionFluents, std::vector<Evaluatable*> _actionPreconditions) :
        index(_index), state(_state), scheduledActionFluents(_scheduledActionFluents), actionPreconditions(_actionPreconditions) {}

/*****************************************************************
               Get and modify member variables
*****************************************************************/

    int& operator[](int const& index) {
        return state[index];
    }

    const int& operator[](int const& index) const {
        return state[index];
    }

/*****************************************************************
                              Printer
*****************************************************************/

    void printCompact(std::ostream& out) const;
    void print(std::ostream& out) const;

/*****************************************************************
                     Member variables
*****************************************************************/
    int index;
    std::vector<int> state;
    std::vector<ActionFluent*> scheduledActionFluents;
    std::vector<Evaluatable*> actionPreconditions;
};

/*****************************************************************
                             PD STATE
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

    void print(std::ostream& out) const;

protected:
    std::vector<DiscretePD> state;
    int remSteps;
};

/*****************************************************************
                           KLEENE STATE
*****************************************************************/

class KleeneState {
public:
/*****************************************************************
                State creation and adoption
*****************************************************************/

    KleeneState() :
        state(KleeneState::stateSize), stateFluentHashKeys(KleeneState::numberOfStateFluentHashKeys, 0), hashKey(-1) {}

    KleeneState(State const& origin) :
        state(origin.state.size()), stateFluentHashKeys(origin.stateFluentHashKeys.size(), 0), hashKey(-1) {

        for(unsigned int index = 0; index < state.size(); ++index) {
            state[index].insert(origin[index]);
        }
    }

/*****************************************************************
      Calculation of state and state fluent hash keys
*****************************************************************/

    // Calculate the hash key of a KleeneState
    static void calcStateHashKey(KleeneState& state) {
        if(KleeneState::stateHashingPossible) {
            state.hashKey = 0;
            for(unsigned int index = 0; index < KleeneState::stateSize; ++index) {
                int multiplier = 0;
                for(std::set<double>::iterator it = state[index].begin(); it != state[index].end(); ++it) {
                    multiplier |= 1<<((int)*it);
                }
                --multiplier;

                state.hashKey += (multiplier * KleeneState::hashKeyBases[index]);
            }
        } else {
            assert(state.hashKey == -1);
        }
    }

    // Calculate the hash key for each state fluent in a KleeneState
    static void calcStateFluentHashKeys(KleeneState& state) {
        for(unsigned int i = 0; i < KleeneState::stateSize; ++i) {
            int multiplier = 0;
            for(std::set<double>::iterator it = state[i].begin(); it != state[i].end(); ++it) {
                multiplier |= 1<<((int)*it);
            }
           --multiplier;
           if(multiplier > 0) {
               for(unsigned int j = 0; j < KleeneState::indexToStateFluentHashKeyMap[i].size(); ++j) {
                   assert(state.stateFluentHashKeys.size() > KleeneState::indexToStateFluentHashKeyMap[i][j].first);
                   state.stateFluentHashKeys[KleeneState::indexToStateFluentHashKeyMap[i][j].first] +=
                       (multiplier * KleeneState::indexToStateFluentHashKeyMap[i][j].second);
               }
           }
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

    void print(std::ostream& out) const;

    // The number of state fluents (this is equal to CPFs.size())
    static int stateSize;

    // The number of variables that have a state fluent hash key
    static int numberOfStateFluentHashKeys;

    // Is true if hashing of States (not state fluent hashing) is possible
    static bool stateHashingPossible;

    // These are used to calculate hash keys
    static std::vector<long> hashKeyBases;

    // The Evaluatable with index indexToStateFluentHashKeyMap[i][j].first
    // depends on the CPF with index i, and is updated by multiplication with
    // indexToStateFluentHashKeyMap[i][j].second
    static std::vector<std::vector<std::pair<int, long> > > indexToStateFluentHashKeyMap;

protected:
    std::vector<std::set<double> > state;
    std::vector<long> stateFluentHashKeys;
    long hashKey;

private:
    KleeneState(KleeneState const& other) :
        state(other.state), stateFluentHashKeys(other.stateFluentHashKeys), hashKey(other.hashKey) {}
};


#endif
