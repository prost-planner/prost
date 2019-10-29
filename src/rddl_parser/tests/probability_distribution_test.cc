#include "../../doctest/doctest.h"

#include "../probability_distribution.h"

using std::map;
using std::vector;

TEST_CASE("Testing equality of distributions") {
    DiscretePD pd1;
    DiscretePD pd2;
    SUBCASE("Distributions with different size are not equal") {
        pd1.assignDiracDelta(1.0);
        pd2.assignBernoulli(0.3);
        // != for distributions is not implemented
        CHECK(!(pd1 == pd2));
    }
    SUBCASE(
        "Distributions with the same size but different probabilities or "
        "values are not equal") {
        pd1.assignDiracDelta(1.0);
        pd2.assignDiracDelta(2.0);
        CHECK(!(pd1 == pd2));

        pd1.assignBernoulli(0.1);
        pd2.assignBernoulli(0.5);
        CHECK(!(pd1 == pd2));

        pd1.assignDiscrete({{2.0, 0.5}, {1.0, 0.5}});
        pd2.assignDiscrete({{1.0, 0.5}, {1.0, 0.5}});
        CHECK(!(pd1 == pd2));

        // TODO This test should succeed at some point
        // pd1.assignDiscrete({{1.0, 0.5}, {1.0, 0.8}});
        // pd2.assignDiscrete({{1.0, 0.5}, {1.0, 0.5}});
        // CHECK(!(pd1 == pd2));
    }
    SUBCASE("Distributions with same values and probabilities are equal") {
        pd1.assignDiracDelta(1.0);
        pd2.assignBernoulli(1.0);
        CHECK(pd1 == pd2);
        pd2.assignDiscrete({{1.0, 1.0}});
        CHECK(pd1 == pd2);

        pd1.assignBernoulli(0.5);
        pd2.assignDiscrete({{1.0, 0.5}, {0.0, 0.5}});
        CHECK(pd1 == pd2);

        pd1.assignDiracDelta(2.0);
        pd2.assignDiscrete({{2.0, 1.0}});
        CHECK(pd1 == pd2);
    }
}

TEST_CASE("Testing ordering of distributions") {
    DiscretePD pd1;
    DiscretePD pd2;
    DiscretePD pd3;
    SUBCASE(
        "Distributions of smaller size are less than distributions of larger "
        "size") {
        pd1.assignDiracDelta(1.0);
        pd2.assignBernoulli(0.3);
        pd3.assignDiscrete({{1, 0.25}, {2, 0.25}, {5, 0.5}});
        CHECK(pd1 < pd2);
        CHECK(pd2 < pd3);
        CHECK(pd1 < pd3);
    }
    SUBCASE("Distributions of same size are first ordered by their value") {
        pd1.assignDiracDelta(2.0);
        pd2.assignDiracDelta(3.3);
        CHECK(pd1 < pd2);

        pd1.assignDiscrete({{0, 0.25}, {10, 0.25}, {100, 0.5}});
        pd2.assignDiscrete({{1, 0.25}, {2, 0.25}, {5, 0.5}});
        CHECK(pd1 < pd2);
    }

    SUBCASE(
        "Distributions with same values are ordered by their probabilities") {
        pd1.assignBernoulli(0.3);
        pd2.assignBernoulli(0.1);
        // This one might be unintuitive, but Bernoulli distributions assign
        // their input to the truth value (i.e. 1.0), and the remainder to the
        // false value (i.e. 0.0).
        CHECK(pd1 < pd2);

        pd1.assignDiscrete({{1, 0.25}, {2, 0.2}, {5, 0.55}});
        pd2.assignDiscrete({{1, 0.25}, {2, 0.25}, {5, 0.5}});
        CHECK(pd1 < pd2);
    }
    SUBCASE("Equality does not imply less than.") {
        pd1.assignBernoulli(0.5);
        pd2.assignBernoulli(0.5);
        CHECK(!(pd1 < pd2));
    }
}

TEST_CASE("Testing retrieving the probability of a value") {
    DiscretePD pd1;
    pd1.assignDiracDelta(2.0);
    CHECK(pd1.probabilityOf(5.0) == 0);
    CHECK(pd1.probabilityOf(2.0) == 1);

    pd1.assignDiscrete({{1, 0.25}, {2, 0.2}, {5, 0.55}});
    CHECK(pd1.probabilityOf(1) == .25);
    CHECK(pd1.probabilityOf(1.5) == 0);
    CHECK(pd1.probabilityOf(2) == .2);
    CHECK(pd1.probabilityOf(3) == 0);
    CHECK(pd1.probabilityOf(5) == 0.55);
    CHECK(pd1.probabilityOf(6) == 0);
}

TEST_CASE("Testing isFalsity and isTruth") {
    DiscretePD pd1;
    pd1.assignDiracDelta(1.0);
    CHECK(pd1.isTruth());
    CHECK(!pd1.isFalsity());

    pd1.assignDiracDelta(0.0);
    CHECK(!pd1.isTruth());
    CHECK(pd1.isFalsity());

    pd1.assignBernoulli(0.0);
    CHECK(!pd1.isTruth());
    CHECK(pd1.isFalsity());

    pd1.assignBernoulli(1.0);
    CHECK(pd1.isTruth());
    CHECK(!pd1.isFalsity());
}

TEST_CASE("Testing well-definedness of probabilities") {
    DiscretePD pd1;
    CHECK(!pd1.isWellDefined());
    SUBCASE("Testing DiracDelta") {
        pd1.assignDiracDelta(1.0);
        CHECK(pd1.isWellDefined());

        pd1.assignDiracDelta(-1.0);
        CHECK(pd1.isWellDefined());
    }

    SUBCASE("Testing Bernoulli") {
        pd1.assignBernoulli(0.3);
        CHECK(pd1.isWellDefined());

        pd1.assignBernoulli(0.0);
        CHECK(pd1.isWellDefined());

        pd1.assignBernoulli(0.1);
        CHECK(pd1.isWellDefined());
    }
    SUBCASE("Testing Discrete") {
        pd1.assignDiscrete({{1, .2}, {2, .8}});
        CHECK(pd1.isWellDefined());

        pd1.assignDiscrete({{1, 0}, {2, 1}});
        CHECK(pd1.isWellDefined());

        pd1.assignDiscrete({{1, -2}, {2, 1}});
        CHECK(!pd1.isWellDefined());

        pd1.assignDiscrete({{1, 0.2}, {2, 0.2}});
        CHECK(!pd1.isWellDefined());

        pd1.assignDiscrete({{1, 0.2}, {2, 1.2}});
        CHECK(!pd1.isWellDefined());
    }
}
