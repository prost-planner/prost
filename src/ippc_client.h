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
               std::map<std::string,int> const& _stateVariableIndices,
               std::vector<std::vector<std::string> > const& _stateVariableValues) :
        planner(_planner),
        hostName(_hostName),
        port(_port),
        socket(-1),
        accumulatedReward(0.0),
        numberOfRounds(-1),
        stateVariableIndices(_stateVariableIndices),
        stateVariableValues(_stateVariableValues) {}

    void init();
    void run(std::string const& problemName);
    void stop();

private:
    int connectToServer();
    void initSession(std::string const& rddlProblem, int& remainingTime);
    void initRound(std::vector<double>& initialState);

    bool submitAction(std::vector<std::string>& action, std::vector<double>& nextState);

    void readState(const XMLNode* node, std::vector<double>& nextState);
    void readVariable(const XMLNode* node, std::map<std::string, std::string>& result);

    ProstPlanner* planner;
    std::string hostName;
    unsigned short port;
    int socket;

    double accumulatedReward;
    int numberOfRounds;

    std::map<std::string,int> stateVariableIndices;
    std::vector<std::vector<std::string> > stateVariableValues;
};

#endif
