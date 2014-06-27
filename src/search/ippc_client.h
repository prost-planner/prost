#ifndef RDDL_CLIENT_H
#define RDDL_CLIENT_H

#include <vector>
#include <map>
#include <string>

class ProstPlanner;
class XMLNode;

class IPPCClient {
public:
    IPPCClient(ProstPlanner* _planner,
               std::string _hostName,
               unsigned short _port,
               std::map<std::string, int> const& _stateVariableIndices,
               std::vector<std::vector<std::string> > const& _stateVariableValues) :
          planner(_planner),
          hostName(_hostName),
          port(_port),
          socket(-1),
          numberOfRounds(-1),
          remainingTime(0),
          stateVariableIndices(_stateVariableIndices),
          stateVariableValues(_stateVariableValues) {}

    void run(std::string const& problemName);

private:
    void initConnection();
    int connectToServer();
    void closeConnection();

    void initSession(std::string const& rddlProblem);
    void finishSession();

    void initRound(std::vector<double>& initialState, double& immediateReward);
    void finishRound(XMLNode const* node, double& immediateReward);

    bool submitAction(std::vector<std::string>& action,
                      std::vector<double>& nextState,
                      double& immediateReward);

    void readState(XMLNode const* node,
                   std::vector<double>& nextState,
                   double& immediateReward);
    void readVariable(XMLNode const* node,
                      std::map<std::string, std::string>& result);

    ProstPlanner* planner;
    std::string hostName;
    unsigned short port;
    int socket;

    int numberOfRounds;

    long remainingTime;

    std::map<std::string, int> stateVariableIndices;
    std::vector<std::vector<std::string> > stateVariableValues;
};

#endif
