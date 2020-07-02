#include "action_selection.h"

#include <cassert>

#include "thts.h"

#include "utils/logger.h"
#include "utils/math_utils.h"
#include "utils/string_utils.h"
#include "utils/system_utils.h"

using namespace std;

/******************************************************************
                     Action Selection Creation
******************************************************************/

ActionSelection* ActionSelection::fromString(string& desc, THTS* thts) {
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
        string param;
        string value;
        StringUtils::nextParamValuePair(desc, param, value);

        if (!result->setValueFromString(param, value)) {
            SystemUtils::abort(
                "Unused parameter value pair: " + param + " / " + value);
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
    int denominator = numExplorationInRoot + numExploitationInRoot;
    if ((percentageExplorationInFirstRelevantState < 0.0) &&
        (denominator > 0)) {
        percentageExplorationInFirstRelevantState =
            static_cast<double>(numExplorationInRoot) /
            static_cast<double>(denominator);
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
    double bestValue = -numeric_limits<double>::max();
    int numChildren = node->children.size();

    for (int index = 0; index < numChildren; ++index) {
        SearchNode* child = node->children[index];
        if (child && child->initialized) {
            double reward = child->getExpectedRewardEstimate();
            if (MathUtils::doubleIsGreater(reward, bestValue)) {
                bestActionIndices.clear();
                bestActionIndices.push_back(index);
                bestValue = reward;
            } else if (MathUtils::doubleIsEqual(reward, bestValue)) {
                bestActionIndices.push_back(index);
            }
        }
    }
}

void ActionSelection::selectLeastVisitedAction(SearchNode* node) {
    int leastVisits = numeric_limits<int>::max();
    int numChildren = node->children.size();

    for (int index = 0; index < numChildren; ++index) {
        SearchNode* child = node->children[index];
        if (child && child->initialized && !child->solved) {
            int numVisits = child->numberOfVisits;
            if (numVisits < leastVisits) {
                bestActionIndices.clear();
                bestActionIndices.push_back(index);
                leastVisits = numVisits;
            } else if (numVisits == leastVisits) {
                bestActionIndices.push_back(index);
            }
        }
    }
}

inline void ActionSelection::selectRandomAction(SearchNode* node) {
    int numChildren = node->children.size();

    for (int index = 0; index < numChildren; ++index) {
        SearchNode* child = node->children[index];
        if (child && child->initialized && !child->solved) {
            bestActionIndices.push_back(index);
        }
    }
}

inline void ActionSelection::selectActionBasedOnVisitDifference(
    SearchNode* node) {
    int leastVisits = numeric_limits<int>::max();
    int mostVisits = 0;
    int numChildren = node->children.size();

    for (int index = 0; index < numChildren; ++index) {
        SearchNode* child = node->children[index];
        if (child && child->initialized && !child->solved) {
            int numVisits = child->numberOfVisits;
            if (numVisits < leastVisits) {
                bestActionIndices.clear();
                bestActionIndices.push_back(index);
                leastVisits = numVisits;
            } else if (numVisits == leastVisits) {
                bestActionIndices.push_back(index);
            }

            if (numVisits > mostVisits) {
                mostVisits = numVisits;
            }
        }
    }

    if ((maxVisitDiff * leastVisits) >= mostVisits) {
        bestActionIndices.clear();
    }
}

/******************************************************************
                              UCB1
******************************************************************/

void UCB1ActionSelection::_selectAction(SearchNode* node) {
    double magicConstant = 100.0;
    double reward = node->getExpectedFutureRewardEstimate();
    double numVisits = static_cast<double>(node->numberOfVisits);
    int numChildren = node->children.size();

    if (!MathUtils::doubleIsMinusInfinity(reward) &&
        !MathUtils::doubleIsEqual(reward, 0.0)) {
        magicConstant = magicConstantScaleFactor * abs(reward);
    }
    assert(node->numberOfVisits > 0);

    double bestUCTValue = -numeric_limits<double>::max();
    double parentVisitPart = 1.0; // 2.0

    switch (explorationRate) {
    case SQRT:
        parentVisitPart *= sqrt(numVisits);
        break;
    case LIN:
        parentVisitPart *= numVisits;
        break;
    case LNQUAD: {
        double logPart = log(numVisits);
        parentVisitPart *= logPart * logPart;
        break;
    }
    case LOG:
        parentVisitPart *= log(numVisits);
    }

    for (int index = 0; index < numChildren; ++index) {
        SearchNode* child = node->children[index];
        if (child && child->initialized && !child->solved) {
            double childNumVisits = static_cast<double>(child->numberOfVisits);
            double visitPart =
                magicConstant * sqrt(parentVisitPart / childNumVisits);
            double UCTValue = child->getExpectedRewardEstimate() + visitPart;

            assert(!MathUtils::doubleIsMinusInfinity(UCTValue));

            if (MathUtils::doubleIsGreater(UCTValue, bestUCTValue)) {
                bestActionIndices.clear();
                bestActionIndices.push_back(index);
                bestUCTValue = UCTValue;
            } else if (MathUtils::doubleIsEqual(UCTValue, bestUCTValue)) {
                bestActionIndices.push_back(index);
            }
        }
    }
}

/******************************************************************
                            Parameter
******************************************************************/

bool ActionSelection::setValueFromString(string& param,
                                         string& value) {
    if (param == "-lvar") {
        setSelectLeastVisitedActionInRoot(atoi(value.c_str()));
        return true;
    } else if (param == "-mvd") {
        setMaxVisitDiff(atoi(value.c_str()));
        return true;
    }

    return false;
}

bool UCB1ActionSelection::setValueFromString(string& param,
                                             string& value) {
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

void ActionSelection::printConfig(string indent) const {
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
                    to_string(maxVisitDiff), Verbosity::VERBOSE);
}

void UCB1ActionSelection::printConfig(string indent) const {
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
        indent + "Magic constant: " + to_string(magicConstantScaleFactor),
        Verbosity::VERBOSE);
}

void ActionSelection::printStepStatistics(string indent) const {
    Logger::logLine(indent + name + " step statistics:", Verbosity::VERBOSE);
    indent += "  ";

    double perc = 0.0;
    int denominator = numExplorationInRoot + numExploitationInRoot;
    if (denominator > 0) {
        perc =
            static_cast<double>(numExplorationInRoot) /
            static_cast<double>(denominator);
    }
    Logger::logLine(
        indent + "Percentage exploration: " + to_string(perc),
        Verbosity::VERBOSE);
    Logger::logLine("", Verbosity::VERBOSE);
}

void ActionSelection::printRoundStatistics(string indent) const {
    Logger::logLine(indent + name + " round statistics:", Verbosity::NORMAL);
    indent += "  ";
    double perc = max(percentageExplorationInFirstRelevantState, 0.0);
    Logger::logLine(
        indent + "Percentage exploration in first relevant state: " + to_string(perc),
        Verbosity::SILENT);
    Logger::logLine("", Verbosity::VERBOSE);
}

