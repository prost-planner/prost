#include "test_utils.cc"
#include "../logical_expressions.h"

#include <sstream>
#include <string>

using std::string;

TEST_CASE_FIXTURE(ProstUnitTest, "Testing function evaluation") {
    State const dummyState;
    ActionState const dummyAction(0, {}, {}, {});
    string s;
    double result = 0;
    SUBCASE("Testing the evaluation of a constant") {
        s = "$c(2.0)";
        LogicalExpression* constant = LogicalExpression::createFromString(s);
        constant->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(2.0));
        s = "$c(-1.5)";
        LogicalExpression* negConstant = LogicalExpression::createFromString(s);
        negConstant->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(-1.5));
    }
    SUBCASE("Testing the evaluation of conjunctions which are either 1 or 0") {
        s = "and($c(1) $c(0) $c(0))";
        LogicalExpression* conjunct = LogicalExpression::createFromString(s);
        conjunct->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "and($c(0) $c(1) $c(1))";
        conjunct = LogicalExpression::createFromString(s);
        result = 0;
        conjunct->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "and($c(0) $c(1))";
        conjunct = LogicalExpression::createFromString(s);
        result = 0;
        conjunct->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "and($c(0))";
        conjunct = LogicalExpression::createFromString(s);
        result = 0;
        conjunct->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "and($c(1))";
        conjunct = LogicalExpression::createFromString(s);
        result = 0;
        conjunct->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "and($c(1) $c(1))";
        conjunct = LogicalExpression::createFromString(s);
        result = 0;
        conjunct->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "and($c(1) $c(1) $c(1))";
        conjunct = LogicalExpression::createFromString(s);
        result = 0;
        conjunct->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));
    }
    SUBCASE("Tests evaluation of disjunctions which are either 1 or 0") {
        s = "or($c(1) $c(0) $c(0))";
        LogicalExpression* disjunct = LogicalExpression::createFromString(s);
        disjunct->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "or($c(0) $c(1) $c(1))";
        disjunct = LogicalExpression::createFromString(s);
        result = 0;
        disjunct->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "or($c(0) $c(1))";
        disjunct = LogicalExpression::createFromString(s);
        result = 0;
        disjunct->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "or($c(0))";
        disjunct = LogicalExpression::createFromString(s);
        result = 0;
        disjunct->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "or($c(1))";
        disjunct = LogicalExpression::createFromString(s);
        result = 0;
        disjunct->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "or($c(0) $c(0))";
        disjunct = LogicalExpression::createFromString(s);
        result = 0;
        disjunct->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "or($c(0) $c(0) $c(0))";
        disjunct = LogicalExpression::createFromString(s);
        result = 0;
        disjunct->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));
    }
    SUBCASE("Tests evaluation with EqualsExpression") {
        s = "==($c(1) $c(0))";
        LogicalExpression* equal = LogicalExpression::createFromString(s);
        equal->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "==($c(1) $c(1))";
        equal = LogicalExpression::createFromString(s);
        result = 0;
        equal->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "==($c(0) $c(0))";
        equal = LogicalExpression::createFromString(s);
        result = 0;
        equal->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "==($c(-1) $c(1))";
        equal = LogicalExpression::createFromString(s);
        result = 0;
        equal->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "==($c(-2.5) $c(-2.5))";
        equal = LogicalExpression::createFromString(s);
        result = 0;
        equal->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));
    }
    SUBCASE("Tests evaluation with Greater Expression") {
        s = ">($c(1) $c(0))";
        LogicalExpression* greater = LogicalExpression::createFromString(s);
        greater->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = ">($c(1) $c(1))";
        greater = LogicalExpression::createFromString(s);
        result = 0;
        greater->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = ">($c(0) $c(0))";
        greater = LogicalExpression::createFromString(s);
        result = 0;
        greater->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = ">($c(-1) $c(1))";
        greater = LogicalExpression::createFromString(s);
        result = 0;
        greater->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = ">($c(-2.5) $c(-2.5))";
        greater = LogicalExpression::createFromString(s);
        result = 0;
        greater->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = ">($c(2.5) $c(-2.5))";
        greater = LogicalExpression::createFromString(s);
        result = 0;
        greater->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));
    }
    SUBCASE("Tests evaluation with Lower Expression") {
        s = "<($c(1) $c(0))";
        LogicalExpression* lower = LogicalExpression::createFromString(s);
        lower->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "<($c(1) $c(1))";
        lower = LogicalExpression::createFromString(s);
        result = 0;
        lower->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "<($c(0) $c(0))";
        lower = LogicalExpression::createFromString(s);
        result = 0;
        lower->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "<($c(-1) $c(1))";
        lower = LogicalExpression::createFromString(s);
        result = 0;
        lower->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "<($c(-2.5) $c(-2.5))";
        lower = LogicalExpression::createFromString(s);
        result = 0;
        lower->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "<($c(2.5) $c(-2.5))";
        lower = LogicalExpression::createFromString(s);
        result = 0;
        lower->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "<($c(-2.5) $c(2.5))";
        lower = LogicalExpression::createFromString(s);
        result = 0;
        lower->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));
    }
    SUBCASE("Tests evaluation with GreaterEquals Expression") {
        s = ">=($c(1) $c(0))";
        LogicalExpression* greaterEqual =
            LogicalExpression::createFromString(s);

        greaterEqual->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = ">=($c(1) $c(1))";
        greaterEqual = LogicalExpression::createFromString(s);
        result = 0;
        greaterEqual->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = ">=($c(0) $c(0))";
        greaterEqual = LogicalExpression::createFromString(s);
        result = 0;
        greaterEqual->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = ">=($c(-1) $c(1))";
        greaterEqual = LogicalExpression::createFromString(s);
        result = 0;
        greaterEqual->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = ">=($c(-2.5) $c(-2.5))";
        greaterEqual = LogicalExpression::createFromString(s);
        result = 0;
        greaterEqual->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = ">=($c(2.5) $c(-2.5))";
        greaterEqual = LogicalExpression::createFromString(s);
        result = 0;
        greaterEqual->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));
    }
    SUBCASE("Tests evaluation with LowerEquals Expression") {
        s = "<=($c(1) $c(0))";
        LogicalExpression* lowerEqual = LogicalExpression::createFromString(s);

        lowerEqual->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "<=($c(1) $c(1))";
        lowerEqual = LogicalExpression::createFromString(s);
        result = 0;
        lowerEqual->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "<=($c(0) $c(0))";
        lowerEqual = LogicalExpression::createFromString(s);
        result = 0;
        lowerEqual->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "<=($c(-1) $c(1))";
        lowerEqual = LogicalExpression::createFromString(s);
        result = 0;
        lowerEqual->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "<=($c(-2.5) $c(-2.5))";
        lowerEqual = LogicalExpression::createFromString(s);
        result = 0;
        lowerEqual->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "<=($c(2.5) $c(-2.5))";
        lowerEqual = LogicalExpression::createFromString(s);
        result = 0;
        lowerEqual->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));
    }
    SUBCASE("Tests evaluation of simple additions") {
        s = "+($c(1) $c(0) $c(0))";
        LogicalExpression* addition = LogicalExpression::createFromString(s);

        addition->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "+($c(0) $c(1) $c(1))";
        addition = LogicalExpression::createFromString(s);
        result = 0;
        addition->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(2));

        s = "+($c(0) $c(1))";
        addition = LogicalExpression::createFromString(s);
        result = 0;
        addition->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "+($c(0))";
        addition = LogicalExpression::createFromString(s);
        result = 0;
        addition->evaluate(result, dummyState, dummyAction);

        s = "+($c(1))";
        addition = LogicalExpression::createFromString(s);
        result = 0;
        addition->evaluate(result, dummyState, dummyAction);

        s = "+($c(1) $c(1))";
        addition = LogicalExpression::createFromString(s);
        result = 0;
        addition->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(2));

        s = "+($c(1) $c(1) $c(1))";
        addition = LogicalExpression::createFromString(s);
        result = 0;
        addition->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(3));

        s = "+($c(0) $c(0))";
        addition = LogicalExpression::createFromString(s);
        result = 0;
        addition->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "+($c(-1) $c(1))";
        addition = LogicalExpression::createFromString(s);
        result = 0;
        addition->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "+($c(-2.5) $c(-2.5))";
        addition = LogicalExpression::createFromString(s);
        result = 0;
        addition->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(-5));

        s = "+($c(2.5) $c(-2.5))";
        addition = LogicalExpression::createFromString(s);
        result = 0;
        addition->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));
    }
    SUBCASE("Tests evaluation of subtractions") {
        // Subtractions are only defined for two expressions
        s = "-($c(0) $c(1))";
        LogicalExpression* subtraction = LogicalExpression::createFromString(s);
        subtraction->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(-1));

        s = "-($c(1) $c(1))";
        subtraction = LogicalExpression::createFromString(s);
        result = 0;
        subtraction->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "-($c(0) $c(0))";
        subtraction = LogicalExpression::createFromString(s);
        result = 0;
        subtraction->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "-($c(-1) $c(1))";
        subtraction = LogicalExpression::createFromString(s);
        result = 0;
        subtraction->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(-2));

        s = "-($c(-2.5) $c(-2.5))";
        subtraction = LogicalExpression::createFromString(s);
        result = 0;
        subtraction->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "-($c(2.5) $c(-2.5))";
        subtraction = LogicalExpression::createFromString(s);
        result = 0;
        subtraction->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(5));

        s = "-($c(-2.5) $c(2.5))";
        subtraction = LogicalExpression::createFromString(s);
        result = 0;
        subtraction->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(-5));
    }
    SUBCASE("Tests evaluation of multipications") {
        // Note that multiplications are only defined for two expressions
        s = "*($c(0) $c(1))";
        LogicalExpression* multiplication =
            LogicalExpression::createFromString(s);
        multiplication->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "*(*($c(1) $c(1)) $c(1))";
        multiplication = LogicalExpression::createFromString(s);
        result = 0;
        multiplication->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "*($c(0) $c(0))";
        multiplication = LogicalExpression::createFromString(s);
        result = 0;
        multiplication->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "*($c(-1) $c(1))";
        multiplication = LogicalExpression::createFromString(s);
        result = 0;
        multiplication->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(-1));

        s = "*($c(-2.5) $c(-2.5))";
        multiplication = LogicalExpression::createFromString(s);
        result = 0;
        multiplication->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(6.25));

        s = "*($c(2.5) $c(-2.5))";
        multiplication = LogicalExpression::createFromString(s);
        result = 0;
        multiplication->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(-6.25));

        s = "*($c(-2.5) $c(2.5))";
        multiplication = LogicalExpression::createFromString(s);
        result = 0;
        multiplication->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(-6.25));
    }
    SUBCASE("Tests evaluation of divisions") {
        // Note that divisions are only defined for two expressions
        s = "/($c(0) $c(1))";
        LogicalExpression* division = LogicalExpression::createFromString(s);
        division->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        s = "/($c(1) $c(1))";
        division = LogicalExpression::createFromString(s);
        result = 0;
        division->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "/($c(-1) $c(1))";
        division = LogicalExpression::createFromString(s);
        result = 0;
        division->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(-1));

        s = "/($c(-2.5) $c(-2.5))";
        division = LogicalExpression::createFromString(s);
        result = 0;
        division->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        s = "/($c(2.5) $c(-2.5))";
        division = LogicalExpression::createFromString(s);
        result = 0;
        division->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(-1));

        s = "/($c(-2.5) $c(2.5))";
        division = LogicalExpression::createFromString(s);
        result = 0;
        division->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(-1));

        s = "/($c(-2) $c(5))";
        division = LogicalExpression::createFromString(s);
        result = 0;
        division->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(-0.4));
    }
    SUBCASE("Tests evaluation of negations") {
        // ~1 = 0
        s = "~($c(1))";
        LogicalExpression* negation = LogicalExpression::createFromString(s);
        result = 0;
        negation->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0));

        // ~0 = 1
        s = "~($c(0))";
        negation = LogicalExpression::createFromString(s);
        result = 0;
        negation->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));
    }
    SUBCASE("Tests evaluation of multiconditions") {
        // if (1) then (0.5) if(1) then (2)
        std::stringstream ss;
        ss << "switch( ($c(1) : $c(0.5)) ($c(1) : $c(2)))";
        string s = ss.str();
        LogicalExpression* multicond = LogicalExpression::createFromString(s);
        multicond->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(0.5));

        // if (0) then (0.5) else (2) = 2
        ss.str("");
        ss << "switch( ($c(0) : $c(0.5)) ($c(1) : $c(2)))";
        s = ss.str();
        result = 0;
        multicond = LogicalExpression::createFromString(s);
        multicond->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(2));
    }
    SUBCASE("Tests evaluation of nested if conditions") {
        // equals 2 * 0.5 = 1
        s = "*(switch( ($c(0) : $c(1)) ($c(1) : $c(2)))"
            "switch( ($c(1) : $c(0.5)) ($c(1) : $c(2))))";
        LogicalExpression* multiplication =
            LogicalExpression::createFromString(s);
        multiplication->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(1));

        // equals 3 * 2 * 0.5 = 3
        s = "*(switch( ($c(0) : $c(1)) ($c(0) : $c(2)) ($c(1) : $c(3)))"
            "*(switch( ($c(0) : $c(1)) ($c(1) : $c(2)))"
            "switch( ($c(1) : $c(0.5)) ($c(1) : $c(2)))))";
        multiplication = LogicalExpression::createFromString(s);
        multiplication->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(3));

        // equals 3 + 2 + 0.5 = 5.5
        s = "+(switch( ($c(0) : $c(1)) ($c(0) : $c(2)) ($c(1) : $c(3)))"
            "switch( ($c(0) : $c(1)) ($c(1) : $c(2)))"
            "switch( ($c(1) : $c(0.5)) ($c(1) : $c(2))))";
        multiplication = LogicalExpression::createFromString(s);
        multiplication->evaluate(result, dummyState, dummyAction);
        CHECK(result == doctest::Approx(5.5));
    }
}
