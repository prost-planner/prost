#include "probability_distribution.h"

#include <algorithm>

#include "utils/math_utils.h"

using namespace std;

bool DiscretePD::isWellDefined() const {
    // Only use this function in assertions, it's quite inefficient!

    // Each value must have a probability
    if (isUndefined() || (values.size() != probabilities.size())) {
        return false;
    }

    // The sum of the probabilities must be 1, each individual probability must
    // not be 0.0 and the values must be ordered and unique
    double probSum = probabilities[0];
    double lastVal = values[0];
    for (unsigned int i = 1; i < probabilities.size(); ++i) {
        if (MathUtils::doubleIsEqual(probabilities[i], 0.0)) {
            return false;
        }
        probSum += probabilities[i];
        if (!MathUtils::doubleIsGreater(values[i], lastVal)) {
            return false;
        }
        lastVal = values[i];
    }
    return MathUtils::doubleIsEqual(probSum, 1.0);
}

void DiscretePD::print(ostream& out) const {
    out << "[ ";
    for (unsigned int i = 0; i < values.size(); ++i) {
        out << values[i] << ":" << probabilities[i] << " ";
    }
    out << "]" << endl;
}

pair<double, double> DiscretePD::sample(vector<int> const& blacklist) const {
    assert(isWellDefined());
    double remainingProbSum = 1.0;
    for (int i : blacklist) {
        remainingProbSum -= probabilities[i];
    }
    assert(MathUtils::doubleIsGreater(remainingProbSum, 0.0));

    double randNum = MathUtils::rnd->genDouble(0.0, remainingProbSum);
    double probSum = 0.0;

    for (int i = 0; i < values.size(); ++i) {
        if (find(blacklist.begin(), blacklist.end(), i) == blacklist.end()) {
            probSum += probabilities[i];
            if (MathUtils::doubleIsSmallerOrEqual(randNum, probSum)) {
                return std::make_pair(values[i], probabilities[i]);
            }
        }
    }
    assert(false);
    return std::make_pair(0, 0);
}
