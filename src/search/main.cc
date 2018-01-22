#include "ippc_client.h"

#include "utils/stopwatch.h"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;

void printUsage() {
    cout << "Usage: ./prost <rddl-parser-output | rddl-problem-name> "
            "[PROST <options>]"
         << endl;

    cout << "**************************************************************"
         << endl
         << "                     PROST Planner Options" << endl
         << "**************************************************************"
         << endl;

    cout << "  -s <double>" << endl;
    cout << "    Specifies the seed." << endl;
    cout << "    Default: time(nullptr)" << endl << endl;

    cout << "  -ram <int>" << endl;
    cout << "    Specifies the RAM limit (in KB) used to disable caching."
         << endl;
    cout << "    Default: 2097152 (i.e. 2 GB)" << endl << endl;

    cout << "  -bit <32 | 64>" << endl;
    cout << "    Specifies the system's bit size, and detects it by default "
            "automatically."
         << endl;
    cout << "    Default: sizeof(long)*8" << endl << endl;

    cout << "  -se <SearchEngine>" << endl;
    cout << "    Specifies the used main search engine." << endl;
    cout << "    MANDATORY." << endl << endl << endl;

    cout << "******************************************************************"
            "***"
         << endl
         << "                         Search Engines" << endl
         << "******************************************************************"
            "***"
         << endl
         << endl;

    cout << "************** Iterative Deepening Search ****************"
         << endl;

    cout << "An IDS search engine is equivalent to the search engine that is "
            "described by Keller and Eyerich (2012) as their initialization "
            "method. It is created by [IDS <options>] with the following "
            "options:"
         << endl
         << endl;

    cout << "  -uc <0|1>" << endl;
    cout << "    Specifies if caching is used. If this is switched on, we keep "
            "track of the memory and stop caching once a critical amount of "
            "memory is used."
         << endl;
    cout << "    Default: 1" << endl << endl;

    cout << "  -sd <int>" << endl;
    cout << "    Specifies the considered horizon." << endl;
    cout << "    Default: Horizon of the task" << endl << endl;

    cout << "  -t <double>" << endl;
    cout << "    Specifies the timeout limit in seconds that we try to achieve "
            "in the worst case during learning."
         << endl;
    cout << "    Default: 0.005" << endl << endl;

    cout << "  -st <double>" << endl;
    cout << "    Specifies the strict timeout limit in seconds. If any "
            "iteration during learning is beyond this, we immediately rule out "
            "any iteration depth equal to the current or higher."
         << endl;
    cout << "    Default: 0.1" << endl << endl;

    cout << "  -tra <0|1>" << endl;
    cout << "    Specifies if IDS terminates if an action is found that is "
            "better than others (i.e., if the result is informative)."
         << endl;
    cout << "    Default: 1" << endl << endl;

    cout << "  -minsd <int>" << endl;
    cout << "    Specifies the minimal search depth we expect from learning. "
            "If learning determines a lower search depth than this, it is set "
            "to 0 instead."
         << endl;
    cout << "    Default: 2" << endl << endl << endl;

    cout << "**************** Depth First Search **********************"
         << endl;

    cout << "A DFS search engine is a depth first search engine that has only "
            "been tested as part of IDS. It is created by [DFS <options>] with "
            "the following options:"
         << endl
         << endl;

    cout << "  -uc <0|1>" << endl;
    cout << "    Specifies if caching is used. If this is switched on, we keep "
            "track of the memory and stop caching once a critical amount of "
            "memory is used."
         << endl;
    cout << "    Default: 1" << endl << endl;

    cout << "***************** Uniform Evaluation Search **************"
         << endl;

    cout << "Uniform Evaluation Search evaluates each action identical. It is "
            "created by [Uniform <options>] with the following options:"
         << endl
         << endl;

    cout << "  -val <double | MAX>" << endl;
    cout << "    Specifies the estimate assigned to each action. If this is "
            "MAX, it is set to the maximal reward of the task and can thereby "
            "be used as a (very simple) admissible initialization."
         << endl
         << endl
         << endl;

    cout << "***************** Minimal Lookahead Search **************" << endl;

    cout << "Minimal Lookahead Search performs the minimal possible, "
            "informative lookahead in the given task. It is created by [MLS "
            "<options>] with the following options:"
         << endl
         << endl;

    cout << "  -uc <0|1>" << endl;
    cout << "    Specifies if caching is used. If this is switched on, we keep "
            "track of the memory and stop caching once a critical amount of "
            "memory is used."
         << endl;
    cout << "    Default: 1" << endl << endl;

    cout << "************************** THTS **************************"
         << endl;

    cout << "THTS is the framework that is described in our ICAPS 2013 paper "
            "(Trial-based Heuristic Tree Search for Finite Horizon MDPs) and "
            "in my PhD thesis. A THTS algorithm is specified in terms of six "
            "ingredients: action selection, outcome selection, backup "
            "function, trial length, initialization, and recommendation "
            "function. The first three are specified with an object of the "
            "corresponding type, and the latter three are described with "
            "parameters of THTS. It is created by [THTS <options>] with the "
            "following options:"
         << endl
         << endl;

    cout << "  -act <ActionSelection>" << endl;
    cout << "    Specifies the used action selection (available options are "
            "given below)."
         << endl;
    cout << "    MANDATORY" << endl << endl;

    cout << "  -out <OutcomeSelection>" << endl;
    cout << "    Specifies the used outcome selection (available options are "
            "given below)."
         << endl;
    cout << "    MANDATORY" << endl << endl;

    cout << "  -backup <BackupFunction>" << endl;
    cout << "    Specifies the used backup function (available options are "
            "given below)."
         << endl;
    cout << "    MANDATORY" << endl << endl;

    cout << "  -init <Initializer>" << endl;
    cout << "    Specifies the used initializer (available options are given "
            "below)."
         << endl;
    cout << "    MANDATORY" << endl << endl;

    cout << "  -rec <RecommendationFunction>" << endl;
    cout << "    Specifies the used recommendation function (available options "
            "are given below)."
         << endl;
    cout << "    Default: [EBA]" << endl << endl;

    cout << "  -uc <0|1>" << endl;
    cout << "    Specifies if caching is used. If this is switched on, we keep "
            "track of the memory and stop caching once a critical amount of "
            "memory is used."
         << endl;
    cout << "    Default: 1" << endl << endl;

    cout << "  -rld <0|1>" << endl;
    cout << "    Specifies if reward lock detection is used." << endl;
    cout << "    Default: As learned approximately in the training phase"
         << endl
         << endl;

    cout << "  -crl <0|1>" << endl;
    cout << "    Specifies if reward lock caching with BDDs as described in "
            "our ICAPS 2013 paper is used. If reward lock detection is not "
            "used, this parameter has no influence on the planner."
         << endl;
    cout << "    Default: 1" << endl << endl;

    cout << "  -node-limit <int>" << endl;
    cout << "    Specifies the maximal number of search nodes that is used by "
            "the THTS algorithm."
         << endl;
    cout << "    Default: 24000000" << endl << endl;

    cout << "  -sd <int>" << endl;
    cout << "    Specifies the considered horizon." << endl;
    cout << "    Default: Horizon of the task" << endl << endl;

    cout << "  -T <TIME | TRIALS | TIME_AND_TRIALS>" << endl;
    cout << "    Specifies the termination criterion of trials, which can be "
            "based on a timeout (TIME), the number of trials (TRIALS) or on "
            "both, whichever comes first (TIME_AND_TRIALS). If one of the "
            "latter two is used, you MUST set the maximum number of trials."
         << endl;
    cout << "    Default:TIME" << endl << endl;

    cout << "  -t <double>" << endl;
    cout << "    Specifies the timeout limit in seconds (if TIME of "
            "TIME_AND_TRIALS is the termination criterion)."
         << endl;
    cout << "    Default: 1.0" << endl << endl;

    cout << "  -r <int>" << endl;
    cout << "    Specifies the maximal number of trials (if TRIALS is the "
            "termination criterion)."
         << endl;
    cout << "    Default: 0" << endl << endl;

    cout << "  -ndn <int|H>" << endl;
    cout << "    This is the parameter that describes the trial length "
            "ingredient. It specifies the number of previously unvisited "
            "decision nodes that is expanded before the trial length is "
            "considered sufficient."
         << endl;
    cout << "    Default: Horizon of task" << endl << endl;

    cout << "  -mv <0|1>" << endl;
    cout << "    This is the parameter that describes the recommendation "
            "function: if this is set to 0, the action with the highest "
            "action-value estimate is executed, and otherwise the one that has "
            "been visited most often."
         << endl;
    cout << "    Default: 0" << endl << endl;

    cout << "******************************************************************"
            "**"
         << endl
         << "                           Action Selections" << endl
         << "******************************************************************"
            "**"
         << endl
         << endl;

    cout << "************************** UCB1 **************************"
         << endl;

    cout << "UCB1 is the action selection that is described by Auer, "
            "Ceas-Bianchi & Fischer (2002) in the context of Multi-armed "
            "Bandit Problems. It is also the action selection method that is "
            "used in UCT (Kocsis & Szepesvari, 2006). It is created by [UCB1 "
            "<options>] with the following options:"
         << endl;

    cout << "  -lvar <0|1>" << endl;
    cout << "    Specifies if the least visited action is selected in the root "
            "node."
         << endl;
    cout << "    Default: 0" << endl << endl;

    cout << "  -mvd <int>" << endl;
    cout << "    Specifies the factor of the number of visited between the "
            "most and least selected action (the least visited is selected if "
            "the factor gets higher than this)."
         << endl;
    cout << "    Default: 50" << endl << endl;

    cout << "  -mcs <double>" << endl;
    cout << "    Specifies the magic constant scale factor (called C_p in the "
            "UCT paper of Kocsis and Szepesvari)."
         << endl;
    cout << "    Default: 1.0" << endl << endl;

    cout << " -er <LOG | SQRT | LIN | LNQUAD>" << endl;
    cout << "    Specifies the exploration function that is used to compute "
            "the nominator of the UCB1 formula."
         << endl;
    cout << "    Default: LOG" << endl << endl;

    cout << "************************** BFS ***************************"
         << endl;

    cout << " BFS selects one of the actions among the least selected ones "
            "uniformly at random. It is created by [BFS] and has no options."
         << endl
         << endl;

    cout << "******************************************************************"
            "**"
         << endl
         << "                         Outcome Selections" << endl
         << "******************************************************************"
            "**"
         << endl
         << endl;

    cout << "*************************** MC ***************************"
         << endl;

    cout << "Monte-Carlo outcome selection samples each outcome according to "
            "its probability. It is created by [MC] and has no options."
         << endl
         << endl;

    cout << "*************************** UMC **************************"
         << endl;

    cout << "Monte-Carlo outcome selection that only considers actions that "
            "are not labeled as solved. It is created by [UMC] and has no "
            "options."
         << endl
         << endl;

    cout << "******************************************************************"
            "**"
         << endl
         << "                           Backup Functions" << endl
         << "******************************************************************"
            "**"
         << endl
         << endl;

    cout << "*************************** MC ***************************"
         << endl;

    cout << "In the Monte-Carlo backup function, each visited node is updated "
            "by extending the current estimate such that it reflects the "
            "average over all samples. It is created by [MC <options>] with "
            "the following options:"
         << endl;

    cout << "  -ilr <double>" << endl;
    cout << "    Specifies the initial learning rate (compare, for instance, "
            "with the reinforcmenet learning book by Sutton & Barto)."
         << endl;
    cout << "    Default: 1.0" << endl << endl;

    cout << "  -lrd <double>" << endl;
    cout << "    Specifies the learning rate decay (compare, for instance, "
            "with the reinforcmenet learning book by Sutton & Barto)."
         << endl;
    cout << "    Default: 1.0" << endl << endl;

    cout << "************************** MaxMC **************************"
         << endl;

    cout << "The MaxMonte-Carlo backup function is just like MC, except that "
            "decision nodes are updated by maximization over all actions. It "
            "is created by [MaxMC] and has no options."
         << endl
         << endl;

    cout << "/*************************** PB ***************************"
         << endl;

    cout << "The PB backup function is a partial variant of the Bellman backup "
            "function that can be applied even if some chance node successors "
            "of a decision node are not yet explicated.  It is created by [PB] "
            "and has no options."
         << endl
         << endl;

    cout << "******************************************************************"
            "**"
         << endl
         << "                           Initializers" << endl
         << "******************************************************************"
            "**"
         << endl
         << endl;

    cout << "*********************** Expand Node **********************"
         << endl;

    cout << "The Expand Node initializer expands the current decision node by "
            "creating and intializing a child node for each applicable action. "
            "It is created by [Expand <options>] with the following options:"
         << endl;

    cout << "  -h <SearchEngine>" << endl;
    cout << "    This is the search engine that is used to compute the "
            "heuristic values that are used for initialization."
         << endl;
    cout << "    MANDATORY." << endl << endl << endl;

    cout << "  -iv <int>" << endl;
    cout << "    Specifies the number of initial visits that is considered. "
            "The higher this is, the higher is the weight given to the "
            "initialization."
         << endl;
    cout << "    Default: 1" << endl << endl;

    cout << "  -hw <double>" << endl;
    cout << "    Specifies the factor applied to the heuristic initialization. "
            "The higher, the more is the trust in the heuristic. If lower than "
            "1.0, immediate rewards that have actually been encountered (and "
            "not just estimated) are given a higher trust."
         << endl;
    cout << "    Default: 0.5" << endl << endl;

    cout << "*********************** Single Child **********************"
         << endl;

    cout << "The Single Child initializer selects a single, uninitialized "
            "child of the current decision node at random and initializes it. "
            "It is created by [Single] with the following options:"
         << endl;

    cout << "  -h <SearchEngine>" << endl;
    cout << "    This is the search engine that is used to compute the "
            "heuristic values that are used for initialization."
         << endl;
    cout << "    MANDATORY." << endl << endl << endl;

    cout << "  -iv <int>" << endl;
    cout << "    Specifies the number of initial visits that is considered. "
            "The higher this is, the higher is the weight given to the "
            "initialization."
         << endl;
    cout << "    Default: 1" << endl << endl;

    cout << "  -hw <double>" << endl;
    cout << "    Specifies the factor applied to the heuristic initialization. "
            "The higher, the more is the trust in the heuristic. If lower than "
            "1.0, immediate rewards that have actually been encountered (and "
            "not just estimated) are given a higher trust."
         << endl;
    cout << "    Default: 0.5" << endl << endl;

    cout << "******************************************************************"
            "**"
         << endl
         << "                     Recommendation Functions" << endl
         << "******************************************************************"
            "**"
         << endl
         << endl;

    cout << "*********************** Expected Best Arm **********************"
         << endl;

    cout << "The Expected Best Arm recommendation function recommends the "
            "action with the highest action-value estimate. It is created by "
            "[EBA] and has no options."
         << endl;

    cout << "*********************** Most Played Arm **********************"
         << endl;

    cout << "The Most Played Arm recommendation function recommends the action "
            "that has been selected most often in the root node during "
            "simulation. It is created by [MPA] and has no options."
         << endl;

    cout << "******************************************************************"
            "**"
         << endl
         << "                             Abbreviations" << endl
         << "******************************************************************"
            "**"
         << endl;

    cout << "There are also some shortcuts for popular search engines:" << endl
         << endl;
    cout << "  [IPPC2011] := [THTS -act [UCB1] -out [MC] -backup [MC] -init "
            "[Expand -h [IDS -sd 15] -iv 5 -hw 1.0] -ndn H -sd 15]"
         << endl;
    cout << "    The PROST configuration that won IPPC 2011." << endl << endl;

    cout << "  [IPPC2014] := [THTS -act [UCB1] -out [UMC] -backup [PB] -init "
            "[Expand -h [IDS]]]"
         << endl;
    cout << "    The PROST configuration that won IPPC 2014." << endl << endl;

    cout << "  [UCTStar <options>] := [THTS -act [UCB1] -out [UMC] -backup "
            "[PB] <options>]"
         << endl;
    cout << "    The algorithm that has been introduced as UCT* in Keller and "
            "Helmert (ICAPS 2013). To obtain the configuration that has been "
            "used for the described experiment, use '-init [Expand -h [IDS]]' "
            "as additional option."
         << endl
         << endl;

    cout << "  [DP-UCT <options>] := [THTS -act [UCB1] -out [UMC] -backup [PB] "
            "-ndn H <options>]"
         << endl;
    cout << "    The algorithm that has been introduced as DP-UCT in Keller "
            "and Helmert (ICAPS 2013). To obtain the configuration that has "
            "been used for the described experiment, use '-init [Expand -h "
            "[IDS]]' as additional option."
         << endl
         << endl;

    cout << "  [MaxUCT <options>] := [THTS -act [UCB1] -out [MC] -backup [PB] "
            "<options>]"
         << endl;
    cout << "    The algorithm that has been introduced as MaxUCT in Keller "
            "and Helmert (ICAPS 2013). To obtain the configuration that has "
            "been used for the described experiment, use '-init [Expand -h "
            "[IDS]]' and '-ndn H' as additional options."
         << endl
         << endl;

    cout << "  [UCT <options>] := [THTS -act [UCB1] -out [MC] -backup [MC] "
            "<options>]"
         << endl;
    cout << "    The famous UCT algorithm of Kocsis and Szepesvari (ECML "
            "2006). To obtain the most common UCT variant that performs a "
            "random walk as default policy, use '-init [Single -h "
            "[RandomWalk]]' as additional option."
         << endl
         << endl;

    cout << "  [BFS <options>] := [THTS -act [BFS] -out [UMC] -backup [PB] "
            "<options>]"
         << endl;
    cout << "    A breadth-first search algorithm in the THTS framework."
         << endl
         << endl;
}

int main(int argc, char** argv) {
    Stopwatch totalTime;
    if (argc < 3) {
        printUsage();
        return 1;
    }

    /******************************************************************
                          Parse command line
    ******************************************************************/

    // read non-optionals
    string problemFileName = string(argv[1]);

    // init optionals to default values
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
                port = (unsigned short)(atoi(string(argv[++i]).c_str()));
            } else {
                cerr << "Unknown option: " << nextOption << endl;
                printUsage();
                return 1;
            }
        }
    }

    // Create connector to rddlsim and run
    IPPCClient* client = new IPPCClient(hostName, port);
    client->run(problemFileName, plannerDesc);

    cout << "PROST complete running time: " << totalTime << endl;
    return 0;
}
