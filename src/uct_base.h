#ifndef UCT_BASE_H
#define UCT_BASE_H

// UCTBase is the super class of all UCT variants, i.e. it implements
// the UCB1 based action selection. To derive a non-abstract UCT
// algorithm, implement selectOutcome, initializeDecisionNodeChild and
// the backup functions (and continueTrial if desired).

// SearchNode must be a class that satisfies all constraints (1-5) of
// a THTS Search node and additionally contains:

// 6. A member variable numberOfVisits that returns the number of
// times this node has been visited in a trial.

// 7. A member variable numberOfChildrenVisits that returns the number
// of times this node has been visited in a trial.

#include "thts.h"
#include "prost_planner.h"

template <class SearchNode>
class UCTBase : public THTS<SearchNode> {
public:
    // Search engine creation
    bool setValueFromString(std::string& param, std::string& value);

    // Parameter setters: new parameters
    virtual void setNumberOfInitialVisits(int _numberOfInitialVisits) {
        numberOfInitialVisits = _numberOfInitialVisits;
    }

    virtual void setInitializer(SearchEngine* _initializer) {
        if(dynamic_cast<RandomSearch*>(_initializer)) {
            numberOfInitialVisits = 0;
        }

        THTS<SearchNode>::setInitializer(_initializer);
    }

    virtual void setMagicConstantScaleFactor(double _magicConstantScaleFactor) {
        magicConstantScaleFactor = _magicConstantScaleFactor;
    }

protected:
    UCTBase(std::string _name, ProstPlanner* _planner) :
        THTS<SearchNode>(_name, _planner, _planner->getProbabilisticTask()), 
        numberOfInitialVisits(5),
        magicConstantScaleFactor(1.0) {}

    // Action selection
    int selectAction(SearchNode* node);
    void selectUnselectedAction(SearchNode* node);
    void selectActionBasedOnVisitDifference(SearchNode* node);
    void selectActionBasedOnUCTFormula(SearchNode* node);

    // Vector for decision node children of equal quality (wrt UCT
    // formula)
    std::vector<int> bestActionIndices;

    // Variables to calculate UCT formula
    double magicConstant;
    double parentVisitPart;
    double visitPart;
    double UCTValue;
    double bestUCTValue;

    // Variables to choose successor that has been tried too rarely
    // from UCT
    int smallestNumVisits;
    int highestNumVisits;

    // Parameter
    int numberOfInitialVisits;
    double magicConstantScaleFactor;
};

/******************************************************************
                      Search engine creation 
******************************************************************/

template <class SearchNode>
bool UCTBase<SearchNode>::setValueFromString(std::string& param, std::string& value) {
    if(param == "-mcs") {
        setMagicConstantScaleFactor(atof(value.c_str()));
        return true;
    } else if(param == "-iv") {
        setNumberOfInitialVisits(atoi(value.c_str()));
        return true;
    }

    return THTS<SearchNode>::setValueFromString(param, value);
}

/******************************************************************
                         Action Selection
******************************************************************/

template <class SearchNode>
int UCTBase<SearchNode>::selectAction(SearchNode* node) {
    bestActionIndices.clear();

    selectUnselectedAction(node);

    if(bestActionIndices.empty()) {
        selectActionBasedOnVisitDifference(node);
    }

    if(bestActionIndices.empty()) {
        selectActionBasedOnUCTFormula(node);
    }

    assert(!bestActionIndices.empty());
    return bestActionIndices[rand() % bestActionIndices.size()];
}

template <class SearchNode>
inline void UCTBase<SearchNode>::selectUnselectedAction(SearchNode* node) {
    for(unsigned int i = 0; i < node->children.size(); ++i) {
        if(node->children[i] && (node->children[i]->numberOfVisits == 0)) {
            bestActionIndices.push_back(i);
        }
    }
}

template <class SearchNode>
inline void UCTBase<SearchNode>::selectActionBasedOnVisitDifference(SearchNode* node) {
    unsigned int childIndex = 0;
    for(; childIndex < node->children.size(); ++childIndex) {
    	if(node->children[childIndex] && !node->children[childIndex]->isSolved()) {
            bestActionIndices.push_back(childIndex);
            smallestNumVisits = node->children[childIndex]->numberOfVisits;
            highestNumVisits = node->children[childIndex]->numberOfVisits;
            break;
        }
    }

    ++childIndex;

    for(; childIndex < node->children.size(); ++childIndex) {
        if(node->children[childIndex] && !node->children[childIndex]->isSolved()) {
            if(MathUtils::doubleIsSmaller(node->children[childIndex]->numberOfVisits,smallestNumVisits)) {
                bestActionIndices.clear();
                bestActionIndices.push_back(childIndex);
                smallestNumVisits = node->children[childIndex]->numberOfVisits;
            } else if(MathUtils::doubleIsEqual(node->children[childIndex]->numberOfVisits,smallestNumVisits)) {
                bestActionIndices.push_back(childIndex);
            } else if(MathUtils::doubleIsGreater(node->children[childIndex]->numberOfVisits, highestNumVisits))  {
                highestNumVisits = node->children[childIndex]->numberOfVisits;
            }
        }
    }

    if(50*smallestNumVisits >= highestNumVisits) {
        bestActionIndices.clear();
    }
}

template <class SearchNode>
inline void UCTBase<SearchNode>::selectActionBasedOnUCTFormula(SearchNode* node) {
    if(node->numberOfVisits == 0) {
        magicConstant = 0.0;
    } else {
        magicConstant = magicConstantScaleFactor * std::abs(node->getExpectedRewardEstimate());
        if(MathUtils::doubleIsEqual(magicConstant,0.0)) {
            magicConstant = 100.0;
        }
    }

    assert(node->numberOfChildrenVisits != 0);

    bestUCTValue = -std::numeric_limits<double>::max();
    parentVisitPart = std::log((double)node->numberOfChildrenVisits);

    for(unsigned int childIndex = 0; childIndex < node->children.size(); ++childIndex) {
        if(node->children[childIndex] && !node->children[childIndex]->isSolved()) {
            visitPart = magicConstant * sqrt(parentVisitPart / (double)node->children[childIndex]->numberOfVisits);
            UCTValue = node->children[childIndex]->getExpectedRewardEstimate() + visitPart;

            assert(!MathUtils::doubleIsMinusInfinity(UCTValue));

            if(MathUtils::doubleIsGreater(UCTValue,bestUCTValue)) {
                bestActionIndices.clear();
                bestActionIndices.push_back(childIndex);
                bestUCTValue = UCTValue;
            } else if(MathUtils::doubleIsEqual(UCTValue,bestUCTValue)) {
                bestActionIndices.push_back(childIndex);
            }
        }
    }
}


#endif 
