#include "ippc_client.h"

#include "prost_planner.h"

#include "utils/system_utils.h"
#include "utils/string_utils.h"
#include "utils/strxml.h"

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

using namespace std;

void IPPCClient::init() {
    assert(socket == -1);
    try {
        socket = connectToServer();
        if(socket <= 0) {
            SystemUtils::abort("Error: couldn't connect to server.");
        }
    } catch(const exception& e) {
        SystemUtils::abort("Error: couldn't connect to server.");
    } catch(...) {
        SystemUtils::abort("Error: couldn't connect to server.");
    }
}

void IPPCClient::run(string const& problemName) {
    // Request round
    int remainingTime = -1;
    initSession(problemName, remainingTime);

    planner->setNumberOfRounds(numberOfRounds);
    vector<double> nextState(stateVariableIndices.size());

    // Main loop
    for(int i = 0; i < numberOfRounds; ++i) {
        initRound(nextState);
        planner->initNextRound();

        while(true) {
            vector<string> nextActions = planner->plan(nextState);
            if(!submitAction(nextActions, nextState)) {
                break;
            }
        }
    }

    stop();
}

void IPPCClient::stop() {
    cout << "***********************************************" << endl;
    cout << ">>>            END OF SESSION                  " << endl;
    cout << ">>>           TOTAL REWARD: " << accumulatedReward << endl;
    cout << ">>>          AVERAGE REWARD: " << (double)(accumulatedReward / (double) numberOfRounds) << endl;
    cout << "***********************************************\n" << endl;

    if(socket == -1) {
        SystemUtils::abort("Error: couldn't disconnect from server.");
    }
    close(socket);
}

/******************************************************************************
                               Server Communication
******************************************************************************/

int IPPCClient::connectToServer() {
    struct hostent* host = ::gethostbyname(hostName.c_str());
    if(!host) {
        return -1;
    }

    int res = ::socket(PF_INET, SOCK_STREAM, 0);
    if(res == -1) {
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((struct in_addr*) host->h_addr);
    memset(&(addr.sin_zero), '\0', 8);

    if(::connect(res, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
        return -1;
    }
    return res;
}

void IPPCClient::initSession(string const& rddlProblem, int& remainingTime) {
    stringstream os;
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << "<session-request>"
            << "<problem-name>" << rddlProblem << "</problem-name>"
            << "<client-name>" << "prost" << "</client-name>"
            << "<no-header/>" << "</session-request>" << '\0';
    if(write(socket, os.str().c_str(), os.str().length()) == -1) {
        SystemUtils::abort("Error: writing to socket failed.");
    }

    const XMLNode* serverResponse = XMLNode::readNode(socket);
    if(!serverResponse) {
        SystemUtils::abort("Error: initializing session failed.");
    }

    string s;
    if(!serverResponse->dissect("num-rounds", s)) {
        SystemUtils::abort("Error: server response insufficient.");
    }
    numberOfRounds = atoi(s.c_str());

    if(!serverResponse->dissect("time-allowed", s)) {
        SystemUtils::abort("Error: server response insufficient.");
    }
    remainingTime = atoi(s.c_str());

    delete serverResponse;
}

void IPPCClient::initRound(vector<double>& initialState) {
    stringstream os;
    os.str("");
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            << "<round-request/>" << '\0';

    if(write(socket, os.str().c_str(), os.str().length()) == -1) {
        SystemUtils::abort("Error: writing to socket failed.");
    }

    const XMLNode* serverResponse = XMLNode::readNode(socket);

    if(!serverResponse || serverResponse->getName() != "round-init") {
        SystemUtils::abort("Error: round-request response insufficient.");
    }

    string s;
    if(!serverResponse->dissect("time-left", s)) {
        SystemUtils::abort("Error: round-request response insufficient.");
    }
    remainingTime = atoi(s.c_str());
    cout << "***********************************************" << endl;
    cout << ">>> STARTING ROUND -- REMAINING TIME " << (remainingTime/1000) << "s" << endl;
    cout << "***********************************************\n" << endl;

    delete serverResponse;

    serverResponse = XMLNode::readNode(socket);
    readState(serverResponse, initialState);

    delete serverResponse;
}

bool IPPCClient::submitAction(vector<string>& actions, vector<double>& nextState) {
    stringstream os;
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" << "<actions>";

    for(unsigned int i = 0; i < actions.size(); ++i) {
        size_t cutPos = actions[i].find("(");
        if(cutPos == string::npos) {
            os << "<action><action-name>" << actions[i] << "</action-name>";
        } else {
            string actionName = actions[i].substr(0,cutPos);
            os << "<action><action-name>" << actionName << "</action-name>";
            string allParams = actions[i].substr(cutPos+1);
            assert(allParams[allParams.length()-1] == ')');
            allParams = allParams.substr(0,allParams.length()-1);
            vector<string> params;
            StringUtils::split(allParams, params, ",");
            for(unsigned int j = 0; j < params.size(); ++j) {
                StringUtils::trim(params[j]);
                os << "<action-arg>" << params[j] << "</action-arg>";
            }
        }
        os << "<action-value>true</action-value></action>";
    }
    os << "</actions>" << '\0';
    if(write(socket, os.str().c_str(), os.str().length()) == -1) {
        return false;
    }

    const XMLNode* serverResponse = XMLNode::readNode(socket);

    if(serverResponse->getName() == "round-end" || serverResponse->getName() == "end-session") {
        string s;
        if(!serverResponse->dissect("round-reward", s)) {
            SystemUtils::abort("Error: server communication failed.");
        }
        double reward = atof(s.c_str());
        accumulatedReward += reward;

        cout << "***********************************************" << endl;
        cout << ">>> END OF ROUND -- REWARD RECEIVED: " << reward << endl;
        cout << "***********************************************\n" << endl;
        delete serverResponse;
        return false;
    }

    readState(serverResponse, nextState);

    delete serverResponse;
    return true;
}

void IPPCClient::readState(const XMLNode* node, vector<double>& nextState) {
    assert(node);
    assert(node->getName() == "turn");

    if(node->size() == 2 && node->getChild(1)->getName() == "no-observed-fluents") {
        assert(false);
    }

    map<string, string> newValues;

    for(int i = 0; i < node->size(); i++) {
        const XMLNode* child = node->getChild(i);
        if(child->getName() == "observed-fluent") {
            readVariable(child, newValues);
        }
    }

    for(map<string,string>::iterator it = newValues.begin(); it != newValues.end(); ++it) {
        string varName = it->first;
        string value = it->second;

        // If the variable has no parameters, its name is different from the one
        // that is used by PROST internally where no parents are used (afaik,
        // this changed at some point in rddlsim, and I am not sure if it will
        // change back which is why this hacky solution is fine for the moment).
        if(varName[varName.length()-2] == '(') {
            varName = varName.substr(0,varName.length()-2);
        }

        if(stateVariableIndices.find(varName) != stateVariableIndices.end()) {
            if(stateVariableValues[stateVariableIndices[varName]].empty()) {
                // TODO: This should be a numerical variable without
                // value->index mapping, but it can also be a boolean one atm.
                if(value =="true") {
                    nextState[stateVariableIndices[varName]] = 1.0;
                } else if(value == "false") {
                    nextState[stateVariableIndices[varName]] = 0.0;
                } else {
                    nextState[stateVariableIndices[varName]] = atof(value.c_str());
                }
            } else {
                for(unsigned int i = 0; i < stateVariableValues[stateVariableIndices[varName]].size(); ++i) {
                    if(stateVariableValues[stateVariableIndices[varName]][i] == value) {
                        nextState[stateVariableIndices[varName]] = i;
                        break;
                    }
                }
            }
        }
    }
}

void IPPCClient::readVariable(const XMLNode* node, map<string, string>& result) {
    string name;
    if(node->getName() != "observed-fluent") {
        assert(false);
    }

    if(!node->dissect("fluent-name", name)) {
        assert(false);
    }
    name = name.substr(0,name.length()-1);

    vector<string> params;
    string value;
    string fluentName;

    for(int i = 0; i < node->size(); i++) {
        const XMLNode* paramNode = node->getChild(i);
        if(!paramNode) {
            assert(false);
            continue;
        } else if(paramNode->getName() == "fluent-arg") {
            string param = paramNode->getText();
            params.push_back(param.substr(0, param.length()-1));
        } else if(paramNode->getName() == "fluent-value") {
            value = paramNode->getText();
            value = value.substr(0, value.length()-1);
        }
    }
    name += "(";
    for(unsigned int i = 0; i < params.size(); ++i) {
        name += params[i];
        if(i != params.size()-1) {
            name += ", ";
        }
    }
    name += ")";
    assert(result.find(name) == result.end());
    result[name] = value;
}
