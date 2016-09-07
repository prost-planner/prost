#include "initializer.h"

#include "thts.h"
#include "uniform_evaluation_search.h"

#include "utils/math_utils.h"
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
    } else if (desc.find("Single") == 0) {
        desc = desc.substr(6, desc.size());
        result = new SingleChildInitializer(thts);
    } else {
        SystemUtils::abort("Unknown Initializer: " + desc);
    }

    StringUtils::trim(desc);

    while (!desc.empty()) {
        std::string param;
        std::string value;
        StringUtils::nextParamValuePair(desc, param, value);

        if (!result->setValueFromString(param, value)) {
            SystemUtils::abort("Unused parameter value pair: " + param + " / " +
                               value);
        }
    }
    return result;
}

bool Initializer::setValueFromString(std::string& param, std::string& value) {
    if (param == "-h") {
        setHeuristic(SearchEngine::fromString(value));
        return true;
    } else if (param == "-hw") {
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
        std::cout << "Search depth is too low for selected heuristic!"
                  << std::endl;
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

void Initializer::printStats(std::ostream& out, bool const& printRoundStats,
                             std::string indent) const {
    out << indent << "Initializer: " << name << std::endl;
    out << indent << "Heuristic weight: " << heuristicWeight << std::endl;
    out << indent << "Number of initial visits: " << numberOfInitialVisits
        << std::endl;
    out << indent << "Heuristic: " << std::endl;
    heuristic->printStats(out, printRoundStats, indent + "  ");
}

/******************************************************************
                        ExpandNodeInitializer
******************************************************************/

void ExpandNodeInitializer::initialize(SearchNode* node, State const& current) {
    // std::cout << "initializing state: " << std::endl;
    // current.print(std::cout);

    assert(node->children.empty());
    node->children.resize(SearchEngine::numberOfActions, nullptr);

    std::vector<int> actionsToExpand = thts->getApplicableActions(current);
    std::vector<double> initialQValues(SearchEngine::numberOfActions,
                                       -std::numeric_limits<double>::max());
    heuristic->estimateQValues(current, actionsToExpand, initialQValues);

    for (unsigned int index = 0; index < node->children.size(); ++index) {
        if (actionsToExpand[index] == index) {
            node->children[index] = thts->createChanceNode(1.0);
            node->children[index]->futureReward =
                heuristicWeight * initialQValues[index];
            node->children[index]->numberOfVisits = numberOfInitialVisits;
            node->children[index]->initialized = true;

            node->numberOfVisits += numberOfInitialVisits;
            node->futureReward = std::max(node->futureReward,
                                          node->children[index]->futureReward);

            // std::cout << "Initialized child ";
            // SearchEngine::actionStates[index].printCompact(std::cout);
            // node->children[index]->print(std::cout);
        }
    }
    // std::cout << std::endl;

    node->initialized = true;
}

/******************************************************************
                        ExpandNodeInitializer
******************************************************************/

void SingleChildInitializer::initialize(SearchNode* node,
                                        State const& current) {
    // std::cout << "initializing state: " << std::endl;
    // current.print(std::cout);

    std::vector<int> candidates;

    if (node->children.empty()) {
        node->children.resize(SearchEngine::numberOfActions, nullptr);

        std::vector<int> actionsToExpand = thts->getApplicableActions(current);
        for (unsigned int index = 0; index < node->children.size(); ++index) {
            if (actionsToExpand[index] == index) {
                node->children[index] = thts->createChanceNode(1.0);
                candidates.push_back(index);
            }
        }
    } else {
        for (unsigned int index = 0; index < node->children.size(); ++index) {
            if (node->children[index] &&
                MathUtils::doubleIsMinusInfinity(
                    node->children[index]->futureReward)) {
                candidates.push_back(index);
            }
        }
    }

    assert(!candidates.empty());
    int actionIndex = MathUtils::rnd->randomElement(candidates);

    double initialQValue = 0.0;
    heuristic->estimateQValue(current, actionIndex, initialQValue);

    node->children[actionIndex]->futureReward = heuristicWeight * initialQValue;
    node->children[actionIndex]->numberOfVisits = numberOfInitialVisits;
    node->children[actionIndex]->initialized = true;
    node->numberOfVisits += numberOfInitialVisits;
    node->futureReward =
        std::max(node->futureReward, node->children[actionIndex]->futureReward);

    node->initialized = (candidates.size() == 1);

    // std::cout << "Initialized child ";
    // SearchEngine::actionStates[actionIndex].printCompact(std::cout);
    // node->children[actionIndex]->print(std::cout);
    // std::cout << std::endl;
}
