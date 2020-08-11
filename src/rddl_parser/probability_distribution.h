#ifndef PARSER_PROBABILITY_DISTRIBUTION_H
#define PARSER_PROBABILITY_DISTRIBUTION_H

// For now, we only consider discrete probability distributions, which will be
// used for the RDDL KronDelta, Bernoulli and Discrete statements (TODO: maybe
// it is more efficient to distinguish these by using different classes.)

#include <iostream>
#include <map>
#include <vector>

namespace prost::parser {
class DiscretePD {
public:
    DiscretePD() {}

    bool operator==(DiscretePD const& rhs) const;
    bool operator<(DiscretePD const& rhs) const;

    // Places all probability mass on val
    void assignDiracDelta(double const& val) {
        reset();
        values.push_back(val);
        probabilities.push_back(1.0);
    }

    // Places truthProb on 1.0 and the rest on 0.0
    void assignBernoulli(double const& truthProb);

    // We use a map here as this makes sure that the values are sorted
    void assignDiscrete(std::map<double, double> const& valProbPairs);

    void reset() {
        values.clear();
        probabilities.clear();
    }

    double probabilityOf(double const& val) const;

    double falsityProbability() const {
        return probabilityOf(0.0);
    }

    bool isFalsity() const;

    double truthProbability() const {
        return 1.0 - falsityProbability();
    }

    bool isTruth() const;

    bool isDeterministic() const {
        return values.size() == 1;
    }

    bool isUndefined() const {
        return values.empty();
    }

    int getNumberOfOutcomes() const;

    int size() const {
        return values.size();
    }

    bool isWellDefined() const;
    void print(std::ostream& out) const;

    std::vector<double> values;
    std::vector<double> probabilities;
};
} // namespace prost::parser

#endif // PARSER_PROBABILITY_DISTRIBUTION_H
