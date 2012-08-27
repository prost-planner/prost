#include "ippc_client.h"
#include "prost_planner.h"
#include "search_engine.h"

#include "utils/timer.h"
#include "utils/string_utils.h"

#include <string>
#include <iostream>
#include <cstdlib>

using namespace std;

void printUsage() {
    cout << "Usage: prost <base-dir> <rddlProblem> [PROST <options>]" << endl << endl;

    cout << "The PROST options are:" << endl;
    cout << "  -s <number>       --- specify the seed (DEFAULT: time(null))" << endl;
    cout << "-ram <number>       --- specify the RAM limit used to disable caching (DEFAULT: 2621440)" << endl;
    cout << "-bit <number>       --- specify the system's bit size (DEFAULT: sizeof(long)*8)" << endl;
    cout << " -se <SearchEngine> --- specify the used search engine (MANDATORY)" << endl << endl;

    cout << "All search engines have the following options: " << endl;
    cout << "  -uc <0|1>      --- specify if caching is used (DEFAULT: YES)" << endl;
    cout << "-task <PROB|MLD> --- specify if the probabilistic task or the most likely determinized is used (DEFAULT: search engine dependent)" << endl;
    cout << "  -sd <number>   --- specify the used horizon of this search engine (DEFAULT: 15)" << endl << endl;

    cout << "A UCT search engine is defined as [UCT <options>] with the following options:" << endl;
    cout << "-mcs <number>                          --- specify the magic constant scaling factor (DEFAULT: 1.0)" << endl;
    cout << "  -T <TIME|ROLLOUTS|TIME_AND_ROLLOUTS> --- specify the timeout method (DEFAULT: TIME)" << endl;
    cout << "  -t <number>                          --- specify timeout limit (DEFAULT: 2.0)" << endl;
    cout << "  -r <number>                          --- specify the maximum number of rollouts (DEFAULT: 0)" << endl;
    cout << " -iv <number>                          --- specify number of initial visits for most likely initialization (DEFAULT: 5)" << endl;
    cout << "  -i <SearchEngine>                    --- specify the search engine used for initialization (MANDATORY)" << endl << endl;

    cout << "A IDS search engine is defined as [IDS <options>] with the following options:" << endl;
    cout << "    -t <number>  --- specify timeout limit (DEFAULT: 0.005)" << endl;
    cout << "   -st <number>  --- specify the strict timeout limit used in learning (DEFAULT: 0.1)" << endl;
    cout << "  -tra <0|1>     --- specify if IDS is terminated if a good action was found (DEFAULT: YES)" << endl << endl;
    cout << "-minsd <number>  --- specify the minimal search depth (Default: 1) If learning determines a lower search depth than this, it is set to 0 instead." << endl;

    cout << "A Random search engine is defined as [RAND <options>]." << endl << endl;

    cout << "A DFS search engine is defined as [DFS <options]. Note that is untested to be used alone (it is invoked by IDS, though)." << endl << endl;

    cout << "EXAMPLES:" << endl;
    cout << "A UCT search engine with IDS initialization of maximal search depth 10, a maximum number of 20000 rollouts is created by:" << endl;
    cout << "[PROST -se [UCT -T ROLLOUTS -r 20000 -i [IDS -sd 10]]]" << endl << endl;
    cout << "A UCT search eninge without initialization, a maximal search time of 5.0 and seed 1 is created by:" << endl;
    cout << "[PROST -se [UCT -t 5.0 -i [RAND]]]" << endl << endl;
}

int main(int argc, char** argv) {
    Timer timer;
    if(argc < 4) {
        printUsage();
        return 1;
    }

    //read non-optionals
    string dir = string(argv[1]);
    string problemName = string(argv[2]);

    //init optionals to default values
    string hostName = "localhost";
    unsigned short port = 2323;

    bool allParamsRead = false;
    string inputAsString;
    for(unsigned int i = 3; i < argc; ++i) {
        if(!allParamsRead && string(argv[i])[0] == '[') {
            allParamsRead = true;
        }
        if(allParamsRead) {
            inputAsString += string(argv[i]) ;
            inputAsString += " ";
        } else {
            string nextOption = string(argv[i]);
            if(nextOption == "-h" || nextOption == "--hostname") {
                hostName = string(argv[++i]);
            } else if(nextOption == "-p" || nextOption == "--port") {
                port = (unsigned short)(atoi(string(argv[++i]).c_str()));
            } else {
                cerr << "Unknown option: " << nextOption << endl;
                printUsage();
                return 1;
            }
        }
    }
    cout << inputAsString << endl;

    IPPCClient* client = new IPPCClient(hostName, port);
    if(!client->init()) {
        cout << "Error connecting to server " << hostName << " at port " << port << "!" << endl;
        return 1;
    }

    if(!client->run(dir, problemName, inputAsString)) {
        cout << "Error while running!" << endl;
        return 1;
    }

    if(!client->stop()) {
        cout << "Error when closing connection to server!" << endl;
        return 1;
    }

    cout << "PROST complete running time: " << timer << endl;
    return 0;
}
