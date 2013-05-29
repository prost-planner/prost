#include "max_mc_uct_search.h"

using namespace std;

/******************************************************************
                        Initialization
******************************************************************/

void MaxMCUCTSearch::initializeDecisionNodeChild(MaxMCUCTNode* node, unsigned int const& actionIndex, double const& initialQValue) {
    node->children[actionIndex] = getSearchNode();
    node->children[actionIndex]->reward = heuristicWeight * (double)remainingConsideredSteps() * initialQValue;
    node->children[actionIndex]->numberOfVisits = numberOfInitialVisits;
    node->numberOfChildrenVisits += numberOfInitialVisits;

    // cout << "initializing child ";
    // task->printAction(cout,index);
    // cout << " with " << initialQValue << " leading to init of " << node->children[index]->accumulatedReward << endl;
}

/******************************************************************
                         Outcome selection
******************************************************************/

MaxMCUCTNode* MaxMCUCTSearch::selectOutcome(MaxMCUCTNode* node, State& stateAsProbDistr, int& varIndex) {
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

void MaxMCUCTSearch::backupDecisionNodeLeaf(MaxMCUCTNode* node, double const& immReward, double const& futReward) {
    node->reward = immReward + futReward;
    ++node->numberOfVisits;

    // cout << "updated dec node leaf with immediate reward " << immReward << endl;
    // node->print(cout);
    // cout << endl;
}

void MaxMCUCTSearch::backupDecisionNode(MaxMCUCTNode* node, double const& immReward, double const& /*futReward*/) {
    // TODO: Store the best child. Then:

    // 1. The best child has been sampled. If futReward is at least as
    // good as that child's reward before it is still the best child
    // (it got better). Only otherwise check for the best child again.

    // 2. Another child has been sampled. Check if it is better than
    // the former best child. If so, it is the new best child,
    // otherwise the former best child is still the best child.

    // Identify best child and take its reward estimate
    node->reward = node->children[0]->reward;
    node->numberOfChildrenVisits = node->children[0]->numberOfVisits;

    for(unsigned int childIndex = 1; childIndex < node->children.size(); ++childIndex) {
        if(node->children[childIndex]) {
            node->numberOfChildrenVisits += node->children[childIndex]->numberOfVisits;

            if(MathUtils::doubleIsGreater(node->children[childIndex]->reward, node->reward)) {
                node->reward = node->children[childIndex]->reward;
            }
        }
    }

    // Add immediate reward and visit
    node->reward += immReward;
    ++node->numberOfVisits;

    // cout << "updated dec node with immediate reward " << immReward << endl;
    // node->print(cout);
    // cout << endl;
}

void MaxMCUCTSearch::backupChanceNode(MaxMCUCTNode* node, double const& /*futReward*/) {
    assert(node->children.size() == 2);

    // Propagate values from children
    if(node->children[0] && node->children[1]) {
        node->numberOfVisits = node->children[0]->numberOfVisits + node->children[1]->numberOfVisits;
        node->reward = (((node->children[0]->numberOfVisits * node->children[0]->reward) + 
                         (node->children[1]->numberOfVisits * node->children[1]->reward)) /
                        node->numberOfVisits);
    } else if(node->children[0]) {
        node->numberOfVisits = node->children[0]->numberOfVisits;
        node->reward = node->children[0]->reward;
    } else {
        assert(node->children[1]);

        node->numberOfVisits = node->children[1]->numberOfVisits;        
        node->reward = node->children[1]->reward;
    }
}



