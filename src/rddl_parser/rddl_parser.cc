#include "rddl_parser.h"

#include "logical_expressions.h"
#include "planning_task.h"

#include "utils/string_utils.h"
#include "utils/system_utils.h"

#include <cstdlib>
#include <iostream>

using namespace std;

/*****************************************************************
                         Constructor
*****************************************************************/

RDDLParser::RDDLParser() : task(new PlanningTask()) {}

/*****************************************************************
                        Toplevel parsing
*****************************************************************/

PlanningTask* RDDLParser::parse(string& domainFile, string& problemFile) {
    readFiles(domainFile, problemFile);

    parseDomain();
    parseNonFluents();
    parseInstance();

    return task;
}

void RDDLParser::readFiles(string& domainFile, string& problemFile) {
    cout << "opening files " << domainFile << " / " << problemFile << endl;

    if (!SystemUtils::readFile(domainFile, domainDesc, "//")) {
        SystemUtils::abort("Error: cannot find file " + domainFile + ".");
    }

    string problemDesc;
    if (!SystemUtils::readFile(problemFile, problemDesc, "//")) {
        SystemUtils::abort("Error: cannot find file " + problemFile + ".");
    }

    StringUtils::standardizeParens(domainDesc);
    StringUtils::standardizeSemicolons(domainDesc);
    StringUtils::standardizeColons(domainDesc);
    StringUtils::standardizeEqualSign(domainDesc);
    StringUtils::removeConsecutiveWhiteSpaces(domainDesc);

    StringUtils::standardizeParens(problemDesc);
    StringUtils::standardizeSemicolons(problemDesc);
    StringUtils::standardizeColons(problemDesc);
    StringUtils::standardizeEqualSign(problemDesc);
    StringUtils::removeConsecutiveWhiteSpaces(problemDesc);

    std::vector<string> nonFluentsAndInstance;
    StringUtils::tokenize(problemDesc, nonFluentsAndInstance);
    if (nonFluentsAndInstance.size() == 1) {
        // There is no non-fluents block
        if (nonFluentsAndInstance[0].find("instance ") != 0) {
            SystemUtils::abort("Error: No instance description found.");
        }

        instanceDesc = nonFluentsAndInstance[0];
    } else {
        assert(nonFluentsAndInstance.size() == 2);
        assert(nonFluentsAndInstance[0].find("non-fluents ") == 0 &&
               nonFluentsAndInstance[1].find("instance ") == 0);

        nonFluentsDesc = nonFluentsAndInstance[0];
        instanceDesc = nonFluentsAndInstance[1];
    }
}

void RDDLParser::parseDomain() {
    getTokenName(domainDesc, domainName, 7);

    vector<string> tokens;
    splitToken(domainDesc, tokens);

    for (unsigned int i = 0; i < tokens.size(); ++i) {
        if (tokens[i].find("requirements =") == 0) {
            // We ignore the requirements section
            continue;
        } else if (tokens[i].find("types ") == 0) {
            tokens[i] = tokens[i].substr(7, tokens[i].length() - 9);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            vector<string> typesAsString;
            StringUtils::split(tokens[i], typesAsString, ";");
            for (unsigned int j = 0; j < typesAsString.size(); ++j) {
                parseType(typesAsString[j]);
            }
        } else if (tokens[i].find("objects ") == 0) {
            tokens[i] = tokens[i].substr(9, tokens[i].length() - 11);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            vector<string> objectsAsString;
            StringUtils::split(tokens[i], objectsAsString, ";");
            for (unsigned int j = 0; j < objectsAsString.size(); ++j) {
                parseObject(objectsAsString[j]);
            }
        } else if (tokens[i].find("pvariables ") == 0) {
            tokens[i] = tokens[i].substr(12, tokens[i].length() - 14);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            vector<string> pvarsAsString;
            StringUtils::split(tokens[i], pvarsAsString, ";");
            for (unsigned int j = 0; j < pvarsAsString.size(); ++j) {
                parseVariableDefinition(pvarsAsString[j]);
            }
        } else if ((tokens[i].find("cpfs ") == 0) ||
                   (tokens[i].find("cdfs ") == 0)) {
            tokens[i] = tokens[i].substr(6, tokens[i].length() - 8);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            vector<string> cpfsAsString;
            StringUtils::split(tokens[i], cpfsAsString, ";");
            for (unsigned int i = 0; i < cpfsAsString.size(); ++i) {
                parseCPFDefinition(cpfsAsString[i]);
            }
        } else if (tokens[i].find("reward = ") == 0) {
            tokens[i] = tokens[i].substr(0, tokens[i].length() - 1);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);

            size_t cutPos = tokens[i].find("=");
            assert(cutPos != string::npos);
            string formulaString = tokens[i].substr(cutPos + 1);
            StringUtils::trim(formulaString);

            LogicalExpression* rewardFormula = parseRDDLFormula(formulaString);
            task->setRewardCPF(rewardFormula);
        } else if (tokens[i].find("state-action-constraints ") == 0) {
            tokens[i] = tokens[i].substr(26, tokens[i].length() - 28);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            vector<string> sacAsString;
            StringUtils::split(tokens[i], sacAsString, ";");
            for (unsigned int i = 0; i < sacAsString.size(); ++i) {
                LogicalExpression* formula = parseRDDLFormula(sacAsString[i]);
                task->SACs.push_back(formula);
            }
        } else {
            SystemUtils::abort("Error: Domain token '" + tokens[i] +
                               "' is unknown!");
        }
    }
}

void RDDLParser::parseNonFluents() {
    if (!nonFluentsDesc.empty()) {
        getTokenName(nonFluentsDesc, nonFluentsName, 12);

        vector<string> tokens;
        splitToken(nonFluentsDesc, tokens);

        for (unsigned int i = 0; i < tokens.size(); ++i) {
            if (tokens[i].find("domain =") == 0) {
                string domName = tokens[i].substr(9, tokens[i].length() - 10);
                StringUtils::trim(domName);
                StringUtils::removeTRN(domName);
                assert(domainName == domName);
            } else if (tokens[i].find("objects ") == 0) {
                tokens[i] = tokens[i].substr(9, tokens[i].length() - 11);
                StringUtils::trim(tokens[i]);
                StringUtils::removeTRN(tokens[i]);
                vector<string> objectsAsString;
                StringUtils::split(tokens[i], objectsAsString, ";");
                for (unsigned int j = 0; j < objectsAsString.size(); ++j) {
                    parseObject(objectsAsString[j]);
                }
            } else if (tokens[i].find("non-fluents ") == 0) {
                tokens[i] = tokens[i].substr(13, tokens[i].length() - 15);
                StringUtils::trim(tokens[i]);
                StringUtils::removeTRN(tokens[i]);
                vector<string> nonFluentsAsString;
                StringUtils::split(tokens[i], nonFluentsAsString, ";");
                for (unsigned int j = 0; j < nonFluentsAsString.size(); ++j) {
                    parseAtomicLogicalExpression(nonFluentsAsString[j]);
                }
            } else {
                SystemUtils::abort("Error: Non-fluents token '" + tokens[i] +
                                   "' is unknown!");
            }
        }
    }
}

void RDDLParser::parseInstance() {
    getTokenName(instanceDesc, instanceName, 9);
    task->name = instanceName;

    vector<string> tokens;
    splitToken(instanceDesc, tokens);

    for (unsigned int i = 0; i < tokens.size(); ++i) {
        if (tokens[i].find("domain =") == 0) {
            string domName = tokens[i].substr(9, tokens[i].length() - 10);
            StringUtils::trim(domName);
            StringUtils::removeTRN(domName);
            assert(domainName == domName);
        } else if (tokens[i].find("non-fluents =") == 0) {
            string nflName = tokens[i].substr(14, tokens[i].length() - 15);
            StringUtils::trim(nflName);
            StringUtils::removeTRN(nflName);
            assert(nonFluentsName == nflName);
        } else if (tokens[i].find("init-state ") == 0) {
            tokens[i] = tokens[i].substr(13, tokens[i].length() - 15);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            vector<string> initStateAsString;
            StringUtils::split(tokens[i], initStateAsString, ";");
            for (unsigned int j = 0; j < initStateAsString.size(); ++j) {
                parseAtomicLogicalExpression(initStateAsString[j]);
            }
        } else if (tokens[i].find("max-nondef-actions =") == 0) {
            tokens[i] = tokens[i].substr(21, tokens[i].length() - 22);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            task->numberOfConcurrentActions = atoi(tokens[i].c_str());
        } else if (tokens[i].find("horizon =") == 0) {
            tokens[i] = tokens[i].substr(10, tokens[i].length() - 11);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            task->horizon = atoi(tokens[i].c_str());
        } else if (tokens[i].find("discount =") == 0) {
            tokens[i] = tokens[i].substr(11, tokens[i].length() - 12);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            task->discountFactor = atof(tokens[i].c_str());
        } else {
            SystemUtils::abort("Error: Instance token '" + tokens[i] +
                               "' is unknown!");
        }
    }
}

/*****************************************************************
               Parsing of entries in toplevel tokens
*****************************************************************/

void RDDLParser::parseType(string& desc) {
    size_t cutPos = desc.find(":");
    assert(cutPos != string::npos);

    string typeName = desc.substr(0, cutPos);
    StringUtils::trim(typeName);

    string rest = desc.substr(cutPos + 1, desc.length());
    StringUtils::trim(rest);

    if (rest.find("{") == 0) {
        assert(rest[rest.length() - 1] == '}');
        rest = rest.substr(1, rest.length() - 2);

        task->addType(typeName);
        vector<string> valsAsString;
        StringUtils::split(rest, valsAsString, ",");
        for (unsigned int i = 0; i < valsAsString.size(); ++i) {
            task->addObject(typeName, valsAsString[i]);
        }
    } else {
        task->addType(typeName, rest);
    }
}

void RDDLParser::parseObject(string& desc) {
    size_t cutPos = desc.find(":");
    assert(cutPos != string::npos);

    string typeName = desc.substr(0, cutPos);
    StringUtils::trim(typeName);
    assert(task->types.find(typeName) != task->types.end());

    string objs = desc.substr(cutPos + 1, desc.length());
    StringUtils::trim(objs);
    assert(objs[0] == '{');
    assert(objs[objs.length() - 1] == '}');
    objs = objs.substr(1, objs.length() - 2);

    vector<string> objectNames;
    StringUtils::split(objs, objectNames, ",");
    for (unsigned int i = 0; i < objectNames.size(); ++i) {
        task->addObject(typeName, objectNames[i]);
    }
}

void RDDLParser::parseVariableDefinition(string& desc) {
    size_t cutPos = desc.find(":");
    assert(cutPos != string::npos);

    string nameAndParams = desc.substr(0, cutPos);
    StringUtils::trim(nameAndParams);
    string rest = desc.substr(cutPos + 1);
    StringUtils::trim(rest);

    string name;
    vector<Parameter*> params;

    if (nameAndParams[nameAndParams.length() - 1] != ')') {
        name = nameAndParams;
    } else {
        cutPos = nameAndParams.find("(");
        name = nameAndParams.substr(0, cutPos);

        string allParams = nameAndParams.substr(
            cutPos + 1, nameAndParams.length() - cutPos - 2);
        vector<string> allParamsAsString;
        StringUtils::split(allParams, allParamsAsString, ",");
        for (unsigned int i = 0; i < allParamsAsString.size(); ++i) {
            if (task->types.find(allParamsAsString[i]) == task->types.end()) {
                SystemUtils::abort("Error: undefined type " +
                                   allParamsAsString[i] +
                                   " used as parameter in " + name + ".");
            }
            params.push_back(
                new Parameter(task->types[allParamsAsString[i]]->name,
                              task->types[allParamsAsString[i]]));
        }
    }

    assert(rest[0] == '{');
    assert(rest[rest.length() - 1] == '}');
    rest = rest.substr(1, rest.length() - 2);
    vector<string> optionals;
    StringUtils::split(rest, optionals, ",");

    assert(optionals.size() == 3);
    ParametrizedVariable::VariableType varType;
    if (optionals[0] == "state-fluent") {
        varType = ParametrizedVariable::STATE_FLUENT;
    } else if (optionals[0] == "action-fluent") {
        varType = ParametrizedVariable::ACTION_FLUENT;
    } /*else if(optionals[0] == "interm-fluent") {
        varType = ParametrizedVariable::INTERM_FLUENT;
        } else if(optionals[0] == "derived-fluent") {
        varType = ParametrizedVariable::DERIVED_FLUENT;
        } */
    else {
        assert(optionals[0] == "non-fluent");
        varType = ParametrizedVariable::NON_FLUENT;
    }

    assert(task->types.find(optionals[1]) != task->types.end());
    Type* valueType = task->types[optionals[1]];

    double defaultVal = 0.0;
    string defaultValString;
    // int level = 0;

    switch (varType) {
    case ParametrizedVariable::NON_FLUENT:
    case ParametrizedVariable::STATE_FLUENT:
    case ParametrizedVariable::ACTION_FLUENT:
        assert(optionals[2].find("default =") == 0);
        defaultValString = optionals[2].substr(9, optionals[2].length());
        StringUtils::trim(defaultValString);
        if (valueType->name == "int") {
            defaultVal = atoi(defaultValString.c_str());
        } else if (valueType->name == "real") {
            defaultVal = atof(defaultValString.c_str());
        } else {
            if (task->objects.find(defaultValString) == task->objects.end()) {
                string err = "Error: Default value " + defaultValString +
                             " of variable " + name + " not defined.";
                SystemUtils::abort(err);
            }
            Object* val = task->objects[defaultValString];
            defaultVal = val->value;
        }
        break;
        // case ParametrizedVariable::INTERM_FLUENT:
        // assert(optionals[2].find("level =") == 0);
        // optionals[2] = optionals[2].substr(7,optionals[2].length());
        // StringUtils::trim(optionals[2]);
        // level = atoi(optionals[2].c_str());
        // break;
    }

    ParametrizedVariable* var =
        new ParametrizedVariable(name, params, varType, valueType, defaultVal);

    task->addVariableDefinition(var);
}

void RDDLParser::parseCPFDefinition(string& desc) {
    size_t cutPos = desc.find("=");
    assert(cutPos != string::npos);

    string nameAndParams = desc.substr(0, cutPos);
    StringUtils::trim(nameAndParams);
    string rest = desc.substr(cutPos + 1);
    StringUtils::trim(rest);

    string name;
    vector<string> params;
    StringUtils::trim(nameAndParams);

    StringUtils::removeFirstAndLastCharacter(nameAndParams);

    vector<string> nameAndParamsVec;
    StringUtils::split(nameAndParams, nameAndParamsVec, " ");

    assert(nameAndParamsVec.size() > 0);
    name = nameAndParamsVec[0];
    StringUtils::trim(name);

    if (name[name.length() - 1] == '\'') {
        name = name.substr(0, name.length() - 1);
    }

    if (task->variableDefinitions.find(name) ==
        task->variableDefinitions.end()) {
        SystemUtils::abort("Error: no according variable to CPF " + name + ".");
    }
    ParametrizedVariable* head = task->variableDefinitions[name];
    assert(head->params.size() == (nameAndParamsVec.size() - 1));

    // TODO: Currently, we don't allow constants in the head of a CPF, as the
    // RDDL specification is not really clear if it is allowed.
    for (unsigned int i = 0; i < head->params.size(); ++i) {
        // Set the parameter names
        head->params[i]->name = nameAndParamsVec[i + 1];
    }
    if (task->CPFDefinitions.find(head) != task->CPFDefinitions.end()) {
        SystemUtils::abort("Error: Multiple definition of CPF " + name + ".");
    }
    task->CPFDefinitions[head] = parseRDDLFormula(rest);
}

void RDDLParser::parseAtomicLogicalExpression(string& desc) {
    if (desc.find("~") == 0) {
        assert(desc.find("=") == string::npos);
        desc = desc.substr(1) + " = false";
    } else if (desc.find("=") == string::npos) {
        desc += " = true";
    }
    size_t cutPos = desc.find("=");
    assert(cutPos != string::npos);

    string nameAndParams = desc.substr(0, cutPos);
    StringUtils::trim(nameAndParams);

    string initialValString = desc.substr(cutPos + 1);
    StringUtils::trim(initialValString);

    string name;
    vector<Parameter*> params;
    if ((cutPos = nameAndParams.find("(")) == string::npos) {
        name = nameAndParams;
    } else {
        name = nameAndParams.substr(0, cutPos);
        string paramsAsString = nameAndParams.substr(cutPos + 1);
        assert(paramsAsString[paramsAsString.size() - 1] == ')');
        paramsAsString = paramsAsString.substr(0, paramsAsString.size() - 1);
        vector<string> paramStrings;
        StringUtils::split(paramsAsString, paramStrings, ",");
        for (unsigned int i = 0; i < paramStrings.size(); ++i) {
            if (task->objects.find(paramStrings[i]) == task->objects.end()) {
                SystemUtils::abort("Error: Undefined object " +
                                   paramStrings[i] + " referenced in " + desc +
                                   ".");
            }
            params.push_back(task->objects[paramStrings[i]]);
        }
    }

    if (task->variableDefinitions.find(name) ==
        task->variableDefinitions.end()) {
        SystemUtils::abort("Error: variable " + name +
                           " used but not defined.");
    }
    ParametrizedVariable* parent = task->variableDefinitions[name];

    // Instantiation is not performed yet, so the according fluents don't exist
    // and we have to create them here

    double initialVal = 0.0;
    if (parent->valueType->name == "int") {
        initialVal = atoi(initialValString.c_str());
    } else if (parent->valueType->name == "real") {
        initialVal = atof(initialValString.c_str());
    } else {
        if (task->objects.find(initialValString) == task->objects.end()) {
            string err = "Error: Initial value" + initialValString +
                         " of variable " + name + " not defined.";
            SystemUtils::abort(err);
        }
        Object* val = task->objects[initialValString];
        initialVal = val->value;
    }

    task->addParametrizedVariable(parent, params, initialVal);
}

LogicalExpression* RDDLParser::parseRDDLFormula(string desc) {
    vector<string> tokens = tokenizeFormula(desc);
    assert(!tokens.empty());

    if (tokens.size() == 1) {
        if (isNumericConstant(tokens[0])) {
            double val = atof(tokens[0].c_str());
            return new NumericConstant(val);
        } else {
            Parameter* param = parseParameter(tokens[0]);
            if (param) {
                return param;
            }
        } // Otherwise, this is a parameterless fluent
    }

    if (task->variableDefinitions.find(tokens[0]) !=
        task->variableDefinitions.end()) {
        // Fluent
        vector<Parameter*> params;
        for (unsigned int i = 1; i < tokens.size(); ++i) {
            Parameter* param = parseParameter(tokens[i]);
            assert(param);
            params.push_back(param);
        }
        // Copy the schematic variable and replace the params
        return new ParametrizedVariable(*task->variableDefinitions[tokens[0]],
                                        params);
    } else if (tokens[0] == "sum") {
        // Sumation
        assert(tokens.size() == 3);
        ParameterList* paramList = parseParameterList(tokens[1]);
        LogicalExpression* expr = parseRDDLFormula(tokens[2]);
        return new Sumation(paramList, expr);
    } else if (tokens[0] == "prod") {
        // Product
        assert(tokens.size() == 3);
        ParameterList* paramList = parseParameterList(tokens[1]);
        LogicalExpression* expr = parseRDDLFormula(tokens[2]);
        return new Product(paramList, expr);
    } else if (tokens[0] == "forall") {
        // UniversalQuantification
        assert(tokens.size() == 3);
        ParameterList* paramList = parseParameterList(tokens[1]);
        LogicalExpression* expr = parseRDDLFormula(tokens[2]);
        return new UniversalQuantification(paramList, expr);
    } else if (tokens[0] == "exists") {
        // ExistentialQuantification
        assert(tokens.size() == 3);
        ParameterList* paramList = parseParameterList(tokens[1]);
        LogicalExpression* expr = parseRDDLFormula(tokens[2]);
        return new ExistentialQuantification(paramList, expr);
    } else if ((tokens[0] == "^") || (tokens[0] == "&")) {
        // Conjunction
        vector<LogicalExpression*> exprs;
        for (unsigned int i = 1; i < tokens.size(); ++i) {
            LogicalExpression* expr = parseRDDLFormula(tokens[i]);
            exprs.push_back(expr);
        }
        return new Conjunction(exprs);
    } else if (tokens[0] == "|") {
        // Disjunction
        vector<LogicalExpression*> exprs;
        for (unsigned int i = 1; i < tokens.size(); ++i) {
            LogicalExpression* expr = parseRDDLFormula(tokens[i]);
            exprs.push_back(expr);
        }
        return new Disjunction(exprs);
    } else if (tokens[0] == "=>") {
        // Implication
        assert(tokens.size() == 3);

        vector<LogicalExpression*> exprs;

        LogicalExpression* expr1 = parseRDDLFormula(tokens[1]);
        exprs.push_back(new Negation(expr1));

        LogicalExpression* expr2 = parseRDDLFormula(tokens[2]);
        exprs.push_back(expr2);

        return new Disjunction(exprs);
    } else if (tokens[0] == "<=>") {
        // Logical Equivalence
        assert(tokens.size() == 3);

        vector<LogicalExpression*> posExprs;
        vector<LogicalExpression*> negExprs;

        LogicalExpression* expr1 = parseRDDLFormula(tokens[1]);
        posExprs.push_back(expr1);
        LogicalExpression* expr1Copy(expr1);
        negExprs.push_back(new Negation(expr1Copy));

        LogicalExpression* expr2 = parseRDDLFormula(tokens[2]);
        posExprs.push_back(expr2);
        LogicalExpression* expr2Copy(expr2);
        negExprs.push_back(new Negation(expr2Copy));

        vector<LogicalExpression*> exprs;
        exprs.push_back(new Conjunction(posExprs));
        exprs.push_back(new Conjunction(negExprs));

        return new Disjunction(exprs);
    } else if (tokens[0] == "==") {
        // EqualsExpression

        vector<LogicalExpression*> exprs;
        for (unsigned int i = 1; i < tokens.size(); ++i) {
            Parameter* param = parseParameter(tokens[i]);
            if (param) {
                exprs.push_back(param);
            } else {
                LogicalExpression* expr = parseRDDLFormula(tokens[i]);
                exprs.push_back(expr);
            }
        }
        return new EqualsExpression(exprs);
    } else if ((tokens[0] == "~=") || (tokens[0] == "!=")) {
        // Negation of EqualsExpression

        vector<LogicalExpression*> exprs;
        for (unsigned int i = 1; i < tokens.size(); ++i) {
            Parameter* param = parseParameter(tokens[i]);
            if (param) {
                exprs.push_back(param);
            } else {
                LogicalExpression* expr = parseRDDLFormula(tokens[i]);
                exprs.push_back(expr);
            }
        }
        return new Negation(new EqualsExpression(exprs));
    } else if (tokens[0] == "exp") {
        // Exponential Function
        assert(tokens.size() == 2);

        LogicalExpression* expr = parseRDDLFormula(tokens[1]);
        return new ExponentialFunction(expr);
    } else if (tokens[0] == ">") {
        // GreaterExpression

        vector<LogicalExpression*> exprs;
        for (unsigned int i = 1; i < tokens.size(); ++i) {
            Parameter* param = parseParameter(tokens[i]);
            if (param) {
                assert(false); // This can only occur with ranged ints
                exprs.push_back(param);
            } else {
                LogicalExpression* expr = parseRDDLFormula(tokens[i]);
                exprs.push_back(expr);
            }
        }
        return new GreaterExpression(exprs);
    } else if (tokens[0] == "<") {
        // LowerExpression

        vector<LogicalExpression*> exprs;
        for (unsigned int i = 1; i < tokens.size(); ++i) {
            Parameter* param = parseParameter(tokens[i]);
            if (param) {
                assert(false); // This can only occur with ranged ints
                exprs.push_back(param);
            } else {
                LogicalExpression* expr = parseRDDLFormula(tokens[i]);
                exprs.push_back(expr);
            }
        }
        return new LowerExpression(exprs);
    } else if (tokens[0] == ">=") {
        // GreaterEqualsExpression

        vector<LogicalExpression*> exprs;
        for (unsigned int i = 1; i < tokens.size(); ++i) {
            Parameter* param = parseParameter(tokens[i]);
            if (param) {
                assert(false); // This can only occur with ranged ints
                exprs.push_back(param);
            } else {
                LogicalExpression* expr = parseRDDLFormula(tokens[i]);
                exprs.push_back(expr);
            }
        }
        return new GreaterEqualsExpression(exprs);
    } else if (tokens[0] == "<=") {
        // LowerEqualsExpression

        vector<LogicalExpression*> exprs;
        for (unsigned int i = 1; i < tokens.size(); ++i) {
            Parameter* param = parseParameter(tokens[i]);
            if (param) {
                assert(false); // This can only occur with ranged ints
                exprs.push_back(param);
            } else {
                LogicalExpression* expr = parseRDDLFormula(tokens[i]);
                exprs.push_back(expr);
            }
        }
        return new LowerEqualsExpression(exprs);
    } else if (tokens[0] == "+") {
        // Addition

        vector<LogicalExpression*> exprs;
        for (unsigned int i = 1; i < tokens.size(); ++i) {
            Parameter* param = parseParameter(tokens[i]);
            if (param) {
                assert(false); // This can only occur with ranged ints
                exprs.push_back(param);
            } else {
                LogicalExpression* expr = parseRDDLFormula(tokens[i]);
                exprs.push_back(expr);
            }
        }
        return new Addition(exprs);
    } else if (tokens[0] == "-") {
        // Subtraction

        vector<LogicalExpression*> exprs;
        for (unsigned int i = 1; i < tokens.size(); ++i) {
            Parameter* param = parseParameter(tokens[i]);
            if (param) {
                assert(false); // This can only occur with ranged ints
                exprs.push_back(param);
            } else {
                LogicalExpression* expr = parseRDDLFormula(tokens[i]);
                exprs.push_back(expr);
            }
        }
        return new Subtraction(exprs);
    } else if (tokens[0] == "*") {
        // Multiplication

        vector<LogicalExpression*> exprs;
        for (unsigned int i = 1; i < tokens.size(); ++i) {
            Parameter* param = parseParameter(tokens[i]);
            if (param) {
                assert(false); // This can only occur with ranged ints
                exprs.push_back(param);
            } else {
                LogicalExpression* expr = parseRDDLFormula(tokens[i]);
                exprs.push_back(expr);
            }
        }
        return new Multiplication(exprs);
    } else if (tokens[0] == "/") {
        // Division

        vector<LogicalExpression*> exprs;
        for (unsigned int i = 1; i < tokens.size(); ++i) {
            Parameter* param = parseParameter(tokens[i]);
            if (param) {
                assert(false); // This can only occur with ranged ints
                exprs.push_back(param);
            } else {
                LogicalExpression* expr = parseRDDLFormula(tokens[i]);
                exprs.push_back(expr);
            }
        }
        return new Division(exprs);
    } else if (tokens[0] == "~") {
        // Negation
        assert(tokens.size() == 2);
        LogicalExpression* expr = parseRDDLFormula(tokens[1]);
        return new Negation(expr);
    } else if (tokens[0] == "KronDelta") {
        // KronDeltaDistribution

        // TODO: Can we return expr instead of the KronDelta?
        assert(tokens.size() == 2);
        LogicalExpression* expr = parseRDDLFormula(tokens[1]);
        return new KronDeltaDistribution(expr);
    } else if (tokens[0] == "Bernoulli") {
        // BernoulliDistribution
        assert(tokens.size() == 2);
        LogicalExpression* expr = parseRDDLFormula(tokens[1]);
        return new BernoulliDistribution(expr);
    } else if (tokens[0] == "Discrete") {
        // DiscreteDistribution

        // TODO: The return value (tokens[1]) is optional in RDDL2.0
        assert(tokens.size() == 3);

        vector<string> valProbPairs = tokenizeFormula(tokens[2]);

        vector<LogicalExpression*> values;
        vector<LogicalExpression*> probabilities;

        for (unsigned int i = 0; i < valProbPairs.size(); ++i) {
            assert(valProbPairs[i][0] == '(');
            assert(valProbPairs[i][valProbPairs[i].length() - 1] == ')');
            StringUtils::removeFirstAndLastCharacter(valProbPairs[i]);

            size_t cutPos = valProbPairs[i].find(":");
            assert(cutPos != string::npos);

            string valueString = valProbPairs[i].substr(0, cutPos);
            StringUtils::trim(valueString);
            LogicalExpression* value = parseRDDLFormula(valueString);
            values.push_back(value);

            string probString = valProbPairs[i].substr(cutPos + 1);
            StringUtils::trim(probString);
            LogicalExpression* prob = parseRDDLFormula(probString);
            probabilities.push_back(prob);
        }
        return new DiscreteDistribution(values, probabilities);
    } else if (tokens[0] == "if") {
        // IfThenElseExpression
        assert(tokens.size() == 6);
        assert(tokens[2].find("then") == 0);
        assert(tokens[4].find("else") == 0);
        LogicalExpression* condition = parseRDDLFormula(tokens[1]);
        LogicalExpression* valueIfTrue = parseRDDLFormula(tokens[3]);
        LogicalExpression* valueIfFalse = parseRDDLFormula(tokens[5]);
        return new IfThenElseExpression(condition, valueIfTrue, valueIfFalse);
    } else if (tokens[0] == "switch") {
        // Switch (MultiConditionChecker)
        assert(tokens.size() == 3);

        LogicalExpression* switchVar = parseRDDLFormula(tokens[1]);

        vector<string> cases = tokenizeFormula(tokens[2]);

        vector<LogicalExpression*> conditions;
        vector<LogicalExpression*> effects;

        for (unsigned int i = 0; i < cases.size(); ++i) {
            assert(cases[i][0] == '(');
            assert(cases[i][cases[i].length() - 1] == ')');
            StringUtils::removeFirstAndLastCharacter(cases[i]);
            StringUtils::trim(cases[i]);
            assert(cases[i].find("case ") == 0);
            cases[i] = cases[i].substr(5);

            size_t cutPos = cases[i].find(":");
            assert(cutPos != string::npos);

            if (i == (cases.size() - 1)) {
                // The last one must be true (we should thereby support the
                // otherwise keyword already)
                conditions.push_back(new NumericConstant(1.0));
            } else {
                // Otherwise we build the condition that says switchVar == value

                string valueString = cases[i].substr(0, cutPos);
                StringUtils::trim(valueString);

                LogicalExpression* value = parseRDDLFormula(valueString);

                vector<LogicalExpression*> switchVarEquality;
                switchVarEquality.push_back(switchVar);
                switchVarEquality.push_back(value);
                conditions.push_back(new EqualsExpression(switchVarEquality));
            }

            string effString = cases[i].substr(cutPos + 1);
            StringUtils::trim(effString);

            LogicalExpression* eff = parseRDDLFormula(effString);
            effects.push_back(eff);
        }

        return new MultiConditionChecker(conditions, effects);
    }

    SystemUtils::abort("Error: Unsupported RDDL Formula: " + desc);
    return nullptr;
}

ParameterList* RDDLParser::parseParameterList(string& desc) {
    vector<string> tokens = tokenizeFormula(desc);

    vector<Parameter*> params;
    vector<Type*> types;

    // TODO: Also allow lists of the form ( (?x1, ?x2 : type_x) )

    for (unsigned int i = 0; i < tokens.size(); ++i) {
        assert(tokens[i][0] == '(');
        assert(tokens[i][tokens[i].length() - 1] == ')');
        StringUtils::removeFirstAndLastCharacter(tokens[i]);

        size_t cutPos = tokens[i].find(":");
        assert(cutPos != string::npos);

        string name = tokens[i].substr(0, cutPos);
        StringUtils::trim(name);
        assert(!name.empty());
        assert(name[0] == '?');

        string type = tokens[i].substr(cutPos + 1, tokens[i].length());
        StringUtils::trim(type);

        if (task->types.find(type) == task->types.end()) {
            SystemUtils::abort("Error: Type " + type + " not defined.");
        }

        params.push_back(new Parameter(name, task->types[type]));
        types.push_back(task->types[type]);
    }

    return new ParameterList(params, types);
}

Parameter* RDDLParser::parseParameter(string& desc) {
    if (task->objects.find(desc) != task->objects.end()) {
        return task->objects[desc];
    } else if (desc[0] == '?') {
        return new Parameter(desc);
    }
    return nullptr;
}

bool RDDLParser::isNumericConstant(string& token) {
    if (token.compare("true") == 0) {
        token = "1.0";
    } else if (token.compare("false") == 0) {
        token = "0.0";
    }
    istringstream inpStream(token);
    double inpValue;
    if (inpStream >> inpValue) {
        return true;
    }
    return false;
}

/*****************************************************************
                      Helper Functions
*****************************************************************/

void RDDLParser::getTokenName(string& token, string& name, int startPos) {
    name = token.substr(startPos, token.find("{") - startPos - 1);
    StringUtils::trim(name);
    token = token.substr(token.find("{") - 1, token.length());
    StringUtils::trim(token);
}

void RDDLParser::splitToken(string& desc, vector<string>& result) {
    assert(desc.find("{") == 0);
    assert(desc[desc.length() - 1] == '}');
    desc = desc.substr(1, desc.length() - 2);
    stringstream tmp;
    int openParens = 0;
    for (size_t i = 0; i < desc.length(); ++i) {
        tmp << desc[i];
        if (desc[i] == '{') {
            openParens++;
        } else if (desc[i] == '}') {
            openParens--;
        }
        if (openParens == 0 && desc[i] == ';') {
            string res = tmp.str();
            StringUtils::trim(res);
            result.push_back(res);
            tmp.str("");
        }
    }
}

vector<string> RDDLParser::tokenizeFormula(string& text) {
    vector<string> tokens;
    if (text[0] == '(') {
        assert(text[text.length() - 1] == ')');
        StringUtils::removeFirstAndLastCharacter(text);
    }

    string buffer = "";
    int openParens = 0;
    for (size_t pos = 0; pos < text.length(); ++pos) {
        const char& c = text[pos];
        if (c == '(') {
            ++openParens;
        }
        buffer += text[pos];

        if (c == ')') {
            --openParens;
            if (openParens == 0) {
                StringUtils::trim(buffer);
                if (!buffer.empty()) {
                    tokens.push_back(buffer);
                    buffer = "";
                }
            }
        } else if ((c == ' ') && (openParens == 0)) {
            StringUtils::trim(buffer);
            if (!buffer.empty()) {
                tokens.push_back(buffer);
                buffer = "";
            }
        }
    }
    StringUtils::trim(buffer);
    if (!buffer.empty()) {
        tokens.push_back(buffer);
    }
    return tokens;
}
