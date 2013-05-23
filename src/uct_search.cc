#include "uct_search.h"

#include "prost_planner.h"
#include "logical_expressions.h"
#include "actions.h"
#include "conditional_probability_function.h"
#include "iterative_deepening_search.h"

#include "utils/timer.h"
#include "utils/math_utils.h"
#include "utils/system_utils.h"

#include <limits>
#include <iostream>

using namespace std;

/******************************************************************
                      Search Engine Creation 
******************************************************************/

bool UCTSearch::setValueFromString(string& param, string& value) {
    if(param == "-mcs") {
        setMagicConstantScaleFactor(atof(value.c_str()));
        return true;
    } else if(param == "-iv") {
        setNumberOfInitialVisits(atoi(value.c_str()));
        return true;
    }

    return THTS<UCTNode>::setValueFromString(param, value);
}

void UCTSearch::setInitializer(SearchEngine* _initializer) {
    THTS<UCTNode>::setInitializer(_initializer);
    
    if(dynamic_cast<RandomSearch*>(initializer)) {
        numberOfInitialVisits = 0;
    }
}

/******************************************************************
                        Initialization
******************************************************************/

inline void UCTSearch::initializeDecisionNodeChild(UCTNode* node, unsigned int const& actionIndex, double const& initialQValue) {
    node->children[actionIndex] = getSearchNode();
    node->children[actionIndex]->accumulatedReward = (double)numberOfInitialVisits * (double)currentStateIndex * initialQValue;
    node->children[actionIndex]->numberOfVisits = numberOfInitialVisits;
    node->numberOfChildrenVisits += numberOfInitialVisits;

    // cout << "initializing child ";
    // task->printAction(cout,index);
    // cout << " with " << initialQValue << " leading to init of " << node->children[index]->accumulatedReward << endl;
}


/******************************************************************
                         Action Selection
******************************************************************/

void UCTSearch::selectAction(UCTNode* node) {
    bestDecisionNodeChildren.clear();

    chooseUnvisitedChild(node);

    if(bestDecisionNodeChildren.empty()) {
        chooseDecisionNodeSuccessorBasedOnVisitDifference(node);
    }

    if(bestDecisionNodeChildren.empty()) {
        chooseDecisionNodeSuccessorBasedOnUCTFormula(node);
    }

    assert(!bestDecisionNodeChildren.empty());

    actions[currentActionIndex] = bestDecisionNodeChildren[rand() % bestDecisionNodeChildren.size()];
    chosenChild = node->children[actions[currentActionIndex]];

    // cout << "Chosen action is ";
    // task->printAction(cout, actions[currentActionIndex]);
    // cout << endl;
}

inline void UCTSearch::chooseUnvisitedChild(UCTNode* node) {
    for(unsigned int i = 0; i < node->children.size(); ++i) {
        if(node->children[i] && node->children[i]->numberOfVisits == 0) {
            bestDecisionNodeChildren.push_back(i);
        }
    }
}

inline void UCTSearch::chooseDecisionNodeSuccessorBasedOnVisitDifference(UCTNode* node) {
    unsigned int childIndex = 0;
    for(; childIndex < node->children.size(); ++childIndex) {
    	if(node->children[childIndex]) {
            bestDecisionNodeChildren.push_back(childIndex);
            smallestNumVisits = node->children[childIndex]->numberOfVisits;
            highestNumVisits = node->children[childIndex]->numberOfVisits;
            break;
        }
    }

    ++childIndex;

    for(; childIndex < node->children.size(); ++childIndex) {
        if(node->children[childIndex]) {
            if(MathUtils::doubleIsSmaller(node->children[childIndex]->numberOfVisits,smallestNumVisits)) {
                bestDecisionNodeChildren.clear();
                bestDecisionNodeChildren.push_back(childIndex);
                smallestNumVisits = node->children[childIndex]->numberOfVisits;
            } else if(MathUtils::doubleIsEqual(node->children[childIndex]->numberOfVisits,smallestNumVisits)) {
                bestDecisionNodeChildren.push_back(childIndex);
            } else if(MathUtils::doubleIsGreater(node->children[childIndex]->numberOfVisits, highestNumVisits))  {
                highestNumVisits = node->children[childIndex]->numberOfVisits;
            }
        }
    }

    if(50*smallestNumVisits >= highestNumVisits) {
        bestDecisionNodeChildren.clear();
    }
}

inline void UCTSearch::chooseDecisionNodeSuccessorBasedOnUCTFormula(UCTNode* node) {
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
    numberOfChildrenVisitsLog = std::log((double)node->numberOfChildrenVisits);

    for(unsigned int i = 0; i < node->children.size(); ++i) {
        if(node->children[i] != NULL) {
            visitPart = magicConstant * sqrt(numberOfChildrenVisitsLog / (double)node->children[i]->numberOfVisits);
            UCTValue = node->children[i]->getExpectedRewardEstimate() + visitPart;

            assert(!MathUtils::doubleIsMinusInfinity(UCTValue));

            if(MathUtils::doubleIsGreater(UCTValue,bestUCTValue)) {
                bestDecisionNodeChildren.clear();
                bestDecisionNodeChildren.push_back(i);
                bestUCTValue = UCTValue;
            } else if(MathUtils::doubleIsEqual(UCTValue,bestUCTValue)) {
                bestDecisionNodeChildren.push_back(i);
            }
        }
    }
}

/******************************************************************
                         Outcome selection
******************************************************************/

void UCTSearch::selectOutcome(UCTNode* node) {
    //select corresponding UCTNode
    if(node->children.empty()) {
        node->children.resize(2,NULL);
    }

    task->sampleVariable(states[nextStateIndex], varIndex);
    unsigned int childIndex = (unsigned int)states[nextStateIndex][varIndex];

    if(!node->children[childIndex]) {
        node->children[childIndex] = getSearchNode();
    }
    chosenChild = node->children[childIndex];
}

/******************************************************************
                          Backup functions
******************************************************************/

void UCTSearch::backupDecisionNode(UCTNode* node, double const& immReward, double const& futReward) {
    node->accumulatedReward += (immReward + futReward);
    ++node->numberOfVisits;
    ++node->numberOfChildrenVisits;
}

void UCTSearch::backupChanceNode(UCTNode* node, double const& futReward) {
    node->accumulatedReward += futReward;
    ++node->numberOfVisits;
}



