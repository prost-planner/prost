#include "ippc_client.h"

#include "parser.h"
#include "prost_planner.h"

#include "utils/base64.h"
#include "utils/string_utils.h"
#include "utils/strxml.h"
#include "utils/system_utils.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

using namespace std;

IPPCClient::IPPCClient(std::string _hostName, unsigned short _port)
    : hostName(_hostName),
      port(_port),
      socket(-1),
      numberOfRounds(-1),
      remainingTime(0) {}

// This destructor is required here to allow forward declaration of
// ProstPlanner in header because of usage of unique_ptr<ProstPlanner>
IPPCClient::~IPPCClient() = default;

void IPPCClient::run(string const& input, string& plannerDesc) {
    string problemName = input;
    // If the input refers to a problem file we parse the file. Otherwise we
    // first have to request the task description from the server.
    if (fs::exists(input)) {
        Parser parser(input);
        parser.parseTask(stateVariableIndices, stateVariableValues);
        problemName = SearchEngine::taskName;
    }

    // Init connection to the rddlsim server
    initConnection();

    // Request round
    initSession(problemName, plannerDesc);

    vector<double> nextState(stateVariableIndices.size());
    double immediateReward = 0.0;

    // Main loop
    for (unsigned int currentRound = 0; currentRound < numberOfRounds;
         ++currentRound) {
        initRound(nextState, immediateReward);

        while (true) {
            planner->initStep(nextState, remainingTime);
            vector<string> nextActions = planner->plan();
            if (!submitAction(nextActions, nextState, immediateReward)) {
                break;
            }
            planner->finishStep(immediateReward);
        }
    }

    // Get end of session message and print total result
    finishSession();

    // Close connection to the rddlsim server
    closeConnection();
}

/******************************************************************************
                               Server Communication
******************************************************************************/

void IPPCClient::initConnection() {
    assert(socket == -1);
    try {
        socket = connectToServer();
        if (socket <= 0) {
            SystemUtils::abort("Error: couldn't connect to server.");
        }
    } catch (const exception& e) {
        SystemUtils::abort("Error: couldn't connect to server.");
    } catch (...) {
        SystemUtils::abort("Error: couldn't connect to server.");
    }
}

int IPPCClient::connectToServer() {
    struct hostent* host = ::gethostbyname(hostName.c_str());
    if (!host) {
        return -1;
    }

    int res = ::socket(PF_INET, SOCK_STREAM, 0);
    if (res == -1) {
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((struct in_addr*)host->h_addr);
    memset(&(addr.sin_zero), '\0', 8);

    if (::connect(res, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        return -1;
    }
    return res;
}

void IPPCClient::closeConnection() {
    if (socket == -1) {
        SystemUtils::abort("Error: couldn't disconnect from server.");
    }
    close(socket);
}

/******************************************************************************
                     Session and rounds management
******************************************************************************/

void IPPCClient::initSession(string const& rddlProblem, string& plannerDesc) {
    stringstream os;
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
       << "<session-request>"
       << "<problem-name>" << rddlProblem << "</problem-name>"
       << "<client-name>"
       << "prost"
       << "</client-name>"
       << "<input-language>rddl</input-language>"
       << "<no-header/>"
       << "</session-request>" << '\0';
    if (write(socket, os.str().c_str(), os.str().length()) == -1) {
        SystemUtils::abort("Error: writing to socket failed.");
    }

    const XMLNode* serverResponse = XMLNode::readNode(socket);
    if (!serverResponse) {
        SystemUtils::abort("Error: initializing session failed.");
    }

    string s;
    // If the task was not initialized, we have to read it from the server and
    // run the parser
    if (SearchEngine::taskName.empty()) {
        if (!serverResponse->dissect("task", s)) {
            SystemUtils::abort(
                "Error: server response does not contain task description.");
        }
        s = decodeBase64(s);
        executeParser(rddlProblem, s);
    }

    if (!serverResponse->dissect("num-rounds", s)) {
        SystemUtils::abort("Error: server response insufficient.");
    }
    numberOfRounds = atoi(s.c_str());

    if (!serverResponse->dissect("time-allowed", s)) {
        SystemUtils::abort("Error: server response insufficient.");
    }
    remainingTime = atoi(s.c_str());

    delete serverResponse;
    // in c++ 14 we would use make_unique<ProstPlanner>
    planner = std::unique_ptr<ProstPlanner>(new ProstPlanner(plannerDesc));
    planner->init();
    planner->initSession(numberOfRounds, remainingTime);
}

void IPPCClient::finishSession() {
    XMLNode const* sessionEndResponse = XMLNode::readNode(socket);

    if (sessionEndResponse->getName() != "session-end") {
        SystemUtils::abort("Error: session end message insufficient.");
    }

    string s;
    if (!sessionEndResponse->dissect("total-reward", s)) {
        SystemUtils::abort("Error: session end message insufficient.");
    }
    double totalReward = atof(s.c_str());

    delete sessionEndResponse;

    planner->finishSession(totalReward);
}

void IPPCClient::initRound(vector<double>& initialState,
                           double& immediateReward) {
    stringstream os;
    os.str("");
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
       << "<round-request> <execute-policy>yes</execute-policy> "
          "</round-request>"
       << '\0';

    if (write(socket, os.str().c_str(), os.str().length()) == -1) {
        SystemUtils::abort("Error: writing to socket failed.");
    }

    XMLNode const* serverResponse = XMLNode::readNode(socket);

    if (!serverResponse || serverResponse->getName() != "round-init") {
        SystemUtils::abort("Error: round-request response insufficient.");
    }

    string s;
    if (!serverResponse->dissect("time-left", s)) {
        SystemUtils::abort("Error: round-request response insufficient.");
    }
    remainingTime = atoi(s.c_str());

    delete serverResponse;

    serverResponse = XMLNode::readNode(socket);

    readState(serverResponse, initialState, immediateReward);
    assert(MathUtils::doubleIsEqual(immediateReward, 0.0));

    delete serverResponse;

    planner->initRound(remainingTime);
}

void IPPCClient::finishRound(XMLNode const* node, double& immediateReward) {
    // TODO: Move immediate rewards
    string s;
    if (!node->dissect("immediate-reward", s)) {
        SystemUtils::abort("Error: round end message insufficient.");
    }
    immediateReward = atof(s.c_str());

    if (!node->dissect("round-reward", s)) {
        SystemUtils::abort("Error: server communication failed.");
    }

    double roundReward = atof(s.c_str());

    planner->finishStep(immediateReward);
    planner->finishRound(roundReward);
}

/******************************************************************************
                         Submission of actions
******************************************************************************/

bool IPPCClient::submitAction(vector<string>& actions,
                              vector<double>& nextState,
                              double& immediateReward) {
    stringstream os;
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
       << "<actions>";

    for (unsigned int i = 0; i < actions.size(); ++i) {
        size_t cutPos = actions[i].find("(");
        if (cutPos == string::npos) {
            os << "<action><action-name>" << actions[i] << "</action-name>";
        } else {
            string actionName = actions[i].substr(0, cutPos);
            os << "<action><action-name>" << actionName << "</action-name>";
            string allParams = actions[i].substr(cutPos + 1);
            assert(allParams[allParams.length() - 1] == ')');
            allParams = allParams.substr(0, allParams.length() - 1);
            vector<string> params;
            StringUtils::split(allParams, params, ",");
            for (unsigned int j = 0; j < params.size(); ++j) {
                StringUtils::trim(params[j]);
                os << "<action-arg>" << params[j] << "</action-arg>";
            }
        }
        os << "<action-value>true</action-value></action>";
    }
    os << "</actions>" << '\0';
    if (write(socket, os.str().c_str(), os.str().length()) == -1) {
        return false;
    }
    XMLNode const* serverResponse = XMLNode::readNode(socket);

    bool roundContinues = true;
    if (serverResponse->getName() == "round-end") {
        finishRound(serverResponse, immediateReward);
        roundContinues = false;
    } else {
        readState(serverResponse, nextState, immediateReward);
    }

    delete serverResponse;
    return roundContinues;
}

/******************************************************************************
                             Receiving of states
******************************************************************************/

void IPPCClient::readState(XMLNode const* node, vector<double>& nextState,
                           double& immediateReward) {
    assert(node);
    assert(node->getName() == "turn");

    if (node->size() == 2 &&
        node->getChild(1)->getName() == "no-observed-fluents") {
        assert(false);
    }

    map<string, string> newValues;

    string s;
    if (!node->dissect("time-left", s)) {
        SystemUtils::abort("Error: turn response message insufficient.");
    }
    remainingTime = atoi(s.c_str());

    if (!node->dissect("immediate-reward", s)) {
        SystemUtils::abort("Error: turn response message insufficient.");
    }
    immediateReward = atof(s.c_str());

    for (int i = 0; i < node->size(); i++) {
        XMLNode const* child = node->getChild(i);
        if (child->getName() == "observed-fluent") {
            readVariable(child, newValues);
        }
    }

    for (map<string, string>::iterator it = newValues.begin();
         it != newValues.end(); ++it) {
        string varName = it->first;
        string value = it->second;

        // If the variable has no parameters, its name is different from the one
        // that is used by PROST internally where no parents are used (afaik,
        // this changed at some point in rddlsim, and I am not sure if it will
        // change back which is why this hacky solution is fine for the moment).
        if (varName[varName.length() - 2] == '(') {
            varName = varName.substr(0, varName.length() - 2);
        }

        if (stateVariableIndices.find(varName) != stateVariableIndices.end()) {
            if (stateVariableValues[stateVariableIndices[varName]].empty()) {
                // TODO: This should be a numerical variable without
                // value->index mapping, but it can also be a boolean one atm.
                if (value == "true") {
                    nextState[stateVariableIndices[varName]] = 1.0;
                } else if (value == "false") {
                    nextState[stateVariableIndices[varName]] = 0.0;
                } else {
                    nextState[stateVariableIndices[varName]] =
                        atof(value.c_str());
                }
            } else {
                for (unsigned int i = 0;
                     i <
                     stateVariableValues[stateVariableIndices[varName]].size();
                     ++i) {
                    if (stateVariableValues[stateVariableIndices[varName]][i] ==
                        value) {
                        nextState[stateVariableIndices[varName]] = i;
                        break;
                    }
                }
            }
        }
    }
}

void IPPCClient::readVariable(XMLNode const* node,
                              map<string, string>& result) {
    string name;
    if (node->getName() != "observed-fluent") {
        assert(false);
    }

    if (!node->dissect("fluent-name", name)) {
        assert(false);
    }
    name = name.substr(0, name.length() - 1);

    vector<string> params;
    string value;
    string fluentName;

    for (int i = 0; i < node->size(); i++) {
        XMLNode const* paramNode = node->getChild(i);
        if (!paramNode) {
            assert(false);
            continue;
        } else if (paramNode->getName() == "fluent-arg") {
            string param = paramNode->getText();
            params.push_back(param.substr(0, param.length() - 1));
        } else if (paramNode->getName() == "fluent-value") {
            value = paramNode->getText();
            value = value.substr(0, value.length() - 1);
        }
    }
    name += "(";
    for (unsigned int i = 0; i < params.size(); ++i) {
        name += params[i];
        if (i != params.size() - 1) {
            name += ", ";
        }
    }
    name += ")";
    assert(result.find(name) == result.end());
    result[name] = value;
}

/******************************************************************************
                             Parser Interaction
******************************************************************************/

void IPPCClient::executeParser(string const& problemName,
                               string const& taskDesc) {
    // Generate temporary input file for parser
    fs::path domainPath = fs::current_path() / "parser_in.rddl";
    std::ofstream ofs(domainPath);
    ofs << taskDesc << endl;
    ofs.close();

    // Assumes that rddl-parser executable exists in the current directory.
    if (!fs::exists(fs::current_path() / "rddl-parser")) {
        SystemUtils::abort(
            "Error: rddl-parser executable not found in working directory.");
    }
    // TODO This probably only works in unix and is not portable.
    int result =
        std::system("./rddl-parser parser_in.rddl . -ipc2018 1");
    if (result != 0) {
        SystemUtils::abort("Error: rddl-parser had an error");
    }
    Parser parser(problemName);
    parser.parseTask(stateVariableIndices, stateVariableValues);

    // Remove temporary files
    fs::remove(fs::current_path() / "parser_in.rddl");
    fs::remove(fs::current_path() / problemName);
}
