#include "logical_expressions.h"

#include "search_engine.h"
#include "utils/math_utils.h"
#include "utils/string_utils.h"
#include "utils/system_utils.h"

#include <iostream>

using namespace std;

LogicalExpression* LogicalExpression::createFromString(string& desc) {
    StringUtils::trim(desc);

    if (StringUtils::startsWith(desc, "$s(")) {
        desc = desc.substr(3, desc.length() - 4);
        int index = atoi(desc.c_str());
        assert((index >= 0) && (index < SearchEngine::stateFluents.size()));
        return SearchEngine::stateFluents[index];
    } else if (StringUtils::startsWith(desc, "$a(")) {
        desc = desc.substr(3, desc.length() - 4);
        int index = atoi(desc.c_str());
        assert((index >= 0) && (index < SearchEngine::actionFluents.size()));
        return SearchEngine::actionFluents[index];
    } else if (StringUtils::startsWith(desc, "$c(")) {
        desc = desc.substr(3, desc.length() - 4);
        double value = atof(desc.c_str());
        return new NumericConstant(value);
    } else if (StringUtils::startsWith(desc, "and(")) {
        vector<LogicalExpression*> exprs = createExpressions(desc);
        return new Conjunction(exprs);
    } else if (StringUtils::startsWith(desc, "or(")) {
        vector<LogicalExpression*> exprs = createExpressions(desc);
        return new Disjunction(exprs);
    } else if (StringUtils::startsWith(desc, "==(")) {
        vector<LogicalExpression*> exprs = createExpressions(desc);
        return new EqualsExpression(exprs);
    } else if (StringUtils::startsWith(desc, ">(")) {
        vector<LogicalExpression*> exprs = createExpressions(desc);
        return new GreaterExpression(exprs);
    } else if (StringUtils::startsWith(desc, "<(")) {
        vector<LogicalExpression*> exprs = createExpressions(desc);
        return new LowerExpression(exprs);
    } else if (StringUtils::startsWith(desc, ">=(")) {
        vector<LogicalExpression*> exprs = createExpressions(desc);
        return new GreaterEqualsExpression(exprs);
    } else if (StringUtils::startsWith(desc, "<=(")) {
        vector<LogicalExpression*> exprs = createExpressions(desc);
        return new LowerEqualsExpression(exprs);
    } else if (StringUtils::startsWith(desc, "+(")) {
        vector<LogicalExpression*> exprs = createExpressions(desc);
        return new Addition(exprs);
    } else if (StringUtils::startsWith(desc, "-(")) {
        vector<LogicalExpression*> exprs = createExpressions(desc);
        return new Subtraction(exprs);
    } else if (StringUtils::startsWith(desc, "*(")) {
        vector<LogicalExpression*> exprs = createExpressions(desc);
        return new Multiplication(exprs);
    } else if (StringUtils::startsWith(desc, "/(")) {
        vector<LogicalExpression*> exprs = createExpressions(desc);
        return new Division(exprs);
    } else if (StringUtils::startsWith(desc, "~(")) {
        desc = desc.substr(2, desc.length() - 3);
        LogicalExpression* expr = LogicalExpression::createFromString(desc);
        return new Negation(expr);
    } else if (StringUtils::startsWith(desc, "exp(")) {
        desc = desc.substr(4, desc.length() - 5);
        LogicalExpression* expr = LogicalExpression::createFromString(desc);
        return new ExponentialFunction(expr);
    } else if (StringUtils::startsWith(desc, "Bernoulli(")) {
        desc = desc.substr(10, desc.length() - 11);
        LogicalExpression* expr = LogicalExpression::createFromString(desc);
        return new BernoulliDistribution(expr);
    } else if (StringUtils::startsWith(desc, "Discrete(")) {
        desc = desc.substr(9, desc.length() - 10);

        // Extract the value-probability pairs
        vector<string> tokens;
        StringUtils::tokenize(desc, '(', ')', tokens);

        vector<LogicalExpression*> values;
        vector<LogicalExpression*> probabilities;

        for (unsigned int index = 0; index < tokens.size(); ++index) {
            pair<LogicalExpression*, LogicalExpression*> valProbPair =
                splitExpressionPair(tokens[index]);

            values.push_back(valProbPair.first);
            probabilities.push_back(valProbPair.second);
        }
        return new DiscreteDistribution(values, probabilities);
    } else if (StringUtils::startsWith(desc, "switch(")) {
        desc = desc.substr(7, desc.length() - 8);

        // Extract the condition-effect pairs
        vector<string> tokens;
        StringUtils::tokenize(desc, '(', ')', tokens);
        assert(tokens.size() > 1);

        vector<LogicalExpression*> conditions;
        vector<LogicalExpression*> effects;

        for (unsigned int index = 0; index < tokens.size(); ++index) {
            pair<LogicalExpression*, LogicalExpression*> condEffPair =
                splitExpressionPair(tokens[index]);

            conditions.push_back(condEffPair.first);
            effects.push_back(condEffPair.second);
        }
        return new MultiConditionChecker(conditions, effects);
    }

    cout << desc << endl;
    assert(false);
    return nullptr;
}

vector<LogicalExpression*> LogicalExpression::createExpressions(string& desc) {
    // desc must be a string of the form keyword(expressions).
    vector<LogicalExpression*> result;

    // Remove keyword and parentheses and create an expression from each token.
    size_t cutPos = desc.find("(") + 1;
    desc = desc.substr(cutPos, desc.length() - cutPos - 1);

    // Extract the expression descriptions (careful, this only works since all
    // expressions end with a closing parenthesis!)
    vector<string> tokens;
    StringUtils::tokenize(desc, '(', ')', tokens);

    // Create the logical expressions
    for (unsigned int index = 0; index < tokens.size(); ++index) {
        result.push_back(LogicalExpression::createFromString(tokens[index]));
    }

    return result;
}

LogicalExpressionPair LogicalExpression::splitExpressionPair(string& desc) {
    // Each pair must be in parentheses
    StringUtils::removeFirstAndLastCharacter(desc);

    vector<string> tokens;
    StringUtils::tokenize(desc, '(', ')', tokens);
    assert(tokens.size() == 2);

    tokens[1] = tokens[1].substr(1);
    StringUtils::trim(tokens[1]);

    LogicalExpression* first = createFromString(tokens[0]);
    LogicalExpression* second = createFromString(tokens[1]);

    return make_pair(first, second);
}

#include "logical_expressions_includes/evaluate.cc"
#include "logical_expressions_includes/evaluate_to_kleene.cc"
#include "logical_expressions_includes/evaluate_to_pd.cc"
#include "logical_expressions_includes/print.cc"
