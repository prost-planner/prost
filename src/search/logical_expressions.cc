#include "logical_expressions.h"

#include "utils/string_utils.h"
#include "utils/math_utils.h"
#include "utils/system_utils.h"

#include <iostream>

using namespace std;

LogicalExpression* LogicalExpression::createFromString(
        vector<StateFluent*> const& stateFluents,
        vector<ActionFluent*> const& actionFluents, string& desc) {
    if (StringUtils::startsWith(desc, "$s(")) {
        desc = desc.substr(3, desc.length() - 4);
        int index = atoi(desc.c_str());
        assert((index >= 0) && (index < stateFluents.size()));
        return stateFluents[index];
    } else if (StringUtils::startsWith(desc, "$a(")) {
        desc = desc.substr(3, desc.length() - 4);
        int index = atoi(desc.c_str());
        assert((index >= 0) && (index < actionFluents.size()));
        return actionFluents[index];
    } else if (StringUtils::startsWith(desc, "$c(")) {
        desc = desc.substr(3, desc.length() - 4);
        double value = atof(desc.c_str());
        return new NumericConstant(value);
    } else if (StringUtils::startsWith(desc, "and(")) {
        vector<LogicalExpression*> exprs;
        createExpressions(stateFluents, actionFluents, desc, exprs);
        return new Conjunction(exprs);
    } else if (StringUtils::startsWith(desc, "or(")) {
        vector<LogicalExpression*> exprs;
        createExpressions(stateFluents, actionFluents, desc, exprs);
        return new Disjunction(exprs);
    } else if (StringUtils::startsWith(desc, "==(")) {
        vector<LogicalExpression*> exprs;
        createExpressions(stateFluents, actionFluents, desc, exprs);
        return new EqualsExpression(exprs);
    } else if (StringUtils::startsWith(desc, ">(")) {
        vector<LogicalExpression*> exprs;
        createExpressions(stateFluents, actionFluents, desc, exprs);
        return new GreaterExpression(exprs);
    } else if (StringUtils::startsWith(desc, "<(")) {
        vector<LogicalExpression*> exprs;
        createExpressions(stateFluents, actionFluents, desc, exprs);
        return new LowerExpression(exprs);
    } else if (StringUtils::startsWith(desc, ">=(")) {
        vector<LogicalExpression*> exprs;
        createExpressions(stateFluents, actionFluents, desc, exprs);
        return new GreaterEqualsExpression(exprs);
    } else if (StringUtils::startsWith(desc, "<=(")) {
        vector<LogicalExpression*> exprs;
        createExpressions(stateFluents, actionFluents, desc, exprs);
        return new LowerEqualsExpression(exprs);
    } else if (StringUtils::startsWith(desc, "+(")) {
        vector<LogicalExpression*> exprs;
        createExpressions(stateFluents, actionFluents, desc, exprs);
        return new Addition(exprs);
    } else if (StringUtils::startsWith(desc, "-(")) {
        vector<LogicalExpression*> exprs;
        createExpressions(stateFluents, actionFluents, desc, exprs);
        return new Subtraction(exprs);
    } else if (StringUtils::startsWith(desc, "*(")) {
        vector<LogicalExpression*> exprs;
        createExpressions(stateFluents, actionFluents, desc, exprs);
        return new Multiplication(exprs);
    } else if (StringUtils::startsWith(desc, "/(")) {
        vector<LogicalExpression*> exprs;
        createExpressions(stateFluents, actionFluents, desc, exprs);
        return new Division(exprs);
    } else if (StringUtils::startsWith(desc, "~(")) {
        desc = desc.substr(2, desc.length() - 3);
        LogicalExpression* expr = LogicalExpression::createFromString(
                stateFluents, actionFluents, desc);
        return new Negation(expr);
    } else if (StringUtils::startsWith(desc, "Bernoulli(")) {
        desc = desc.substr(10, desc.length() - 11);
        LogicalExpression* expr = LogicalExpression::createFromString(
                stateFluents, actionFluents, desc);
        return new BernoulliDistribution(expr);
    } else if (StringUtils::startsWith(desc, "Discrete(")) {
        desc = desc.substr(9, desc.length() - 10);
        vector<LogicalExpression*> values;
        vector<LogicalExpression*> probabilities;

        vector<string> tokens;
        StringUtils::tokenize(desc, tokens);
        for (unsigned int index = 0; index < tokens.size(); ++index) {
            tokens[index] = tokens[index].substr(1, tokens[index].length() - 2);
            vector<string> valProbPair;
            StringUtils::split(tokens[index], valProbPair, ":");
            assert(valProbPair.size() == 2);
            values.push_back(LogicalExpression::createFromString(stateFluents,
                            actionFluents, valProbPair[0]));
            probabilities.push_back(LogicalExpression::createFromString(
                            stateFluents, actionFluents, valProbPair[1]));
        }
        return new DiscreteDistribution(values, probabilities);
    } else if (StringUtils::startsWith(desc, "if(")) {
        vector<LogicalExpression*> conditions;
        vector<LogicalExpression*> effects;

        vector<string> tokens;
        StringUtils::tokenize(desc, tokens);

        // All tokens are syntactically equivalent by adding "el" to the first
        tokens[0] = "el" + tokens[0];
        for (unsigned int index = 0; index < tokens.size();) {
            tokens[index] = tokens[index].substr(5, tokens[index].length() - 6);
            conditions.push_back(LogicalExpression::createFromString(
                            stateFluents, actionFluents, tokens[index]));
            ++index;

            tokens[index] = tokens[index].substr(5, tokens[index].length() - 6);
            effects.push_back(LogicalExpression::createFromString(stateFluents,
                            actionFluents, tokens[index]));
            ++index;
        }
        return new MultiConditionChecker(conditions, effects);
    }

    cout << desc << endl;
    assert(false);
    return NULL;
}

void LogicalExpression::createExpressions(
        vector<StateFluent*> const& stateFluents,
        vector<ActionFluent*> const& actionFluents, string& desc,
        vector<LogicalExpression*>& exprs) {
    size_t cutPos = desc.find("(") + 1;
    desc = desc.substr(cutPos, desc.length() - cutPos - 1);
    vector<string> tokens;
    StringUtils::tokenize(desc, tokens);

    for (unsigned int index = 0; index < tokens.size(); ++index) {
        exprs.push_back(LogicalExpression::createFromString(stateFluents,
                        actionFluents, tokens[index]));
    }
}

#include "logical_expressions_includes/evaluate.cc"
#include "logical_expressions_includes/evaluate_to_pd.cc"
#include "logical_expressions_includes/evaluate_to_kleene.cc"
#include "logical_expressions_includes/print.cc"
