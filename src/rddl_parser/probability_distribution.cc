#include "probability_distribution.h"

#include "utils/math.h"

#include <cassert>

using namespace std;

namespace prost::parser {
bool DiscretePD::operator==(DiscretePD const& rhs) const {
    if (values.size() != rhs.values.size()) {
        return false;
    }

    for (unsigned int i = 0; i < values.size(); ++i) {
        if (!utils::doubleIsEqual(probabilities[i], rhs.probabilities[i]) ||
            !utils::doubleIsEqual(values[i], rhs.values[i])) {
            return false;
        }
    }
    return true;
}

bool DiscretePD::operator<(DiscretePD const& rhs) const {
    if (values.size() < rhs.values.size()) {
        return true;
    } else if (rhs.values.size() < values.size()) {
        return false;
    }

    for (unsigned int i = 0; i < values.size(); ++i) {
        if (utils::doubleIsSmaller(values[i], rhs.values[i])) {
            return true;
        } else if (utils::doubleIsSmaller(rhs.values[i], values[i])) {
            return false;
        }

        if (utils::doubleIsSmaller(probabilities[i],
                                   rhs.probabilities[i])) {
            return true;
        } else if (utils::doubleIsSmaller(rhs.probabilities[i],
                                          probabilities[i])) {
            return false;
        }
    }
    return false;
}

void DiscretePD::assignBernoulli(double const& truthProb) {
    assert(utils::doubleIsGreaterOrEqual(truthProb, 0));
    assert(utils::doubleIsSmallerOrEqual(truthProb, 1));
    reset();
    if (!utils::doubleIsEqual(truthProb, 1.0)) {
        values.push_back(0.0);
        probabilities.push_back(1.0 - truthProb);
    }
    if (!utils::doubleIsEqual(truthProb, 0.0)) {
        values.push_back(1.0);
        probabilities.push_back(truthProb);
    }
}

void DiscretePD::assignDiscrete(std::map<double, double> const& valProbPairs) {
    reset();
    for (std::map<double, double>::const_iterator it = valProbPairs.begin();
         it != valProbPairs.end(); ++it) {
        values.push_back(it->first);
        probabilities.push_back(it->second);
    }
}

double DiscretePD::probabilityOf(double const& val) const {
    for (unsigned int i = 0; i < values.size(); ++i) {
        if (utils::doubleIsEqual(values[i], val)) {
            return probabilities[i];
        } else if (utils::doubleIsGreater(values[i], val)) {
            // As
            return 0.0;
        }
    }
    return 0.0;
}

bool DiscretePD::isFalsity() const {
    return isDeterministic() && utils::doubleIsEqual(values[0], 0.0);
}

bool DiscretePD::isTruth() const {
    return isDeterministic() &&
           (utils::doubleIsGreaterOrEqual(values[0], 1.0) ||
            utils::doubleIsSmaller(values[0], 0.0));
}

inline int DiscretePD::getNumberOfOutcomes() const {
    assert(isWellDefined());
    return values.size();
}

bool DiscretePD::isWellDefined() const {
    // Only use this function in assertions, it's quite inefficient!

    // Each value must have a probability
    if (isUndefined() || (values.size() != probabilities.size())) {
        return false;
    }

    // The sum of the probabilities must be 1, each individual probability must
    // not be <= 0.0 and the values must be ordered and unique
    double probSum = probabilities[0];
    double lastVal = values[0];
    for (unsigned int i = 1; i < probabilities.size(); ++i) {
        if (utils::doubleIsSmallerOrEqual(probabilities[i], 0.0)) {
            return false;
        }
        probSum += probabilities[i];
        if (!utils::doubleIsGreater(values[i], lastVal)) {
            return false;
        }
        lastVal = values[i];
    }
    return utils::doubleIsEqual(probSum, 1.0);
}

void DiscretePD::print(ostream& out) const {
    out << "[ ";
    for (unsigned int i = 0; i < values.size(); ++i) {
        out << values[i] << ":" << probabilities[i] << " ";
    }
    out << "]" << endl;
}
} // namespace prost::parser
