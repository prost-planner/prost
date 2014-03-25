#include "max_mc_uct_search.h"

using namespace std;

/******************************************************************
                        Initialization
******************************************************************/

void MaxMCUCTSearch::initializeDecisionNodeChild(MaxMCUCTNode* node, unsigned int const& actionIndex, double const& initialQValue) {
    node->children[actionIndex] = getSearchNode();
    node->children[actionIndex]->futureReward = heuristicWeight * (double)remainingConsideredSteps() * initialQValue;
    node->children[actionIndex]->numberOfVisits = numberOfInitialVisits;

    node->numberOfVisits += numberOfInitialVisits;
    node->futureReward = std::max(node->futureReward, node->children[actionIndex]->getExpectedRewardEstimate());

    // cout << "initializing child ";
    // task->printAction(cout,index);
    // cout << " with " << initialQValue << " leading to init of " << node->children[index]->accumulatedReward << endl;
}

/******************************************************************
                         Outcome selection
******************************************************************/

MaxMCUCTNode* MaxMCUCTSearch::selectOutcome(MaxMCUCTNode* node, PDState& nextPDState, State& nextState, int& varIndex) {
    // TODO: No node should be created if nextPDState[varIndex] is deterministic
    if(node->children.empty()) {
        node->children.resize(task->getDomainSizeOfCPF(varIndex), NULL);
    }

    int childIndex = (int)task->sampleVariable(nextPDState[varIndex]);
    nextState[varIndex] = childIndex;

    if(!node->children[childIndex]) {
        node->children[childIndex] = getSearchNode();
    }
    return node->children[childIndex];
}

/******************************************************************
                          Backup functions
******************************************************************/

void MaxMCUCTSearch::backupDecisionNodeLeaf(MaxMCUCTNode* node, double const& immReward, double const& futReward) {
    node->immediateReward = immReward;
    node->futureReward = futReward;

    ++node->numberOfVisits;

    // cout << "updated dec node leaf with immediate reward " << immReward << endl;
    // node->print(cout);
    // cout << endl;
}

void MaxMCUCTSearch::backupDecisionNode(MaxMCUCTNode* node, double const& immReward, double const& /*futReward*/) {
    assert(!node->children.empty());

    node->immediateReward = immReward;

    if(selectedActionIndex() != -1) {
        ++node->numberOfVisits;
    }

    // set best child dependent values to noop child first
    node->futureReward = node->children[0]->getExpectedRewardEstimate();

    for(unsigned int childIndex = 1; childIndex < node->children.size(); ++childIndex) {
        if(node->children[childIndex]) {
            node->futureReward = std::max(node->futureReward, node->children[childIndex]->getExpectedRewardEstimate());
        }
    }

    // cout << "updated dec node with immediate reward " << immReward << endl;
    // node->print(cout);
    // cout << endl;
}

void MaxMCUCTSearch::backupChanceNode(MaxMCUCTNode* node, double const& /*futReward*/) {
    assert(MathUtils::doubleIsEqual(node->immediateReward, 0.0));

    ++node->numberOfVisits;
    node->futureReward = 0.0;
    int numberOfChildVisits = 0;

    // Propagate values from children
    for(unsigned int i = 0; i < node->children.size(); ++i) {
        if(node->children[i]) {
            node->futureReward += (node->children[i]->numberOfVisits * node->children[i]->getExpectedRewardEstimate());
            numberOfChildVisits += node->children[i]->numberOfVisits;
        }
    }

    node->futureReward /= numberOfChildVisits;
}



