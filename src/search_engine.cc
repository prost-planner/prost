#include "search_engine.h"

#include "prost_planner.h"
#include "planning_task.h"
#include "actions.h"

#include "mc_uct_search.h"
#include "max_mc_uct_search.h"
#include "dp_uct_search.h"

#include "iterative_deepening_search.h"
#include "depth_first_search.h"
#include "uniform_evaluation_search.h"

#include "utils/math_utils.h"
#include "utils/string_utils.h"
#include "utils/system_utils.h"

using namespace std;

/******************************************************************
                     Search Engine Creation
******************************************************************/

SearchEngine::SearchEngine(std::string _name, ProstPlanner* _planner, bool useProbabilisticTask) :
    CachingComponent(_planner),
    LearningComponent(_planner),
    name(_name),
    planner(_planner), 
    cachingEnabled(true) {

    useProbabilisticPlanningTask(useProbabilisticTask);
    maxSearchDepth = successorGenerator->getHorizon();
}

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
        desc = "DP-UCT -ndn 1 -iv 1" + desc;
    } else if(desc.find("MaxMC-UCTStar") == 0) {
        desc = desc.substr(13,desc.size());
        desc = "MaxMC-UCT -ndn 1" + desc;
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
    } else if(desc.find("Uniform") == 0) {
        desc = desc.substr(7,desc.size());
        result = new UniformEvaluationSearch(planner);
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
    } else if(param == "-prob") {
        useProbabilisticPlanningTask(atoi(value.c_str()));
        return true;
    } else if(param == "-sd") {
        setMaxSearchDepth(atoi(value.c_str()));
        return true;
    }

    return false;
}

/******************************************************************
                            Parameter
******************************************************************/

void SearchEngine::useProbabilisticPlanningTask(bool const& useProbabilisticTask) {
    if(useProbabilisticTask) {
        successorGenerator = planner->getProbabilisticTask();
        if(successorGenerator->isPruningEquivalentToDeterminization()) {
            applicableActionGenerator = planner->getDeterministicTask();
        } else {
            applicableActionGenerator = planner->getProbabilisticTask();
        }
    } else {
        successorGenerator = planner->getDeterministicTask();
        applicableActionGenerator = planner->getDeterministicTask();
    }
}

/******************************************************************
                            Learning
******************************************************************/

bool SearchEngine::learn(std::vector<State> const& trainingSet) {
    if(!successorGenerator->learningFinished() || !applicableActionGenerator->learningFinished()) {
        cout << "damn" << endl;
        return false;
    }
    cout << name << ": learning..." << endl;
    bool res = LearningComponent::learn(trainingSet);
    cout << name << ": ...finished" << endl;
    return res;
}

/******************************************************************
                       Main Search Functions
******************************************************************/

bool SearchEngine::estimateBestActions(State const& _rootState, std::vector<int>& bestActions) {
    vector<double> qValues(applicableActionGenerator->getNumberOfActions());
    vector<int> actionsToExpand = applicableActionGenerator->getApplicableActions(_rootState);

    if(!estimateQValues(_rootState, actionsToExpand, qValues)) {
        return false;
    }

    double stateValue = -numeric_limits<double>::max();    
    for(unsigned int actionIndex = 0; actionIndex < qValues.size(); ++actionIndex) {
        if(actionsToExpand[actionIndex] == actionIndex) {
            if(MathUtils::doubleIsGreater(qValues[actionIndex], stateValue)) {
                stateValue = qValues[actionIndex];
                bestActions.clear();
                bestActions.push_back(actionIndex);
            } else if(MathUtils::doubleIsEqual(qValues[actionIndex], stateValue)) {
                bestActions.push_back(actionIndex);
            }
        }
    }
    return true;
}

bool SearchEngine::estimateStateValue(State const& _rootState, double& stateValue) {
    vector<double> qValues(applicableActionGenerator->getNumberOfActions());
    vector<int> actionsToExpand = applicableActionGenerator->getApplicableActions(_rootState);

    if(!estimateQValues(_rootState, actionsToExpand, qValues)) {
        return false;
    }

    stateValue = -numeric_limits<double>::max();
    for(unsigned int actionIndex = 0; actionIndex < qValues.size(); ++actionIndex) {
        if((actionsToExpand[actionIndex] == actionIndex) && MathUtils::doubleIsGreater(qValues[actionIndex], stateValue)) {
            stateValue = qValues[actionIndex];
        }
    }
    return true;
}

/******************************************************************
                   Statistics and Printers
******************************************************************/

void SearchEngine::print(ostream& out) {
    out << outStream.str() << endl;
    outStream.str("");
}

void SearchEngine::printStats(ostream& out, bool const& /*printRoundStats*/, string indent) {
    out << indent << "Statistics of " << name << ":" << endl;
}

