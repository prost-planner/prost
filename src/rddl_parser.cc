#include "rddl_parser.h"

#include "logical_expressions.h"
#include "conditional_probability_function.h"
#include "state_action_constraint.h"
#include "typed_objects.h"
#include "utils/string_utils.h"

#include <iostream>
#include <cstdlib>

using namespace std;

void RDDLParser::parse() {
    parseDomain();
    parseNonFluents();
    parseInstance();
}

void RDDLParser::parseDomain() {
    string domainDesc = task->domainDesc;
    getTokenName(domainDesc, task->domainName, 7);

    vector<string> tokens;
    splitToken(domainDesc,tokens);

    for(unsigned int i = 0; i < tokens.size(); ++i) {
        if(tokens[i].find("requirements =") == 0) {
            //TODO: Requirements should be parsed and treated more similar to the rest: Requirements::parse(reqsAsString[i], task)
            tokens[i] = tokens[i].substr(16,tokens[i].length()-18);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            StringUtils::toLowerCase(tokens[i]);
            vector<string> reqsAsString;
            StringUtils::split(tokens[i],reqsAsString,",");
            for(unsigned int j = 0; j < reqsAsString.size(); ++j) {
                assert(task->requirements.find(reqsAsString[j]) != task->requirements.end());
                task->requirements[reqsAsString[j]] = true;
            }
        } else if(tokens[i].find("types ") == 0) {
            tokens[i] = tokens[i].substr(7,tokens[i].length()-9);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            vector<string> typesAsString;
            StringUtils::split(tokens[i],typesAsString,";");
            for(unsigned int j = 0; j < typesAsString.size(); ++j) {
                Type::parse(typesAsString[j], task);
            }
        } else if(tokens[i].find("pvariables ") == 0) {
            tokens[i] = tokens[i].substr(12,tokens[i].length()-14);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            vector<string> pvarsAsString;
            StringUtils::split(tokens[i],pvarsAsString,";");
            for(unsigned int j = 0; j < pvarsAsString.size(); ++j) {
                VariableDefinition::parse(pvarsAsString[j], task);
            }
        } else if((tokens[i].find("cpfs ") == 0) || (tokens[i].find("cdfs ") == 0)) {
            tokens[i] = tokens[i].substr(6,tokens[i].length()-8);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            vector<string> cpfsAsString;
            StringUtils::split(tokens[i], cpfsAsString, ";");
            for(unsigned int i = 0; i < cpfsAsString.size(); ++i) {
                ConditionalProbabilityFunctionDefinition::parse(cpfsAsString[i], task, this);
            }
        } else if(tokens[i].find("reward = ") == 0) {
            tokens[i] = tokens[i].substr(0,tokens[i].length()-1);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            ConditionalProbabilityFunctionDefinition::parse(tokens[i], task, this);
        } else if(tokens[i].find("state-action-constraints ") == 0) {
            tokens[i] = tokens[i].substr(26,tokens[i].length()-28);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            vector<string> sacAsString;
            StringUtils::split(tokens[i],sacAsString,";");
            for(unsigned int i = 0; i< sacAsString.size(); ++i) {
                StateActionConstraint::parse(sacAsString[i],task, this);
            }
        } else {
            cout << tokens[i] << " is unknown!" << endl;
            assert(false);
        }
    }
}

void RDDLParser::parseNonFluents() {
    string nonFluentsDesc = task->nonFluentsDesc;

    getTokenName(nonFluentsDesc,task->nonFluentsName,12);

    vector<string> tokens;
    splitToken(nonFluentsDesc,tokens);

    for(unsigned int i = 0; i < tokens.size(); ++i) {
        if(tokens[i].find("domain =") == 0) {
            string domainName = tokens[i].substr(9,tokens[i].length()-10);
            StringUtils::trim(domainName);
            StringUtils::removeTRN(domainName);
            assert(task->domainName == domainName);
        } else if(tokens[i].find("objects ") == 0) {
            tokens[i] = tokens[i].substr(9,tokens[i].length()-11);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            vector<string> objectsAsString;
            StringUtils::split(tokens[i], objectsAsString, ";");
            for(unsigned int j = 0; j < objectsAsString.size(); ++j) {
                Object::parse(objectsAsString[j], task);
            }
        } else if(tokens[i].find("non-fluents ") == 0) {
            tokens[i] = tokens[i].substr(13,tokens[i].length()-15);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            vector<string> nonFluentsAsString;
            StringUtils::split(tokens[i],nonFluentsAsString,";");
            for(unsigned int j = 0; j < nonFluentsAsString.size(); ++j) {
                AtomicLogicalExpression::parse(nonFluentsAsString[j], task);
            }
        } else {
            assert(false);
        }
    }
}

void RDDLParser::parseInstance() {
    string instanceDesc = task->instanceDesc;

    getTokenName(instanceDesc,task->instanceName,9);

    vector<string> tokens;
    splitToken(instanceDesc,tokens);

     for(unsigned int i = 0; i < tokens.size(); ++i) {
        if(tokens[i].find("domain =") == 0) {
            string domainName = tokens[i].substr(9,tokens[i].length()-10);
            StringUtils::trim(domainName);
            StringUtils::removeTRN(domainName);
            assert(task->domainName == domainName);
        } else if(tokens[i].find("non-fluents =") == 0) {
            string nonFluentsName = tokens[i].substr(14,tokens[i].length()-15);
            StringUtils::trim(nonFluentsName);
            StringUtils::removeTRN(nonFluentsName);
            assert(task->nonFluentsName == nonFluentsName);
        } else if(tokens[i].find("init-state ") == 0) {
            tokens[i] = tokens[i].substr(13,tokens[i].length()-15);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            vector<string> initStateAsString;
            StringUtils::split(tokens[i],initStateAsString,";");
            for(unsigned int j = 0; j < initStateAsString.size(); ++j) {
                AtomicLogicalExpression::parse(initStateAsString[j], task);
            }
        } else if(tokens[i].find("max-nondef-actions =") == 0) {
            tokens[i] = tokens[i].substr(21,tokens[i].length()-22);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            task->numberOfConcurrentActions = atoi(tokens[i].c_str());
        } else if(tokens[i].find("horizon =") == 0) {
            tokens[i] = tokens[i].substr(10,tokens[i].length()-11);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);
            task->horizon = atoi(tokens[i].c_str());
        } else if(tokens[i].find("discount =") == 0) {
            tokens[i] = tokens[i].substr(11,tokens[i].length()-12);
            StringUtils::trim(tokens[i]);
            StringUtils::removeTRN(tokens[i]);            
            task->discountFactor = atof(tokens[i].c_str());      
        } else {
            assert(false);
        }
    }
}

void RDDLParser::getTokenName(string& token, string& name, int startPos) {
    name = token.substr(startPos,token.find("{")-startPos-1);
    StringUtils::trim(name);
    token = token.substr(token.find("{")-1,token.length());
    StringUtils::trim(token);
}

void RDDLParser::splitToken(string& desc, vector<string>& result) {
    assert(desc.find("{") == 0);
    assert(desc[desc.length()-1] == '}');
    desc = desc.substr(1,desc.length()-2);
    stringstream tmp;
    int openParens = 0;
    for(size_t i = 0; i < desc.length(); ++i) {
        tmp << desc[i];
        if(desc[i] == '{') {
            openParens++;
        } else if(desc[i] == '}') {
            openParens--;
        }
        if(openParens == 0 && desc[i] == ';') {
            string res = tmp.str();
            StringUtils::trim(res);
            result.push_back(res);
            tmp.str("");
        }
    }
}

LogicalExpression* RDDLParser::parseRDDLFormula(string& desc, UnprocessedPlanningTask* _task) {
    vector<string> tokens;
    tokenizeFormula(desc, tokens);

    vector<vector<string> > keyWords(100); //TODO: 100 is ein HACK
    vector<vector<LogicalExpression*> > readyExpressions(100); //TODO: 100 is ein HACK
    vector<vector<ParameterDefinition*> > parameterDefintions(100);
    int numberOfOpenParanthesis = 0;
    for(unsigned int i = 0; i < tokens.size(); ++i) {
        string& token = tokens[i];
        if(token.compare("(") == 0) {
            numberOfOpenParanthesis++;
        } else if(token.compare(")") == 0) {
            vector<string>& newKeyWords = keyWords[numberOfOpenParanthesis];
            if(newKeyWords.size() == 0) {
                assert(numberOfOpenParanthesis > 0);
                readyExpressions[numberOfOpenParanthesis-1].push_back(new ParameterDefinitionSet(parameterDefintions[numberOfOpenParanthesis]));
                parameterDefintions[numberOfOpenParanthesis].clear();
                numberOfOpenParanthesis--;
            } else if(isParameterDefinition(newKeyWords)) {
                assert(numberOfOpenParanthesis > 0);
                assert(newKeyWords.size() == 3);
                parameterDefintions[numberOfOpenParanthesis-1].push_back(new ParameterDefinition(newKeyWords[0], _task->getObjectType(newKeyWords[2])));
                newKeyWords.clear();
                numberOfOpenParanthesis--;
            } else {
                //is it a variable?
                if(_task->isAVariableDefinition(newKeyWords[0])) {
                    VariableDefinition* varDef = _task->getVariableDefinition(newKeyWords[0]);
                    vector<string> varParams;
                    for(unsigned int i = 1; i < newKeyWords.size(); ++i) {
                        varParams.push_back(newKeyWords[i]);
                    }
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new UninstantiatedVariable(varDef,varParams));
                } else if(newKeyWords[0].compare("^") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new Conjunction(readyExpressions[numberOfOpenParanthesis]));
                } else if(newKeyWords[0].compare("|") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new Disjunction(readyExpressions[numberOfOpenParanthesis]));
                } else if(newKeyWords[0].compare("~") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(readyExpressions[numberOfOpenParanthesis].size() == 1);
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new NegateExpression(readyExpressions[numberOfOpenParanthesis][0]));
                } else if(newKeyWords[0].compare("sum") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new Sumation(readyExpressions[numberOfOpenParanthesis]));
                } else if(newKeyWords[0].compare("prod") == 0) { //TODO: make sure prod is indeed the keyword!
                    assert(newKeyWords.size() == 1);
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new Product(readyExpressions[numberOfOpenParanthesis]));
                } else if(newKeyWords[0].compare("exists") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new ExistentialQuantification(readyExpressions[numberOfOpenParanthesis]));
                } else if(newKeyWords[0].compare("forall") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new UniversalQuantification(readyExpressions[numberOfOpenParanthesis]));
                } else if(newKeyWords[0].compare(">=") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(readyExpressions[numberOfOpenParanthesis].size() == 2);//TODO: muss die assertion gelten?
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new GreaterEqualsExpression(readyExpressions[numberOfOpenParanthesis]));
                } else if(newKeyWords[0].compare("<=") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(readyExpressions[numberOfOpenParanthesis].size() == 2);//TODO: muss die assertion gelten?
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new LowerEqualsExpression(readyExpressions[numberOfOpenParanthesis]));
                } else if(newKeyWords[0].compare("==") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(readyExpressions[numberOfOpenParanthesis].size() == 2);//TODO: muss die assertion gelten?
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new EqualsExpression(readyExpressions[numberOfOpenParanthesis]));
                } else if(newKeyWords[0].compare("!=") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(readyExpressions[numberOfOpenParanthesis].size() == 2);//TODO: muss die assertion gelten?
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new NegateExpression(new EqualsExpression(readyExpressions[numberOfOpenParanthesis])));
                } else if(newKeyWords[0].compare("=>") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(readyExpressions[numberOfOpenParanthesis].size() == 2);//TODO: muss die assertion gelten?
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis][0] = new NegateExpression(readyExpressions[numberOfOpenParanthesis][0]);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new Disjunction(readyExpressions[numberOfOpenParanthesis]));
                } else if(newKeyWords[0].compare("<=>") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(readyExpressions[numberOfOpenParanthesis].size() == 2);//TODO: muss die assertion gelten?
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    NegateExpression* neg1 = new NegateExpression(readyExpressions[numberOfOpenParanthesis][0]);
                    NegateExpression* neg2 = new NegateExpression(readyExpressions[numberOfOpenParanthesis][1]);
                    vector<LogicalExpression*> negs;
                    negs.push_back(neg1);
                    negs.push_back(neg2);
                    Conjunction* negConj = new Conjunction(negs);
                    vector<LogicalExpression*> poss;
                    poss.push_back(readyExpressions[numberOfOpenParanthesis][0]);
                    poss.push_back(readyExpressions[numberOfOpenParanthesis][1]);
                    Conjunction* posConj = new Conjunction(poss);
                    vector<LogicalExpression*> disj;
                    disj.push_back(negConj);
                    disj.push_back(posConj);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new Disjunction(disj));
                } else if(newKeyWords[0].compare("-") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(readyExpressions[numberOfOpenParanthesis].size() == 2);//TODO: muss die assertion gelten?
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new Subtraction(readyExpressions[numberOfOpenParanthesis]));
                } else if(newKeyWords[0].compare("+") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(readyExpressions[numberOfOpenParanthesis].size() == 2);//TODO: muss die assertion gelten?
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new Addition(readyExpressions[numberOfOpenParanthesis]));
                } else if(newKeyWords[0].compare("*") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(readyExpressions[numberOfOpenParanthesis].size() == 2);//TODO: muss die assertion gelten?
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new Multiplication(readyExpressions[numberOfOpenParanthesis]));
                } else if(newKeyWords[0].compare("/") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(readyExpressions[numberOfOpenParanthesis].size() == 2);//TODO: muss die assertion gelten?
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new Division(readyExpressions[numberOfOpenParanthesis]));
                } else if(newKeyWords[0].compare("Bernoulli") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(readyExpressions[numberOfOpenParanthesis].size() == 1);
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new BernoulliDistribution(readyExpressions[numberOfOpenParanthesis][0]));
                } else if(newKeyWords[0].compare("KronDelta") == 0) {
                    assert(newKeyWords.size() == 1);
                    assert(readyExpressions[numberOfOpenParanthesis].size() == 1);
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new KronDeltaDistribution(readyExpressions[numberOfOpenParanthesis][0]));
                } else if(newKeyWords[0].compare("if") == 0) {
                    assert(newKeyWords.size() == 3);
                    assert(newKeyWords[1].compare("then") == 0);
                    assert(newKeyWords[2].compare("else") == 0); //TODO: if ohne else!!
                    assert(readyExpressions[numberOfOpenParanthesis].size() == 3);
                    assert(parameterDefintions[numberOfOpenParanthesis].size() == 0);
                    readyExpressions[numberOfOpenParanthesis-1].push_back(new IfThenElseExpression(
                            readyExpressions[numberOfOpenParanthesis][0],
                            readyExpressions[numberOfOpenParanthesis][1],
                            readyExpressions[numberOfOpenParanthesis][2]));
                } else {
                    cout << "Unknown Keyword: " << newKeyWords[0] << " ?" << endl;
                    assert(false);
                }
                newKeyWords.clear();
                readyExpressions[numberOfOpenParanthesis].clear();
                numberOfOpenParanthesis--;
                assert(numberOfOpenParanthesis >= 0);
            }
        } else if(isNumericConstant(token) || token.compare("true") == 0 || token.compare("false") == 0) {
            if(token.compare("true") == 0) {
                token = "1.0";
            } else if(token.compare("false") == 0) {
                token = "0.0";
            }
            double val = atof(token.c_str());
            readyExpressions[numberOfOpenParanthesis].push_back(_task->getConstant(val));

        } else {
            keyWords[numberOfOpenParanthesis].push_back(token);
        }
    }

    return readyExpressions[0][0];
}

bool RDDLParser::isParameterDefinition(vector<string>& tokens) {
    for(unsigned int i = 0; i < tokens.size(); ++i) {
        if(tokens[i].find(":") != -1) {
            return true;
        }
    }
    return false;
}

bool RDDLParser::isNumericConstant(string& token) {
    istringstream inpStream(token);
    double inpValue;
    if(inpStream >> inpValue) {
        return true;
    }
    return false;
}

void RDDLParser::tokenizeFormula(string& text, vector<string>& tokens) {
    string buffer = "";
    for(size_t pos = 0; pos < text.length(); ++pos) {
        const char& c = text[pos];
        if(c==' ' || pos == text.length() || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '{' ||
           c == '|' || c == '^' || c == '+' || (c == '-' && (pos == 0 || !isalpha(text[pos-1]))) || 
           c == '*' || c == '/' || c == '~' || c == '<' || c == '>') {
            if(buffer.length() > 0) {
                tokens.push_back(buffer);
                buffer = "";
            }
            if(c==' ') {
                continue;
            }
            buffer += c;
            if(c == '<' || c == '>') {
                assert(pos < text.length());
                if(text[pos+1] == '=') {
                    buffer += text[pos+1];
                    pos++;
                    if(text[pos+1] == '>') {
                        buffer += text[pos+1];
                        pos++;
                    }
                }
            }
            tokens.push_back(buffer);
            buffer = "";
        } else if(c == '=') {
            assert(pos > 0 && pos < text.length());
            assert((text[pos+1] == '>') || (text[pos+1] == '='));
            buffer += text[pos];
            buffer += text[pos+1];
            pos++;
            tokens.push_back(buffer);
            buffer = "";
        } else {
            buffer += c;
        }
    }
}
