#ifndef PROBABILITY_DISTRIBUTION_H
#define PROBABILITY_DISTRIBUTION_H

// For now, we only consider discrete probability distributions, which will be
// used for the RDDL KronDelta, Bernoulli and Discrete statements (TODO: maybe
// it is more efficient to distinguish these by using different classes.)

#include <iostream>
#include <map>

#include <random>

#include "utils/math_utils.h"

class DiscretePD {
public:
    DiscretePD() {}

    bool operator==(DiscretePD const& rhs) const {
        if (values.size() != rhs.values.size()) {
            return false;
        }

        for (unsigned int i = 0; i < values.size(); ++i) {
            if (!MathUtils::doubleIsEqual(probabilities[i],
                                          rhs.probabilities[i]) ||
                !MathUtils::doubleIsEqual(values[i], rhs.values[i])) {
                return false;
            }
        }
        return true;
    }

    bool operator<(DiscretePD const& rhs) const {
        if (values.size() < rhs.values.size()) {
            return true;
        } else if (rhs.values.size() < values.size()) {
            return false;
        }

        for (unsigned int i = 0; i < values.size(); ++i) {
            if (MathUtils::doubleIsSmaller(values[i], rhs.values[i])) {
                return true;
            } else if (MathUtils::doubleIsSmaller(rhs.values[i], values[i])) {
                return false;
            }

            if (MathUtils::doubleIsSmaller(probabilities[i],
                                           rhs.probabilities[i])) {
                return true;
            } else if (MathUtils::doubleIsSmaller(rhs.probabilities[i],
                                                  probabilities[i])) {
                return false;
            }
        }
        return false;
    }

    // Places all probability mass on val
    void assignDiracDelta(double const& val) {
        reset();
        values.push_back(val);
        probabilities.push_back(1.0);
    }

    // Places truthProb on 1.0 and the rest on 0.0
    void assignBernoulli(double const& truthProb) {
        reset();
        if (!MathUtils::doubleIsEqual(truthProb, 1.0)) {
            values.push_back(0.0);
            probabilities.push_back(1.0 - truthProb);
        }
        if (!MathUtils::doubleIsEqual(truthProb, 0.0)) {
            values.push_back(1.0);
            probabilities.push_back(truthProb);
        }
    }

    // We use a map here as this makes sure that the values are sorted
    void assignDiscrete(std::map<double, double> const& valProbPairs) {
        reset();
        for (std::map<double, double>::const_iterator it = valProbPairs.begin();
             it != valProbPairs.end(); ++it) {
            values.push_back(it->first);
            probabilities.push_back(it->second);
        }
    }

    void reset() {
        values.clear();
        probabilities.clear();
    }

    double probabilityOf(double const& val) const {
        for (unsigned int i = 0; i < values.size(); ++i) {
            if (MathUtils::doubleIsEqual(values[i], val)) {
                return probabilities[i];
            } else if (MathUtils::doubleIsGreater(values[i], val)) {
                // As
                return 0.0;
            }
        }
        return 0.0;
    }

    double falsityProbability() const {
        return probabilityOf(0.0);
    }

    bool isFalsity() const {
        return isDeterministic() && MathUtils::doubleIsEqual(values[0], 0.0);
    }

    double truthProbability() const {
        return 1.0 - falsityProbability();
    }

    bool isTruth() const {
        return isDeterministic() &&
               (MathUtils::doubleIsGreaterOrEqual(values[0], 1.0) ||
                MathUtils::doubleIsSmaller(values[0], 0.0));
    }

    bool isDeterministic() const {
        return values.size() == 1;
    }

    bool isUndefined() const {
        return values.empty();
    }

    int getNumberOfOutcomes() const {
        assert(isWellDefined());
        return values.size();
    }

    int size() const {
        return values.size();
    }

    bool isWellDefined() const;
    void print(std::ostream& out) const;

    // Sample a value which is not blacklisted. Probability of blackisted values
    // is ignored
    std::pair<double, double> sample(std::vector<int> const& blacklist = {}) const;

    std::vector<double> values;
    std::vector<double> probabilities;
};

#endif
