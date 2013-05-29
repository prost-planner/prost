#include "mc_uct_search.h"

using namespace std;

/******************************************************************
                        Initialization
******************************************************************/

void MCUCTSearch::initializeDecisionNodeChild(MCUCTNode* node, unsigned int const& actionIndex, double const& initialQValue) {
    node->children[actionIndex] = getSearchNode();
    node->children[actionIndex]->accumulatedReward = (double)numberOfInitialVisits * (double)remainingConsideredSteps() * initialQValue;
    node->children[actionIndex]->numberOfVisits = numberOfInitialVisits;
    node->numberOfChildrenVisits += numberOfInitialVisits;

    // cout << "initializing child ";
    // task->printAction(cout,index);
    // cout << " with " << initialQValue << " leading to init of " << node->children[index]->accumulatedReward << endl;
}

/******************************************************************
                         Outcome selection
******************************************************************/

MCUCTNode* MCUCTSearch::selectOutcome(MCUCTNode* node, State& stateAsProbDistr, int& varIndex) {
    if(node->children.empty()) {
        node->children.resize(2,NULL);
    }

    task->sampleVariable(stateAsProbDistr, varIndex);
    unsigned int childIndex = (unsigned int)stateAsProbDistr[varIndex];

    if(!node->children[childIndex]) {
        node->children[childIndex] = getSearchNode();
    }
    return node->children[childIndex];
}

/******************************************************************
                          Backup functions
******************************************************************/

void MCUCTSearch::backupDecisionNode(MCUCTNode* node, double const& immReward, double const& futReward) {
    node->accumulatedReward += (immReward + futReward);
    ++node->numberOfVisits;
    ++node->numberOfChildrenVisits;
}

void MCUCTSearch::backupChanceNode(MCUCTNode* node, double const& futReward) {
    node->accumulatedReward += futReward;
    ++node->numberOfVisits;
}



