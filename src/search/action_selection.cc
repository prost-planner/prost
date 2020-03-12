#include "action_selection.h"

#include <cassert>

#include "thts.h"

#include "utils/logger.h"
#include "utils/math_utils.h"
#include "utils/string_utils.h"
#include "utils/system_utils.h"

/******************************************************************
                     Action Selection Creation
******************************************************************/

ActionSelection* ActionSelection::fromString(std::string& desc, THTS* thts) {
    StringUtils::trim(desc);
    assert(desc[0] == '[' && desc[desc.size() - 1] == ']');
    StringUtils::removeFirstAndLastCharacter(desc);
    StringUtils::trim(desc);

    ActionSelection* result = nullptr;

    if (desc.find("BFS") == 0) {
        desc = desc.substr(3, desc.size());

        result = new BFSActionSelection(thts);
    } else if (desc.find("UCB1") == 0) {
        desc = desc.substr(4, desc.size());

        result = new UCB1ActionSelection(thts);
    } else {
        SystemUtils::abort("Unknown Action Selection: " + desc);
    }

    assert(result);
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

void ActionSelection::initStep(State const& current) {
    stepsToGoInCurrentState = current.stepsToGo();
    currentRootNode = thts->getCurrentRootNode();

    // Reset per step statistics
    numExplorationInRoot = 0;
    numExploitationInRoot = 0;
}

void ActionSelection::finishStep() {
    if (stepsToGoInCurrentState == SearchEngine::horizon) {
        percentageExplorationInInitialState =
            static_cast<double>(numExplorationInRoot) /
            static_cast<double>(numExplorationInRoot + numExploitationInRoot);
    }
}

/******************************************************************
                          ActionSelection
******************************************************************/

int ActionSelection::selectAction(SearchNode* node) {
    bestActionIndices.clear();

    // Select an unvisited child if there is one
    // TODO: Determine why this is <= 1 instead of < 1
    if (node->numberOfVisits <= 1) {
        selectGreedyAction(node);
    }

    // If action selection is uniform in the root node and this is the
    // root node, select the least visited action
    if (selectLeastVisitedActionInRoot && (node == currentRootNode) &&
        bestActionIndices.empty()) {
        selectLeastVisitedAction(node);
    }

    // Select an action has been selected significantly less often than another
    if (bestActionIndices.empty()) {
        selectActionBasedOnVisitDifference(node);
    }

    // Select an action with the action selection method of this
    // action selection
    if (bestActionIndices.empty()) {
        _selectAction(node);
    }

    // Pick one of the candidates uniformly at random
    assert(!bestActionIndices.empty());
    int selectedIndex = MathUtils::rnd->randomElement(bestActionIndices);

    // Update statistics
    if (node == currentRootNode) {
        double bestValue =
            node->children[selectedIndex]->getExpectedRewardEstimate();

        for (SearchNode* child : node->children) {
            if (child && MathUtils::doubleIsGreater(
                             child->getExpectedRewardEstimate(), bestValue)) {
                ++numExplorationInRoot;
                return selectedIndex;
            }
        }
        ++numExploitationInRoot;
    }

    return selectedIndex;
}

inline void ActionSelection::selectGreedyAction(SearchNode* node) {
    double bestValue = -std::numeric_limits<double>::max();

    for (unsigned int childIndex = 0; childIndex < node->children.size();
         ++childIndex) {
        if (node->children[childIndex] &&
            node->children[childIndex]->initialized) {
            if (MathUtils::doubleIsGreater(
                    node->children[childIndex]->getExpectedRewardEstimate(),
                    bestValue)) {
                bestActionIndices.clear();
                bestActionIndices.push_back(childIndex);
                bestValue =
                    node->children[childIndex]->getExpectedRewardEstimate();
            } else if (MathUtils::doubleIsEqual(
                           node->children[childIndex]
                               ->getExpectedRewardEstimate(),
                           bestValue)) {
                bestActionIndices.push_back(childIndex);
            }
        }
    }
}

void ActionSelection::selectLeastVisitedAction(SearchNode* node) {
    int leastVisits = std::numeric_limits<int>::max();

    for (unsigned int childIndex = 0; childIndex < node->children.size();
         ++childIndex) {
        if (node->children[childIndex] &&
            node->children[childIndex]->initialized &&
            !node->children[childIndex]->solved) {
            if (node->children[childIndex]->numberOfVisits < leastVisits) {
                leastVisits = node->children[childIndex]->numberOfVisits;
                bestActionIndices.clear();
                bestActionIndices.push_back(childIndex);
            } else if (node->children[childIndex]->numberOfVisits ==
                       leastVisits) {
                bestActionIndices.push_back(childIndex);
            }
        }
    }
}

inline void ActionSelection::selectRandomAction(SearchNode* node) {
    for (unsigned int childIndex = 0; childIndex < node->children.size();
         ++childIndex) {
        if (node->children[childIndex] &&
            node->children[childIndex]->initialized &&
            !node->children[childIndex]->solved) {
            bestActionIndices.push_back(childIndex);
        }
    }
}

inline void ActionSelection::selectActionBasedOnVisitDifference(
    SearchNode* node) {
    int smallestNumVisits = std::numeric_limits<int>::max();
    int highestNumVisits = 0;

    for (unsigned int childIndex = 0; childIndex < node->children.size();
         ++childIndex) {
        if (node->children[childIndex] &&
            node->children[childIndex]->initialized &&
            !node->children[childIndex]->solved) {
            if (node->children[childIndex]->numberOfVisits <
                smallestNumVisits) {
                bestActionIndices.clear();
                bestActionIndices.push_back(childIndex);
                smallestNumVisits = node->children[childIndex]->numberOfVisits;
            } else if (node->children[childIndex]->numberOfVisits ==
                       smallestNumVisits) {
                bestActionIndices.push_back(childIndex);
            }

            if (node->children[childIndex]->numberOfVisits > highestNumVisits) {
                highestNumVisits = node->children[childIndex]->numberOfVisits;
            }
        }
    }

    if ((maxVisitDiff * smallestNumVisits) >= highestNumVisits) {
        bestActionIndices.clear();
    }
}

/******************************************************************
                              UCB1
******************************************************************/

void UCB1ActionSelection::_selectAction(SearchNode* node) {
    double magicConstant;

    if (MathUtils::doubleIsMinusInfinity(
            node->getExpectedFutureRewardEstimate()) ||
        MathUtils::doubleIsEqual(node->getExpectedFutureRewardEstimate(),
                                 0.0)) {
        magicConstant = 100.0;
    } else {
        magicConstant = magicConstantScaleFactor *
                        std::abs(node->getExpectedFutureRewardEstimate());
    }
    assert(node->numberOfVisits > 0);

    double bestUCTValue = -std::numeric_limits<double>::max();
    double parentVisitPart = 1.0; // 2.0

    switch (explorationRate) {
    case SQRT:
        parentVisitPart *= std::sqrt((double)node->numberOfVisits);
        break;
    case LIN:
        parentVisitPart *= node->numberOfVisits;
        break;
    case LNQUAD: {
        double logPart = std::log((double)node->numberOfVisits);
        parentVisitPart *= logPart * logPart;
        break;
    }
    case LOG:
        parentVisitPart *= std::log((double)node->numberOfVisits);
    }

    for (unsigned int childIndex = 0; childIndex < node->children.size();
         ++childIndex) {
        if (node->children[childIndex] &&
            node->children[childIndex]->initialized &&
            !node->children[childIndex]->solved) {
            double visitPart =
                magicConstant *
                std::sqrt(parentVisitPart /
                          (double)node->children[childIndex]->numberOfVisits);
            double UCTValue =
                node->children[childIndex]->getExpectedRewardEstimate() +
                visitPart;

            assert(!MathUtils::doubleIsMinusInfinity(UCTValue));

            if (MathUtils::doubleIsGreater(UCTValue, bestUCTValue)) {
                bestActionIndices.clear();
                bestActionIndices.push_back(childIndex);
                bestUCTValue = UCTValue;
            } else if (MathUtils::doubleIsEqual(UCTValue, bestUCTValue)) {
                bestActionIndices.push_back(childIndex);
            }
        }
    }
}

/******************************************************************
                            Parameter
******************************************************************/

bool ActionSelection::setValueFromString(std::string& param,
                                         std::string& value) {
    if (param == "-lvar") {
        setSelectLeastVisitedActionInRoot(atoi(value.c_str()));
        return true;
    } else if (param == "-mvd") {
        setMaxVisitDiff(atoi(value.c_str()));
        return true;
    }

    return false;
}

bool UCB1ActionSelection::setValueFromString(std::string& param,
                                             std::string& value) {
    if (param == "-mcs") {
        setMagicConstantScaleFactor(atof(value.c_str()));
        return true;
    } else if (param == "-er") {
        if (value == "LOG") {
            setExplorationRate(LOG);
            return true;
        } else if (value == "SQRT") {
            setExplorationRate(SQRT);
            return true;
        } else if (value == "LIN") {
            setExplorationRate(LIN);
            return true;
        } else if (value == "LNQUAD") {
            setExplorationRate(LNQUAD);
            return true;
        }
    }
    return ActionSelection::setValueFromString(param, value);
}

/******************************************************************
                              Print
******************************************************************/

void ActionSelection::printConfig(std::string indent) const {
    Logger::logLine(indent + "Action selection: " + name, Verbosity::VERBOSE);

    indent += "  ";
    if (selectLeastVisitedActionInRoot) {
        Logger::logLine(indent + "Uniform selection in root state: enabled",
                        Verbosity::VERBOSE);
    } else {
        Logger::logLine(indent + "Uniform selection in root state: disabled",
                        Verbosity::VERBOSE);
    }
    Logger::logLine(indent + "Maximal visit difference factor: " +
                    std::to_string(maxVisitDiff), Verbosity::VERBOSE);
}

void UCB1ActionSelection::printConfig(std::string indent) const {
    ActionSelection::printConfig(indent);

    indent += "  ";
    switch (explorationRate) {
        case LOG:
            Logger::logLine(indent + "Exploration rate: LOG",
                            Verbosity::VERBOSE);
            break;
        case SQRT:
            Logger::logLine(indent + "Exploration rate: SQRT",
                            Verbosity::VERBOSE);
            break;
        case LIN:
            Logger::logLine(indent + "Exploration rate: LIN",
                            Verbosity::VERBOSE);
            break;
        case LNQUAD:
            Logger::logLine(indent + "Exploration rate: LNQUAD",
                            Verbosity::VERBOSE);
            break;
    }

    Logger::logLine(
        indent + "Magic constant: " + std::to_string(magicConstantScaleFactor),
        Verbosity::VERBOSE);
}

void ActionSelection::printStepStatistics(std::string indent) const {
    Logger::logLine(indent + name + " step statistics:", Verbosity::VERBOSE);
    indent += "  ";
    double perc =
        static_cast<double>(numExplorationInRoot) /
        static_cast<double>(numExplorationInRoot + numExploitationInRoot);
    Logger::logLine(
        indent + "Percentage exploration: " + std::to_string(perc),
        Verbosity::VERBOSE);
    Logger::logLine("", Verbosity::VERBOSE);
}

void ActionSelection::printRoundStatistics(std::string indent) const {
    Logger::logLine(indent + name + " round statistics:", Verbosity::NORMAL);
    indent += "  ";
    Logger::logLine(
        indent + "Percentage exploration in initial state: " +
        std::to_string(percentageExplorationInInitialState),
        Verbosity::SILENT);
    Logger::logLine("", Verbosity::VERBOSE);
}

