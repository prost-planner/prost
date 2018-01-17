#ifndef RDDL_CLIENT_H
#define RDDL_CLIENT_H

#include "prost_planner.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class XMLNode;

class IPPCClient {
public:
    IPPCClient(std::string _hostName, unsigned short _port)
        : hostName(_hostName),
          port(_port),
          socket(-1),
          numberOfRounds(-1),
          remainingTime(0) {}

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

    // Writes domain and instance from task description to temporary files.
    void generateTempFiles(std::string const& taskDesc) const;
    void removeTempFiles() const;
    // If the client call did not contain a task file, we have to read the task
    // description from the server and run the external parser to create a task
    // in PROST format.
    void executeParser(std::string const& taskDesc);

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
