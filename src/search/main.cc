#include "parser.h"
#include "ippc_client.h"
#include "prost_planner.h"

#include "utils/timer.h"

#include <string>
#include <iostream>
#include <cstdlib>

using namespace std;

void printUsage() {
    cout << "Usage: ./prost <rddl-parser-output> [PROST <options>]" << endl <<
    endl;

    cout << "/*************************************************************" <<
    endl
         << "                     PROST Planner Options" << endl
         << "*************************************************************/" <<
    endl;

    cout <<
    "Originally, the PROST planner has been the planning system that won IPPC 2011. It has been extended step by step and is now a framework that implements several Trial-based Heuristic Tree Search Algorithms including UCT, DP-UCT and UCT* as described by Keller and Helmert (2013)."
         << endl <<
    "We aim to develop a system where components (i.e., search engines) can be mixed and matched at will. Therefore, option values of search engines in the following are either primitive or they can be a search engine themselves. Note that by far not each cmbination has been tested."
         << endl << endl;

    cout << "  -s <double>" << endl;
    cout << "    Specifies the seed." << endl;
    cout << "    Default: time(null)" << endl << endl;

    cout << "  -ram <int>" << endl;
    cout << "    Specifies the RAM limit (in KB) used to disable caching." <<
    endl;
    cout << "    Default: 2621440 (i.e. 2.5 GB)" << endl << endl;

    cout << "  -bit <32 | 64>" << endl;
    cout <<
    "    Specifies the system's bit size. Should not be set by hand." << endl;
    cout << "    Default: sizeof(long)*8" << endl << endl;

    cout << "  -se <SearchEngine>" << endl;
    cout << "    Specifies the used main search engine." << endl;
    cout << "    Default: n/a, this MUST be specified." << endl << endl << endl;


    cout << "/*************************************************************" <<
    endl
         << "                            MC-UCT" << endl
         << "*************************************************************/" <<
    endl;

    cout <<
    "An MC-UCT search engine is equivalent to the algorithm described by Kocsis and Szepesvari (2006), and is implemented in terms of the Trial-based Heuristic Tree Search framework. It can be created as [MC-UCT <options>] with the following options:"
         << endl << endl;

    cout << "  -uc <0|1>" << endl;
    cout <<
    "    Specifies if caching is used. If this is switched on, we keep track of the memory and stop caching once a critical amount of memory is used."
         << endl;
    cout << "    Default: 1" << endl << endl;

    cout << "  -task <PROB | MLD>" << endl;
    cout <<
    "    Specifies if the probabilistic task (PROB) or the most likely determinized (MLD) is used."
         << endl;
    cout << "    Default: PROB" << endl << endl;

    cout << "  -sd <int>" << endl;
    cout << "    Specifies the considered horizon." << endl;
    cout << "    Default: Horizon of the task" << endl << endl;

    cout << "  -T <TIME | TRIALS | TIME_AND_TRIALS>" << endl;
    cout <<
    "    Specifies the termination criterion of trials, which can be based on a timeout (TIME), the number of trials (TRIALS) or on both, whichever comes first (TIME_AND_TRIALS). If one of the latter two is used, you MUST set the maximum number of trials."
         << endl;
    cout << "    Default:TIME" << endl;

    cout << "  -t <double>" << endl;
    cout << "    Specifies the timeout limit in seconds." << endl;
    cout << "    Default: 1.0" << endl << endl;

    cout << "  -r <int>" << endl;
    cout <<
    "    Specifies the maximal number of trials. This must be set if used as the default is 0."
         << endl;
    cout << "    Default: 0" << endl << endl;

    cout << "  -ndn <int>" << endl;
    cout <<
    "    Specifies the number of previously unvisited decision nodes that is expanded before the trial length is sufficient."
         << endl;
    cout <<
    "    Default: Horizon of task (i.e., the trial length is only sufficient in states with no remaining steps."
         << endl << endl;

    cout << "  -mcs <double>" << endl;
    cout <<
    "    Specifies the magic constant scale factor (called C_p in the UCT paper of Kocsis and Szepesvari)."
         << endl;
    cout << "    Default: 1.0" << endl << endl;

    cout << " -er <LOG | SQRT | LIN | E.SQRT>" << endl;
    cout <<
    "    Specified the exploration function which should be used, to compute the nominator of the"
    "UCT formula." << endl;
    cout << "    Default: LOG" << endl << endl;

    cout << " -iv <int>" << endl;
    cout <<
    "    Specifies the number of initial visits that is considered. The higher this is, the higher is the weight given to the initialization."
         << endl;
    cout << "    Default: 5" << endl << endl;

    cout << "  -i <SearchEngine>" << endl;
    cout <<
    "    Specifies the search engine that is used for initialization." << endl;
    cout << "    Default: n/a, this MUST be specified." << endl << endl << endl;

    cout << "/*************************************************************" <<
    endl
         << "                          MaxMC-UCT" << endl
         << "*************************************************************/" <<
    endl;

    cout <<
    "An MaxMC-UCT search engine is equivalent to the algorithm described by Keller and Helmert(2013), and is implemented in terms of the Trial-based Heuristic Tree Search framework. It can be created as [MaxMC-UCT <options>] with the following options:"
         << endl << endl;

    cout << "  -uc <0|1>" << endl;
    cout <<
    "    Specifies if caching is used. If this is switched on, we keep track of the memory and stop caching once a critical amount of memory is used."
         << endl;
    cout << "    Default: 1" << endl << endl;

    cout << "  -task <PROB | MLD>" << endl;
    cout <<
    "    Specifies if the probabilistic task (PROB) or the most likely determinized (MLD) is used."
         << endl;
    cout << "    Default: PROB" << endl << endl;

    cout << "  -sd <int>" << endl;
    cout << "    Specifies the considered horizon." << endl;
    cout << "    Default: Horizon of the task" << endl << endl;

    cout << "  -T <TIME | TRIALS | TIME_AND_TRIALS>" << endl;
    cout <<
    "    Specifies the termination criterion of trials, which can be based on a timeout (TIME), the number of trials (TRIALS) or on both, whichever comes first (TIME_AND_TRIALS). If one of the latter two is used, you MUST set the maximum number of trials."
         << endl;
    cout << "    Default:TIME" << endl << endl;

    cout << "  -t <double>" << endl;
    cout << "    Specifies the timeout limit in seconds." << endl;
    cout << "    Default: 1.0" << endl << endl;

    cout << "  -r <int>" << endl;
    cout <<
    "    Specifies the maximal number of trials. This must be set if used as the default is 0."
         << endl;
    cout << "    Default: 0" << endl << endl;

    cout << "  -ndn <int>" << endl;
    cout <<
    "    Specifies the number of previously unvisited decision nodes that is expanded before the trial length is sufficient."
         << endl;
    cout <<
    "    Default: Horizon of task (i.e., the trial length is only sufficient in states with no remaining steps."
         << endl << endl;

    cout << "  -mcs <double>" << endl;
    cout <<
    "    Specifies the magic constant scale factor (called C_p in the UCT paper of Kocsis and Szepesvari)."
         << endl;
    cout << "    Default: 1.0" << endl << endl;

    cout << " -iv <int>" << endl;
    cout <<
    "    Specifies the number of initial visits that is considered. The higher this is, the higher is the weight given to the initialization."
         << endl;
    cout << "    Default: 5" << endl << endl;

    cout << " -hw <double>" << endl;
    cout <<
    "    Specifies the factor applied to the heuristic initialization. The higher, the more is the trust in the heuristic. If lower than 1.0, immediate rewards that have actually been encountered (and not just estimated) are given a higher trust."
         << endl;
    cout << "    Default: 0.5" << endl << endl;

    cout << "  -i <SearchEngine>" << endl;
    cout <<
    "    Specifies the search engine that is used for initialization." << endl;
    cout << "    Default: n/a, this MUST be specified." << endl << endl << endl;

    cout << "/*************************************************************" <<
    endl
         << "                            DP-UCT" << endl
         << "*************************************************************/" <<
    endl;

    cout <<
    "An DP-UCT search engine is equivalent to the algorithm described by Keller and Helmert(2013), and is implemented in terms of the Trial-based Heuristic Tree Search framework. It can be created as [DP-UCT <options>] with the following options:"
         << endl << endl;

    cout << "  -uc <0|1>" << endl;
    cout <<
    "    Specifies if caching is used. If this is switched on, we keep track of the memory and stop caching once a critical amount of memory is used."
         << endl;
    cout << "    Default: 1" << endl << endl;

    cout << "  -task <PROB | MLD>" << endl;
    cout <<
    "    Specifies if the probabilistic task (PROB) or the most likely determinized (MLD) is used."
         << endl;
    cout << "    Default: PROB" << endl << endl;

    cout << "  -sd <int>" << endl;
    cout << "    Specifies the considered horizon." << endl;
    cout << "    Default: Horizon of the task" << endl << endl;

    cout << "  -T <TIME | TRIALS | TIME_AND_TRIALS>" << endl;
    cout <<
    "    Specifies the termination criterion of trials, which can be based on a timeout (TIME), the number of trials (TRIALS) or on both, whichever comes first (TIME_AND_TRIALS). If one of the latter two is used, you MUST set the maximum number of trials."
         << endl;
    cout << "    Default:TIME" << endl << endl;

    cout << "  -t <double>" << endl;
    cout << "    Specifies the timeout limit in seconds." << endl;
    cout << "    Default: 1.0" << endl << endl;

    cout << "  -r <int>" << endl;
    cout <<
    "    Specifies the maximal number of trials. This must be set if used as the default is 0."
         << endl;
    cout << "    Default: 0" << endl << endl;

    cout << "  -ndn <int>" << endl;
    cout <<
    "    Specifies the number of previously unvisited decision nodes that is expanded before the trial length is sufficient."
         << endl;
    cout <<
    "    Default: Horizon of task (i.e., the trial length is only sufficient in states with no remaining steps."
         << endl << endl;

    cout << "  -mcs <double>" << endl;
    cout <<
    "    Specifies the magic constant scale factor (called C_p in the UCT paper of Kocsis and Szepesvari)."
         << endl;
    cout << "    Default: 1.0" << endl << endl;

    cout << " -iv <int>" << endl;
    cout <<
    "    Specifies the number of initial visits that is considered. The higher this is, the higher is the weight given to the initialization."
         << endl;
    cout << "    Default: 5" << endl << endl;

    cout << " -hw <double>" << endl;
    cout <<
    "    Specifies the factor applied to the heuristic initialization. The higher, the more is the trust in the heuristic. If lower than 1.0, immediate rewards that have actually been encountered (and not just estimated) are given a higher trust."
         << endl;
    cout << "    Default: 0.5" << endl << endl;

    cout << "  -i <SearchEngine>" << endl;
    cout <<
    "    Specifies the search engine that is used for initialization." << endl;
    cout << "    Default: n/a, this MUST be specified." << endl << endl << endl;

    cout << "/*************************************************************" <<
    endl
         << "                  Iterative Deepening Search" << endl
         << "*************************************************************/" <<
    endl;

    cout <<
    "An IDS search engine is equivalent to the search engine that is described by Keller and Eyerich (2012) as their initialization method. It can be created as [IDS <options>] with the following options:"
         << endl << endl;

    cout << "  -uc <0|1>" << endl;
    cout <<
    "    Specifies if caching is used. If this is switched on, we keep track of the memory and stop caching once a critical amount of memory is used."
         << endl;
    cout << "    Default: 1" << endl << endl;

    cout << "  -task <PROB | MLD>" << endl;
    cout <<
    "    Specifies if the probabilistic task (PROB) or the most likely determinized (MLD) is used."
         << endl;
    cout << "    Default: MLD" << endl << endl;

    cout << "  -sd <int>" << endl;
    cout << "    Specifies the considered horizon." << endl;
    cout << "    Default: Learned based on timeout and strict timeout." <<
    endl << endl;

    cout << "  -t <double>" << endl;
    cout <<
    "    Specifies the timeout limit in seconds that we try to achieve in the worst case during learning."
         << endl;
    cout << "    Default: 0.005" << endl << endl;

    cout << "  -st <double>" << endl;
    cout <<
    "    Specifies the strict timeout limit in seconds. If any iteration during learning is beyond this, we immediately rule out any iteration depth equal to the current or higher."
         << endl;
    cout << "    Default: 0.1" << endl << endl;

    cout << "  -tra <0|1>" << endl;
    cout <<
    "    Specifies if IDS terminates if an action is found that is better than others (i.e., if the result is informative)."
         << endl;
    cout << "    Default: 1" << endl << endl;

    cout << "  -minsd <int>" << endl;
    cout <<
    "    Specifies the minimal search depth we expect from learning. If learning determines a lower search depth than this, it is set to 0 instead."
         << endl;
    cout << "    Default: 2" << endl << endl << endl;

    cout << "/*************************************************************" <<
    endl
         << "                      Depth First Search" << endl
         << "*************************************************************/" <<
    endl;

    cout <<
    "A DFS search engine is a depth first search engine that has only been tested as part of IDS. It can be created as [DFS <options>] with the following options:"
         << endl << endl;

    cout << "  -uc <0|1>" << endl;
    cout <<
    "    Specifies if caching is used. If this is switched on, we keep track of the memory and stop caching once a critical amount of memory is used."
         << endl;
    cout << "    Default: 1" << endl << endl;

    cout << "  -task <PROB | MLD>" << endl;
    cout <<
    "    Specifies if the probabilistic task (PROB) or the most likely determinized (MLD) is used."
         << endl;
    cout << "    Default: MLD" << endl << endl << endl;

    cout << "/*************************************************************" <<
    endl
         << "                   Uniform Evaluation Search" << endl
         << "*************************************************************/" <<
    endl;

    cout <<
    "Uniform Evaluation Search evaluates each action identical. It can be created as [Uniform <options>] with the following options:"
         << endl << endl;

    cout << "  -task <PROB | MLD>" << endl;
    cout <<
    "    Specifies if the probabilistic task (PROB) or the most likely determinized (MLD) is used. If you run this standalone (as a random search engine), use PROB, otherwise, use DET (to achieve correct pruning behaviour)."
         << endl;
    cout << "    Default: MLD" << endl << endl;

    cout << "  -val <double | INFTY>" << endl;
    cout <<
    "    Specifies the estimate assigned to each action. If this is INFTY, it is set to the maximal reward of the task and is thus a (very simple) admissible initialization."
         << endl << endl << endl;

    cout << "/*************************************************************" <<
    endl
         << "                         Abbreviations" << endl
         << "*************************************************************/" <<
    endl;

    cout <<
    "There are also some short cuts to quickly use search engines that have been used quite often:"
         << endl << endl;
    cout << "  [IPPC2011] := [MC-UCT -sd 15 -i [IDS -sd 15]]" << endl;
    cout <<
    "    This corresponds to the planner that has been used for IPPC 2011 (except that all bugfixes are considered and that the timemanagement framework that had been used is not part of this."
         << endl << endl;

    cout << "  [UCTStar <options>] := [DP-UCT -ndn 1 -iv 1 <options>]" << endl;
    cout <<
    "    This corresponds to the algorithms that has described as UCT* by Keller and Helmert (2013). To obtain the planner from that paper, use -i [IDS] as single further option."
         << endl << endl;

    cout <<
    "  [MaxMC-UCTStar <options>] := [MaxMC-UCT -ndn 1 -iv 1 <options>]" << endl;
    cout <<
    "    This is the equivalent to UCT* with MaxMC-UCT as the base algorithm."
         << endl << endl;
}

int main(int argc, char** argv) {
    Timer totalTime;
    if (argc < 3) {
        printUsage();
        return 1;
    }

/******************************************************************
                      Parse command line
******************************************************************/

    //read non-optionals
    string problemFileName = string(argv[1]);

    //init optionals to default values
    string hostName = "localhost";
    unsigned short port = 2323;

    bool allParamsRead = false;
    string plannerDesc;
    for (unsigned int i = 2; i < argc; ++i) {
        if (!allParamsRead && string(argv[i])[0] == '[') {
            allParamsRead = true;
        }
        if (allParamsRead) {
            plannerDesc += string(argv[i]) + " ";
        } else {
            string nextOption = string(argv[i]);
            if (nextOption == "-h" || nextOption == "--hostname") {
                hostName = string(argv[++i]);
            } else if (nextOption == "-p" || nextOption == "--port") {
                port = (unsigned short) (atoi(string(argv[++i]).c_str()));
            } else {
                cerr << "Unknown option: " << nextOption << endl;
                printUsage();
                return 1;
            }
        }
    }

    map<string, int> stateVariableIndices;
    vector<vector<string> > stateVariableValues;

    Parser parser(problemFileName);
    parser.parseTask(stateVariableIndices, stateVariableValues);

    // Create Prost Planner
    ProstPlanner* planner = new ProstPlanner(plannerDesc);
    planner->init();

    // Create connector to rddlsim and run
    IPPCClient* client =
        new IPPCClient(planner, hostName, port, stateVariableIndices,
                       stateVariableValues);

    client->run(SearchEngine::taskName);

    cout << "PROST complete running time: " << totalTime << endl;
    return 0;
}
