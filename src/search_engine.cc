#include "search_engine.h"

#include "mc_uct_search.h"
#include "max_mc_uct_search.h"
#include "dp_uct_search.h"

#include "iterative_deepening_search.h"
#include "depth_first_search.h"
#include "random_search.h"

#include "prost_planner.h"
#include "actions.h"

#include "utils/math_utils.h"
#include "utils/string_utils.h"
#include "utils/system_utils.h"


using namespace std;

SearchEngine* SearchEngine::fromString(string& desc, ProstPlanner* planner) {
    StringUtils::trim(desc);
    assert(desc[0] == '[' && desc[desc.size()-1] == ']');
    StringUtils::removeFirstAndLastCharacter(desc);
    StringUtils::trim(desc);

    // Check if a shortcut description has been used. TODO: Implement
    // this in a clean and extendible way!
    if(desc.find("IPPC2011") == 0) {
        desc = desc.substr(8,desc.size());
        desc = "MC-UCT -sd 15 -i [IDS -sd 15]" + desc;
    } else if(desc.find("UCTStar") == 0) {
        desc = desc.substr(7,desc.size());
        desc = "DP-UCT -ndn 2" + desc;
    } else if(desc.find("MaxMC-UCTStar") == 0) {
        desc = desc.substr(13,desc.size());
        desc = "MaxMC-UCT -ndn 2" + desc;
    }

    SearchEngine* result = NULL;

    if(desc.find("MC-UCT") == 0) {
        desc = desc.substr(6,desc.size());
        result = new MCUCTSearch(planner);
    } else if(desc.find("MaxMC-UCT") == 0) {
        desc = desc.substr(9,desc.size());
        result = new MaxMCUCTSearch(planner);
    } else if(desc.find("DP-UCT") == 0) { 
        desc = desc.substr(6,desc.size());
        result = new DPUCTSearch(planner);
    } else if(desc.find("IDS") == 0) {
        desc = desc.substr(3,desc.size());
        result = new IterativeDeepeningSearch(planner);
    } else if(desc.find("DFS") == 0) {
        desc = desc.substr(3,desc.size());
        result = new DepthFirstSearch(planner);
    } else if(desc.find("RAND") == 0) {
        desc = desc.substr(4,desc.size());
        result = new RandomSearch(planner);
    } else {
        cout << "Unknown Search Engine: " << desc << endl;
        assert(false);
    }

    StringUtils::trim(desc);

    while(!desc.empty()) {
        string param;
        string value;
        StringUtils::nextParamValuePair(desc,param,value);

        if(!result->setValueFromString(param, value)) {
            SystemUtils::abort("Unused parameter value pair: " + param + " / " + value);
        }
    }
    return result;
}

bool SearchEngine::setValueFromString(string& param, string& value) {
    if(param == "-uc") {
        setCachingEnabled(atoi(value.c_str()));
        return true;
    } else if(param == "-task") {
        if(value == "PROB") {
            setPlanningTask(planner->getProbabilisticTask());
            return true;
        } else if(value == "MLD") {
            setPlanningTask(planner->getDeterministicTask());
            return true;
        } else {
            return false;
        }
    } else if(param == "-sd") {
        setMaxSearchDepth(atoi(value.c_str()));
        return true;
    }

    return false;
}

bool SearchEngine::learn(std::vector<State> const& trainingSet) {
    if(!task->learningFinished()) {
        return false;
    }
    cout << name << ": learning..." << endl;
    bool res = LearningComponent::learn(trainingSet);
    cout << name << ": ...finished" << endl;
    return res;
}

void SearchEngine::print(ostream& out) {
    out << outStream.str() << endl;
    outStream.str("");
}

void SearchEngine::printStats(ostream& out, bool const& /*printRoundStats*/, string indent) {
    out << indent << "Statistics of " << name << ":" << endl;
}

