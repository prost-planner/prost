#include "search_engine.h"

#include "uct_search.h"
#include "iterative_deepening_search.h"
#include "depth_first_search.h"
#include "random_search.h"

#include "prost_planner.h"
#include "actions.h"

#include "utils/math_utils.h"
#include "utils/string_utils.h"
#include "utils/system_utils.h"

using namespace std;

SearchEngine* SearchEngine::fromString(string& desc, ProstPlanner* planner, vector<double>& searchEngineResult) {
    StringUtils::trim(desc);
    assert(desc[0] == '[' && desc[desc.size()-1] == ']');
    StringUtils::removeFirstAndLastCharacter(desc);
    StringUtils::trim(desc);

    SearchEngine* result = NULL;

    if(desc.find("UCT") == 0) {
        desc = desc.substr(3,desc.size());
        result = new UCTSearch(planner, searchEngineResult);
    } else if(desc.find("IDS") == 0) {
        desc = desc.substr(3,desc.size());
        result = new IterativeDeepeningSearch(planner, searchEngineResult);
    } else if(desc.find("DFS") == 0) {
        desc = desc.substr(3,desc.size());
        result = new DepthFirstSearch(planner, searchEngineResult);
    } else if(desc.find("RAND") == 0) {
        desc = desc.substr(4,desc.size());
        result = new RandomSearch(planner, searchEngineResult);
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

void SearchEngine::run(State const& _rootState) {
    rootState.setTo(_rootState);
    if(rootState.remainingSteps() > maxSearchDepth) {
        rootState.remainingSteps() = maxSearchDepth;
    }
    _run();
}

void SearchEngine::print() {
    cout << outStream.str() << endl;
    outStream.str("");
}

void SearchEngine::printStats(string indent) {
    outStream << indent << "Statisics of " << name << ":" << endl;
}

