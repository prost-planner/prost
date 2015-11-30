#include "initializer.h"

#include "thts.h"
#include "uniform_evaluation_search.h"

#include "utils/string_utils.h"
#include "utils/system_utils.h"

/******************************************************************
                     Search Engine Creation
******************************************************************/

Initializer* Initializer::fromString(std::string& desc, THTS* thts) {
    StringUtils::trim(desc);
    assert(desc[0] == '[' && desc[desc.size() - 1] == ']');
    StringUtils::removeFirstAndLastCharacter(desc);
    StringUtils::trim(desc);

    Initializer* result = nullptr;

    if (desc.find("Expand") == 0) {
        desc = desc.substr(6, desc.size());
        result = new ExpandNodeInitializer(thts);
    } else {
        SystemUtils::abort("Unknown Initializer: " + desc);
    }

    StringUtils::trim(desc);

    while (!desc.empty()) {
        std::string param;
        std::string value;
        StringUtils::nextParamValuePair(desc, param, value);

        if (!result->setValueFromString(param, value)) {
            SystemUtils::abort("Unused parameter value pair: " +
                               param + " / " + value);
        }
    }
    return result;
}

bool Initializer::setValueFromString(std::string& param, std::string& value) {
    if (param == "-h") {
        setHeuristic(SearchEngine::fromString(value));
        return true;
    }  else if (param == "-hw") {
        setHeuristicWeight(atof(value.c_str()));
        return true;
    } else if (param == "-iv") {
        setNumberOfInitialVisits(atoi(value.c_str()));
        return true;
    } 

    return false;
}

Initializer::~Initializer() {
    delete heuristic;
}

/******************************************************************
                 Search Engine Administration
******************************************************************/

void Initializer::disableCaching() {
    heuristic->disableCaching();
}

void Initializer::learn() {
    heuristic->learn();

    if (heuristic->getMaxSearchDepth() == 0) {
        UniformEvaluationSearch* _heuristic = new UniformEvaluationSearch();
        setHeuristic(_heuristic);
        std::cout << "Search depth is too low for selected heuristic!" << std::endl;
    }
}

/******************************************************************
                          Parameter Setter
******************************************************************/

void Initializer::setHeuristic(SearchEngine* _heuristic) {
    if (heuristic) {
        delete heuristic;
    }
    heuristic = _heuristic;
}

void Initializer::setMaxSearchDepth(int maxSearchDepth) {
    assert(heuristic);
    heuristic->setMaxSearchDepth(maxSearchDepth);
}

/******************************************************************
                            Print
******************************************************************/

void Initializer::printStats(std::ostream& out,
                             bool const& printRoundStats,
                             std::string indent) const {
    out << indent << "Initializer: " << name << std::endl;
    out << indent << "Heuristic weight: " << heuristicWeight << std::endl;
    out << indent << "Number of initial visits: " << numberOfInitialVisits << std::endl;
    out << indent << "Heuristic: " << std::endl;
    heuristic->printStats(out, printRoundStats, indent + "  ");
}

/******************************************************************
                        ExpandNodeInitializer
******************************************************************/

bool ExpandNodeInitializer::isInitialized(SearchNode* node) {
    return !node->children.empty();
}

void ExpandNodeInitializer::initialize(SearchNode* node, State const& current) {
    node->children.resize(SearchEngine::numberOfActions, nullptr);

    // std::cout << "initializing state: " << std::endl;
    // current.print(std::cout);

    std::vector<int> actionsToExpand = thts->getApplicableActions(current);

    std::vector<double> initialQValues(SearchEngine::numberOfActions, 0.0);
    heuristic->estimateQValues(current, actionsToExpand, initialQValues);

    for (unsigned int index = 0; index < node->children.size(); ++index) {
        if (actionsToExpand[index] == index) {
            node->children[index] = thts->getChanceNode(1.0);
            node->children[index]->futureReward =
                heuristicWeight * (double)current.stepsToGo() * initialQValues[index];
            node->children[index]->numberOfVisits = numberOfInitialVisits;

            node->numberOfVisits += numberOfInitialVisits;
            node->futureReward =
                std::max(node->futureReward, node->children[index]->futureReward);

            // std::cout << "Initialized child ";
            // SearchEngine::actionStates[index].printCompact(std::cout);
            // node->children[index]->print(std::cout);
        }
    }
    // std::cout << std::endl;
}

