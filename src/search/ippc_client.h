#ifndef RDDL_CLIENT_H
#define RDDL_CLIENT_H

#include <map>
#include <memory>
#include <string>
#include <vector>

class ProstPlanner;
class XMLNode;

class IPPCClient {
public:
    IPPCClient(std::string _hostName, unsigned short _port);
    ~IPPCClient();

    void run(std::string const& problem, std::string& plannerDesc);

private:
    void initConnection();
    int connectToServer();
    void closeConnection();

    void initSession(std::string const& rddlProblem, std::string& plannerDesc);
    void finishSession();

    void initRound(std::vector<double>& initialState, double& immediateReward);
    void finishRound(XMLNode const* node, double& immediateReward);

    bool submitAction(std::vector<std::string>& action,
                      std::vector<double>& nextState, double& immediateReward);

    void readState(XMLNode const* node, std::vector<double>& nextState,
                   double& immediateReward);
    void readVariable(XMLNode const* node,
                      std::map<std::string, std::string>& result);

    // If the client call did not contain a task file, we have to read the task
    // description from the server and run the external parser to create a task
    // in PROST format.
    void executeParser(std::string const& problemName,
                       std::string const& taskDesc);

    std::unique_ptr<ProstPlanner> planner;
    std::string hostName;
    unsigned short port;
    int socket;

    int numberOfRounds;

    long remainingTime;

    std::map<std::string, int> stateVariableIndices;
    std::vector<std::vector<std::string>> stateVariableValues;
};

#endif
