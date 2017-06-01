#ifndef STATES_H
#define STATES_H

#include <cassert>
#include <set>
#include <vector>

#include "probability_distribution.h"
#include "utils/math_utils.h"

class ActionFluent;
class DeterministicEvaluatable;

/*****************************************************************
                             State
*****************************************************************/

class State {
public:
    friend class KleeneState;
    friend class PDState;

    State(int const& _remSteps = -1)
        : deterministicStateFluents(numberOfDeterministicStateFluents, 0.0),
          probabilisticStateFluents(numberOfProbabilisticStateFluents, 0.0),
          remSteps(_remSteps),
          stateFluentHashKeys(numberOfStateFluentHashKeys, 0),
          hashKey(-1) {}

    State(std::vector<double> _deterministicStateFluents,
          std::vector<double> _probabilisticStateFluents, int const& _remSteps)
        : deterministicStateFluents(_deterministicStateFluents),
          probabilisticStateFluents(_probabilisticStateFluents),
          remSteps(_remSteps),
          stateFluentHashKeys(numberOfStateFluentHashKeys, 0),
          hashKey(-1) {
        assert(deterministicStateFluents.size() ==
               numberOfDeterministicStateFluents);
        assert(probabilisticStateFluents.size() ==
               numberOfProbabilisticStateFluents);
    }

    State(std::vector<double> _stateVector, int const& _remSteps)
        : remSteps(_remSteps),
          stateFluentHashKeys(numberOfStateFluentHashKeys, 0),
          hashKey(-1) {
        for (unsigned int i = 0; i < numberOfDeterministicStateFluents; ++i) {
            deterministicStateFluents.push_back(_stateVector[i]);
        }
        for (unsigned int i = 0; i < numberOfProbabilisticStateFluents; ++i) {
            probabilisticStateFluents.push_back(
                _stateVector[i + numberOfDeterministicStateFluents]);
        }
    }

    State(State const& other)
        : deterministicStateFluents(other.deterministicStateFluents),
          probabilisticStateFluents(other.probabilisticStateFluents),
          remSteps(other.remSteps),
          stateFluentHashKeys(other.stateFluentHashKeys),
          hashKey(other.hashKey) {}

    virtual void setTo(State const& other) {
        for (unsigned int i = 0; i < numberOfDeterministicStateFluents; ++i) {
            deterministicStateFluents[i] = other.deterministicStateFluents[i];
        }
        for (unsigned int i = 0; i < numberOfProbabilisticStateFluents; ++i) {
            probabilisticStateFluents[i] = other.probabilisticStateFluents[i];
        }

        remSteps = other.remSteps;

        for (unsigned int i = 0; i < numberOfStateFluentHashKeys; ++i) {
            stateFluentHashKeys[i] = other.stateFluentHashKeys[i];
        }

        hashKey = other.hashKey;
    }

    virtual void reset(int _remSteps) {
        for (unsigned int i = 0; i < numberOfDeterministicStateFluents; ++i) {
            deterministicStateFluents[i] = 0.0;
        }
        for (unsigned int i = 0; i < numberOfProbabilisticStateFluents; ++i) {
            probabilisticStateFluents[i] = 0.0;
        }

        remSteps = _remSteps;

        for (unsigned int i = 0; i < numberOfStateFluentHashKeys; ++i) {
            stateFluentHashKeys[i] = 0;
        }

        hashKey = -1;
    }

    void swap(State& other) {
        deterministicStateFluents.swap(other.deterministicStateFluents);
        probabilisticStateFluents.swap(other.probabilisticStateFluents);

        std::swap(remSteps, other.remSteps);
        std::swap(hashKey, other.hashKey);
        stateFluentHashKeys.swap(other.stateFluentHashKeys);
    }

    // Calculate the hash key of a State
    static void calcStateHashKey(State& state) {
        if (stateHashingPossible) {
            state.hashKey = 0;
            for (unsigned int index = 0;
                 index < numberOfDeterministicStateFluents; ++index) {
                state.hashKey += stateHashKeysOfDeterministicStateFluents
                    [index][(int)state.deterministicStateFluents[index]];
            }
            for (unsigned int index = 0;
                 index < numberOfProbabilisticStateFluents; ++index) {
                state.hashKey += stateHashKeysOfProbabilisticStateFluents
                    [index][(int)state.probabilisticStateFluents[index]];
            }
        } else {
            assert(state.hashKey == -1);
        }
    }

    // Calculate the hash key for each state fluent in a State
    static void calcStateFluentHashKeys(State& state) {
        for (unsigned int i = 0; i < numberOfDeterministicStateFluents; ++i) {
            if (MathUtils::doubleIsGreater(state.deterministicStateFluents[i],
                                           0.0)) {
                for (unsigned int j = 0;
                     j <
                     stateFluentHashKeysOfDeterministicStateFluents[i].size();
                     ++j) {
                    assert(state.stateFluentHashKeys.size() >
                           stateFluentHashKeysOfDeterministicStateFluents[i][j]
                               .first);
                    state.stateFluentHashKeys
                        [stateFluentHashKeysOfDeterministicStateFluents[i][j]
                             .first] +=
                        ((int)state.deterministicStateFluents[i]) *
                        stateFluentHashKeysOfDeterministicStateFluents[i][j]
                            .second;
                }
            }
        }

        for (unsigned int i = 0; i < numberOfProbabilisticStateFluents; ++i) {
            if (MathUtils::doubleIsGreater(state.probabilisticStateFluents[i],
                                           0.0)) {
                for (unsigned int j = 0;
                     j <
                     stateFluentHashKeysOfProbabilisticStateFluents[i].size();
                     ++j) {
                    assert(state.stateFluentHashKeys.size() >
                           stateFluentHashKeysOfProbabilisticStateFluents[i][j]
                               .first);
                    state.stateFluentHashKeys
                        [stateFluentHashKeysOfProbabilisticStateFluents[i][j]
                             .first] +=
                        ((int)state.probabilisticStateFluents[i]) *
                        stateFluentHashKeysOfProbabilisticStateFluents[i][j]
                            .second;
                }
            }
        }
    }

    double& deterministicStateFluent(int const& index) {
        assert(index < deterministicStateFluents.size());
        return deterministicStateFluents[index];
    }

    double const& deterministicStateFluent(int const& index) const {
        assert(index < deterministicStateFluents.size());
        return deterministicStateFluents[index];
    }

    double& probabilisticStateFluent(int const& index) {
        assert(index < probabilisticStateFluents.size());
        return probabilisticStateFluents[index];
    }

    double const& probabilisticStateFluent(int const& index) const {
        assert(index < probabilisticStateFluents.size());
        return probabilisticStateFluents[index];
    }

    int const& stepsToGo() const {
        return remSteps;
    }

    int& stepsToGo() {
        return remSteps;
    }

    bool isTerminal() const {
        assert(remSteps >= 0);
        return remSteps == 0;
    }

    long const& stateFluentHashKey(int const& index) const {
        assert(index < stateFluentHashKeys.size());
        return stateFluentHashKeys[index];
    }

    struct CompareIgnoringStepsToGo {
        bool operator()(State const& lhs, State const& rhs) const {
            if ((lhs.hashKey >= 0) && (rhs.hashKey >= 0)) {
                return lhs.hashKey < rhs.hashKey;
            }

            for (unsigned int i = 0; i < numberOfDeterministicStateFluents;
                 ++i) {
                if (MathUtils::doubleIsSmaller(
                        rhs.deterministicStateFluents[i],
                        lhs.deterministicStateFluents[i])) {
                    return false;
                } else if (MathUtils::doubleIsSmaller(
                               lhs.deterministicStateFluents[i],
                               rhs.deterministicStateFluents[i])) {
                    return true;
                }
            }

            for (unsigned int i = 0; i < numberOfProbabilisticStateFluents;
                 ++i) {
                if (MathUtils::doubleIsSmaller(
                        rhs.probabilisticStateFluents[i],
                        lhs.probabilisticStateFluents[i])) {
                    return false;
                } else if (MathUtils::doubleIsSmaller(
                               lhs.probabilisticStateFluents[i],
                               rhs.probabilisticStateFluents[i])) {
                    return true;
                }
            }

            return false;
        }
    };

    struct HashWithRemSteps {
        // Hash function adapted from Python's hash function for tuples (and
        // found in the FastDownward code from http://www.fast-downward.org/)
        unsigned int operator()(State const& s) const {
            unsigned int hashValue = 0x345678;
            unsigned int mult = 1000003;
            for (int i = numberOfProbabilisticStateFluents - 1; i >= 0; --i) {
                hashValue =
                    (hashValue ^ ((int)s.probabilisticStateFluent(i))) * mult;
                mult += 82520 + i + i;
            }
            for (int i = numberOfDeterministicStateFluents - 1; i >= 0; --i) {
                hashValue =
                    (hashValue ^ ((int)s.deterministicStateFluent(i))) * mult;
                mult += 82520 + i + i;
            }
            hashValue = (hashValue ^ ((int)s.stepsToGo())) * mult;
            hashValue += 97531;
            return hashValue;
        }
    };

    struct EqualWithRemSteps {
        bool operator()(State const& lhs, State const& rhs) const {
            if (lhs.stepsToGo() != rhs.stepsToGo()) {
                return false;
            }

            if (stateHashingPossible) {
                return lhs.hashKey == rhs.hashKey;
            }

            for (unsigned int i = 0; i < numberOfDeterministicStateFluents;
                 ++i) {
                if (!MathUtils::doubleIsEqual(
                        lhs.deterministicStateFluents[i],
                        rhs.deterministicStateFluents[i])) {
                    return false;
                }
            }

            for (unsigned int i = 0; i < numberOfProbabilisticStateFluents;
                 ++i) {
                if (!MathUtils::doubleIsEqual(
                        lhs.probabilisticStateFluents[i],
                        rhs.probabilisticStateFluents[i])) {
                    return false;
                }
            }
            return true;
        }
    };

    struct HashWithoutRemSteps {
        // Hash function adapted from Python's hash function for tuples (and
        // found in the FastDownward code from http://www.fast-downward.org/)
        unsigned int operator()(State const& s) const {
            unsigned int hashValue = 0x345678;
            unsigned int mult = 1000003;
            for (int i = numberOfProbabilisticStateFluents - 1; i >= 0; --i) {
                hashValue =
                    (hashValue ^ ((int)s.probabilisticStateFluent(i))) * mult;
                mult += 82520 + i + i;
            }
            for (int i = numberOfDeterministicStateFluents - 1; i >= 0; --i) {
                hashValue =
                    (hashValue ^ ((int)s.deterministicStateFluent(i))) * mult;
                mult += 82520 + i + i;
            }
            hashValue += 97531;
            return hashValue;
        }
    };

    struct EqualWithoutRemSteps {
        bool operator()(State const& lhs, State const& rhs) const {
            if (stateHashingPossible) {
                return lhs.hashKey == rhs.hashKey;
            }

            for (unsigned int i = 0; i < numberOfDeterministicStateFluents;
                 ++i) {
                if (!MathUtils::doubleIsEqual(
                        lhs.deterministicStateFluents[i],
                        rhs.deterministicStateFluents[i])) {
                    return false;
                }
            }

            for (unsigned int i = 0; i < numberOfProbabilisticStateFluents;
                 ++i) {
                if (!MathUtils::doubleIsEqual(
                        lhs.probabilisticStateFluents[i],
                        rhs.probabilisticStateFluents[i])) {
                    return false;
                }
            }
            return true;
        }
    };

    void printCompact(std::ostream& out) const;
    void print(std::ostream& out) const;

    // The number of deterministic and probabilistic state fluents
    static int numberOfDeterministicStateFluents;
    static int numberOfProbabilisticStateFluents;

    // The number of variables that have a state fluent hash key
    static int numberOfStateFluentHashKeys;

    // Is true if hashing of states (not state fluent hashing) is possible
    static bool stateHashingPossible;

    // These are used to calculate hash keys
    static std::vector<std::vector<long>>
        stateHashKeysOfDeterministicStateFluents;
    static std::vector<std::vector<long>>
        stateHashKeysOfProbabilisticStateFluents;

    // The Evaluatable with index
    // stateFluentHashKeysOfDeterministicStateFluents[i][j].first depends on the
    // deterministic state fluent with index i, and is updated by multiplication
    // with stateFluentHashKeysOfDeterministicStateFluents[i][j].second
    static std::vector<std::vector<std::pair<int, long>>>
        stateFluentHashKeysOfDeterministicStateFluents;
    static std::vector<std::vector<std::pair<int, long>>>
        stateFluentHashKeysOfProbabilisticStateFluents;

private:
    std::vector<double> deterministicStateFluents;
    std::vector<double> probabilisticStateFluents;

    int remSteps;
    std::vector<long> stateFluentHashKeys;
    long hashKey;
};

/*****************************************************************
  ActionState
 *****************************************************************/

struct ActionState {
    ActionState(int _index, std::vector<int> _state,
                std::vector<ActionFluent*> _scheduledActionFluents,
                std::vector<DeterministicEvaluatable*> _actionPreconditions)
        : index(_index),
          state(_state),
          scheduledActionFluents(_scheduledActionFluents),
          actionPreconditions(_actionPreconditions) {}

    int& operator[](int const& index) {
        return state[index];
    }

    const int& operator[](int const& index) const {
        return state[index];
    }

    void printCompact(std::ostream& out) const;
    void print(std::ostream& out) const;

    int index;
    std::vector<int> state;
    std::vector<ActionFluent*> scheduledActionFluents;
    std::vector<DeterministicEvaluatable*> actionPreconditions;
};

/*****************************************************************
  PDState
 *****************************************************************/

class PDState : public State {
public:
    PDState(int const& _remSteps = -1)
        : State(_remSteps),
          probabilisticStateFluentsAsPD(numberOfProbabilisticStateFluents,
                                        DiscretePD()) {}

    PDState(State const& origin)
        : State(origin),
          probabilisticStateFluentsAsPD(numberOfProbabilisticStateFluents,
                                        DiscretePD()) {}

    DiscretePD& probabilisticStateFluentAsPD(int index) {
        assert(index < probabilisticStateFluentsAsPD.size());
        return probabilisticStateFluentsAsPD[index];
    }

    DiscretePD const& probabilisticStateFluentAsPD(int index) const {
        assert(index < probabilisticStateFluentsAsPD.size());
        return probabilisticStateFluentsAsPD[index];
    }

    void reset(int _remSteps) {
        State::reset(_remSteps);

        for (unsigned int i = 0; i < numberOfProbabilisticStateFluents; ++i) {
            probabilisticStateFluentsAsPD[i].reset();
        }
    }

    void setTo(PDState const& other) {
        State::setTo(other);

        for (unsigned int i = 0; i < numberOfProbabilisticStateFluents; ++i) {
            probabilisticStateFluentsAsPD[i] =
                other.probabilisticStateFluentsAsPD[i];
        }
    }

    std::pair<double, double> sample(int varIndex,
                                     std::vector<int> const& blacklist = {}) {
        DiscretePD& pd = probabilisticStateFluentsAsPD[varIndex];
        std::pair<double, double> outcome = pd.sample(blacklist);
        probabilisticStateFluent(varIndex) = outcome.first;
        return outcome;
    }

    // Remaining steps are not considered here!
    struct PDStateCompare {
        bool operator()(PDState const& lhs, PDState const& rhs) const {
            for (unsigned int i = 0; i < numberOfDeterministicStateFluents;
                 ++i) {
                if (MathUtils::doubleIsSmaller(
                        rhs.deterministicStateFluents[i],
                        lhs.deterministicStateFluents[i])) {
                    return false;
                } else if (MathUtils::doubleIsSmaller(
                               lhs.deterministicStateFluents[i],
                               rhs.deterministicStateFluents[i])) {
                    return true;
                }
            }

            for (unsigned int i = 0; i < numberOfProbabilisticStateFluents;
                 ++i) {
                if (rhs.probabilisticStateFluentsAsPD[i] <
                    lhs.probabilisticStateFluentsAsPD[i]) {
                    return false;
                } else if (lhs.probabilisticStateFluentsAsPD[i] <
                           rhs.probabilisticStateFluentsAsPD[i]) {
                    return true;
                }
            }
            return false;
        }
    };

    void printPDState(std::ostream& out) const;
    void printPDStateCompact(std::ostream& out) const;

protected:
    std::vector<DiscretePD> probabilisticStateFluentsAsPD;
};

/*****************************************************************
  KleeneState
 *****************************************************************/

class KleeneState {
public:
    KleeneState()
        : state(stateSize),
          stateFluentHashKeys(numberOfStateFluentHashKeys, 0),
          hashKey(-1) {}

    KleeneState(State const& origin)
        : state(stateSize),
          stateFluentHashKeys(numberOfStateFluentHashKeys, 0),
          hashKey(-1) {
        for (unsigned int index = 0;
             index < State::numberOfDeterministicStateFluents; ++index) {
            state[index].insert(origin.deterministicStateFluents[index]);
        }

        for (unsigned int index = 0;
             index < State::numberOfProbabilisticStateFluents; ++index) {
            state[State::numberOfDeterministicStateFluents + index].insert(
                origin.probabilisticStateFluents[index]);
        }
    }

    // Calculate the hash key of a KleeneState
    static void calcStateHashKey(KleeneState& state) {
        if (stateHashingPossible) {
            state.hashKey = 0;
            for (unsigned int index = 0; index < stateSize; ++index) {
                int multiplier = 0;
                for (std::set<double>::iterator it = state[index].begin();
                     it != state[index].end(); ++it) {
                    multiplier |= 1 << ((int)*it);
                }
                --multiplier;

                state.hashKey += (multiplier * hashKeyBases[index]);
            }
        } else {
            assert(state.hashKey == -1);
        }
    }

    // Calculate the hash key for each state fluent in a KleeneState
    static void calcStateFluentHashKeys(KleeneState& state) {
        for (unsigned int i = 0; i < stateSize; ++i) {
            int multiplier = 0;
            for (std::set<double>::iterator it = state[i].begin();
                 it != state[i].end(); ++it) {
                multiplier |= 1 << ((int)*it);
            }
            --multiplier;
            if (multiplier > 0) {
                for (unsigned int j = 0;
                     j < indexToStateFluentHashKeyMap[i].size(); ++j) {
                    assert(state.stateFluentHashKeys.size() >
                           indexToStateFluentHashKeyMap[i][j].first);
                    state.stateFluentHashKeys[indexToStateFluentHashKeyMap[i][j]
                                                  .first] +=
                        (multiplier *
                         indexToStateFluentHashKeyMap[i][j].second);
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

    bool operator==(KleeneState const& other) const {
        assert(state.size() == other.state.size());
        if ((hashKey >= 0) && other.hashKey >= 0) {
            return hashKey == other.hashKey;
        }

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

        hashKey = -1;
        return *this;
    }

    KleeneState const operator||(KleeneState const& other) {
        assert(state.size() == other.state.size());
        return KleeneState(*this) |= other;
    }

    void print(std::ostream& out) const;

    // The number of state fluents
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
    static std::vector<std::vector<std::pair<int, long>>>
        indexToStateFluentHashKeyMap;

protected:
    std::vector<std::set<double>> state;
    std::vector<long> stateFluentHashKeys;
    long hashKey;

private:
    KleeneState(KleeneState const& other)
        : state(other.state),
          stateFluentHashKeys(other.stateFluentHashKeys),
          hashKey(other.hashKey) {}
};

#endif
