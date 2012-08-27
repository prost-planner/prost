#ifndef RDDL_CLIENT_H
#define RDDL_CLIENT_H

#include <vector>
#include <map>
#include <string>

class XMLNode;

class IPPCClient {
public:
    IPPCClient(std::string _hostName, unsigned short _port) :
        hostName(_hostName), port(_port), socket(-1), accumulatedReward(0.0), numberOfRounds(-1) {}

    bool init();
    bool run(std::string& dir, std::string& problemName, std::string& plannerDesc);
    bool stop();

private:
    bool readFiles(std::string& dir, std::string& problemName, std::string& domain, std::string& problem);

    int connectToServer();
    bool initSession(std::string& rddlProblem, int& remainingTime);
    bool initRound(std::vector<double>& initialState);

    bool submitAction(std::vector<std::string>& action, std::vector<double>& nextState);

    void readState(const XMLNode* node, std::vector<double>& nextState);
    void readVariable(const XMLNode* node, std::map<std::string, double>& result);

    std::string hostName;
    unsigned short port;
    int socket;
    std::map<std::string,int> stateVariableIndices;
    double accumulatedReward;
    int numberOfRounds;
};

#endif
