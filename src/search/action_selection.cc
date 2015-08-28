#include "action_selection.h"

#include <cassert>

#include "thts.h"

#include "utils/math_utils.h"
#include "utils/system_utils.h"

/******************************************************************
                          ActionSelection
******************************************************************/

template <class SearchNode>
int ActionSelection<SearchNode>::selectAction(SearchNode* node) {
    bestActionIndices.clear();

    if (node->getNumberOfVisits() <= 1) {
        selectGreedyAction(node);
    }

    if (selectLeastVisitedActionInRoot && (node == thts->getCurrentRootNode()) && bestActionIndices.empty()) {
       selectLeastVisitedAction(node);
    }

    if (bestActionIndices.empty()) {
        selectActionBasedOnVisitDifference(node);
    }

    if (bestActionIndices.empty()) {
        _selectAction(node);
    }

    if ((thts->getCurrentRootNode()->remainingSteps == SearchEngine::horizon) && (node == thts->getCurrentRootNode())) {
        int selectedIndex = bestActionIndices[std::rand() % bestActionIndices.size()];
        double bestValue = node->children[selectedIndex]->getExpectedRewardEstimate();

        for (unsigned int childIndex = 0;  childIndex < node->children.size(); ++childIndex) {
            if (node->children[childIndex] && MathUtils::doubleIsGreater(node->children[childIndex]->getExpectedRewardEstimate(), bestValue)) {
                ++exploreInRoot;
                return selectedIndex;
            }
        }
        ++exploitInRoot;
        return selectedIndex;
    }

    // std::cout << "selecting action from among: ";
    // for(unsigned int i = 0; i < bestActionIndices.size(); ++i) {
    //     std::cout << bestActionIndices[i] << " ";
    // }
    // std::cout << std::endl;

    // int rnd = std::rand();
    // std::cout << "rand num is " << rnd << std::endl;

    assert(!bestActionIndices.empty());
    return bestActionIndices[std::rand() % bestActionIndices.size()];
}

template <class SearchNode>
inline void ActionSelection<SearchNode>::selectGreedyAction(SearchNode* node) {
    double bestValue = -std::numeric_limits<double>::max();

    for (unsigned int childIndex = 0;  childIndex < node->children.size(); ++childIndex) {
        if (node->children[childIndex]) {
            if (MathUtils::doubleIsGreater(node->children[childIndex]->getExpectedRewardEstimate(), bestValue)) {
                bestActionIndices.clear();
                bestActionIndices.push_back(childIndex);
                bestValue = node->children[childIndex]->getExpectedRewardEstimate();
            } else if (MathUtils::doubleIsEqual(node->children[childIndex]->getExpectedRewardEstimate(), bestValue)) {
                bestActionIndices.push_back(childIndex);
            }
        }
    }
}

template <class SearchNode>
inline void ActionSelection<SearchNode>::selectLeastVisitedAction(SearchNode* node) {
    int leastVisits = std::numeric_limits<int>::max();

    for (unsigned int childIndex = 0;  childIndex < node->children.size(); ++childIndex) {
        if(node->children[childIndex] && !node->children[childIndex]->isSolved()) {
            if (node->children[childIndex]->getNumberOfVisits() < leastVisits) {
                leastVisits = node->children[childIndex]->getNumberOfVisits();
                bestActionIndices.clear();
                bestActionIndices.push_back(childIndex);
            } else if(node->children[childIndex]->getNumberOfVisits() == leastVisits) {
                bestActionIndices.push_back(childIndex);
            }
        }
    }
}

template <class SearchNode>
inline void ActionSelection<SearchNode>::selectRandomAction(SearchNode* node) {
    for (unsigned int childIndex = 0;  childIndex < node->children.size(); ++childIndex) {
        if (node->children[childIndex] && !node->children[childIndex]->isSolved()) {
            bestActionIndices.push_back(childIndex);
        }
    }
}

template <class SearchNode>
inline void ActionSelection<SearchNode>::selectActionBasedOnVisitDifference(SearchNode* node) {
    int smallestNumVisits = std::numeric_limits<int>::max();
    int highestNumVisits = 0;

    for (unsigned int childIndex = 0; childIndex < node->children.size(); ++childIndex) {
        if (node->children[childIndex] && !node->children[childIndex]->isSolved()) {
            if (node->children[childIndex]->getNumberOfVisits() < smallestNumVisits) {
                bestActionIndices.clear();
                bestActionIndices.push_back(childIndex);
                smallestNumVisits =
                    node->children[childIndex]->getNumberOfVisits();
            } else if (node->children[childIndex]->getNumberOfVisits() == smallestNumVisits) {
                bestActionIndices.push_back(childIndex);
            }

            if (node->children[childIndex]->getNumberOfVisits() > highestNumVisits) {
                highestNumVisits =
                    node->children[childIndex]->getNumberOfVisits();
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

template <class SearchNode>
void UCB1ActionSelection<SearchNode>::_selectAction(SearchNode* node) {
    double magicConstant;

    if (MathUtils::doubleIsMinusInfinity(node->getExpectedFutureRewardEstimate()) || 
        MathUtils::doubleIsEqual(node->getExpectedFutureRewardEstimate(), 0.0)) {
        magicConstant = 100.0;
    } else {
        magicConstant = magicConstantScaleFactor *
            std::abs(node->getExpectedFutureRewardEstimate());
    }
    assert(node->getNumberOfVisits() > 0);

    double bestUCTValue = -std::numeric_limits<double>::max();
    double parentVisitPart = 1.0; //2.0

    switch (explorationRate) {
    case SQRT:
        parentVisitPart *= std::sqrt((double) node->getNumberOfVisits());
        break;
    case LIN:
        parentVisitPart *= node->getNumberOfVisits();
        break;
    case LNQUAD: {
        double logPart = std::log((double) node->getNumberOfVisits());
        parentVisitPart *= logPart * logPart;
        break;
    }
    case LOG:
        parentVisitPart *= std::log((double) node->getNumberOfVisits());
    }

    for (unsigned int childIndex = 0; childIndex < node->children.size(); ++childIndex) {
        if (node->children[childIndex] &&
            !node->children[childIndex]->isSolved()) {
            double visitPart = magicConstant * 
                std::sqrt(parentVisitPart / (double)node->children[childIndex]->numberOfVisits);
            double UCTValue =
                node->children[childIndex]->getExpectedRewardEstimate() + visitPart;

            assert(!MathUtils::doubleIsMinusInfinity(UCTValue));

            if (MathUtils::doubleIsGreater(UCTValue, bestUCTValue)) {
                this->bestActionIndices.clear();
                this->bestActionIndices.push_back(childIndex);
                bestUCTValue = UCTValue;
            } else if (MathUtils::doubleIsEqual(UCTValue, bestUCTValue)) {
                this->bestActionIndices.push_back(childIndex);
            }
        }
    }
}

/******************************************************************
                            Parameter
******************************************************************/

template <class SearchNode>
bool ActionSelection<SearchNode>::setValueFromString(std::string& param,
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

template <class SearchNode>
bool UCB1ActionSelection<SearchNode>::setValueFromString(std::string& param,
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
    return ActionSelection<SearchNode>::setValueFromString(param, value);
}

/******************************************************************
                              Print
******************************************************************/

template <class SearchNode>
void ActionSelection<SearchNode>::printStats(std::ostream& out, std::string indent) {
    if (thts->getCurrentRootNode()->remainingSteps == SearchEngine::horizon) {
        out << indent << "Action Selection:" << std::endl;
        out << indent << "Exploitation in Root: " << exploitInRoot << std::endl;
        out << indent << "Exploration in Root: " << exploreInRoot << std::endl;
        out << indent << "Percentage Exploration in Root: " << ((double) ((double) exploreInRoot) / ((double)exploreInRoot + (double)exploitInRoot)) << std::endl;
    }
}


// force compilation of required template classes
template class ActionSelection<THTSSearchNode>;
template class BFSActionSelection<THTSSearchNode>;
template class UCB1ActionSelection<THTSSearchNode>;

