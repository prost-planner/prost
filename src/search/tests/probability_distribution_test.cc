#include "test_utils.cc"

#include "../probability_distribution.h"

#include <memory>

using std::map;
using std::vector;

// Class to fake generation of random numbers
class RandomFake : public Random<std::mt19937> {
public:
    RandomFake() : counter(0) {}

    int genInt(int min, int) override {
        return min;
    }

    // Alternate generated numbers
    double genDouble(double, double) override {
        ++counter;
        if (counter % 2) {
            return 0.2;
        } else {
            return 0.4;
        }
    }

    double genReal() override {
        return 0.0;
    }

    void seed(int){};

    int counter;
};

TEST_CASE_FIXTURE(ProstUnitTest, "Testing DiracDelta distributions") {
    SUBCASE("Probability distribution only has a single value") {
        DiscretePD pd;
        pd.assignDiracDelta(5.0);
        // Sampling should always return 5.0
        for (int i = 0; i < 1000; ++i) {
            CHECK(pd.sample().first == doctest::Approx(5.0));
        }
    }
}

TEST_CASE_FIXTURE(ProstUnitTest, "Testing Discrete probability distribution") {
    MathUtils::rnd = std::unique_ptr<Random<>>(new RandomFake());
    DiscretePD pd;
    map<double, double> valueProbPairs = {{1.0, 0.2}, {2.0, 0.2}, {3.0, 0.6}};
    pd.assignDiscrete(valueProbPairs);
    SUBCASE("discrete probability distribution when all outcomes are legal") {
        // First random number is 0.2, therefore we should return the first
        // value
        CHECK(pd.sample().first == doctest::Approx(1.0));
        // Second random number is 0.4, therefore we should return 3.0
        CHECK(pd.sample().first == doctest::Approx(2.0));
    }
    SUBCASE(
        "discrete probability distribution when some outcomes are illegal") {
        // We blacklist value 2.0, therefore the new distribution is
        // equal to 1.0:0.25/3.0:0.75
        vector<int> blacklist = {1};
        // First random number is 0.2, therefore we should return the
        // first value
        CHECK(pd.sample(blacklist).first == doctest::Approx(1.0));
        // Second random number is 0.4, therefore we should return 3.0
        CHECK(pd.sample(blacklist).first == doctest::Approx(3.0));
    }
}
