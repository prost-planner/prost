#ifndef IPC_CLIENT_H
#define IPC_CLIENT_H

/*
  The IPC client implementation is based on the GPL3 licensed C++ rddlsim client
  implementation of Sungwook Yoon and Scott Sanner which has been created for
  the International Planning Competition (IPC) 2011. The code was modified
  substantially.
*/

#include <map>
#include <memory>
#include <string>
#include <vector>

class ProstPlanner;
class XMLNode;

class IPCClient {
public:
    IPCClient(std::string _hostName, unsigned short _port,
            std::string parserOptions);
    ~IPCClient();

    void run(std::string const& instanceName, std::string& plannerDesc);

private:
    void initConnection();
    int connectToServer();
    void closeConnection();

    void initSession(std::string const& instanceName, std::string& plannerDesc);
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
    void executeParser(std::string const& taskDesc);

    std::unique_ptr<ProstPlanner> planner;
    std::string hostName;
    unsigned short port;
    int socket;

    std::string parserOptions;

    int numberOfRounds;

    long remainingTime;

    std::map<std::string, int> stateVariableIndices;
    std::vector<std::vector<std::string>> stateVariableValues;
};

#endif // IPC_CLIENT_H
