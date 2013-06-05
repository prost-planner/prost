#include "dp_uct_search.h"

using namespace std;

/******************************************************************
                     Initialization of Nodes
******************************************************************/

void DPUCTSearch::initializeDecisionNodeChild(DPUCTNode* node, unsigned int const& actionIndex, double const& initialQValue) {
    node->children[actionIndex] = getDPUCTNode(1.0);
    node->children[actionIndex]->futureReward = heuristicWeight * (double)remainingConsideredSteps() * initialQValue;
    node->children[actionIndex]->numberOfVisits = numberOfInitialVisits;

    node->numberOfVisits += numberOfInitialVisits;
    node->futureReward = std::max(node->futureReward, node->children[actionIndex]->getExpectedRewardEstimate());

    // cout << "initialized child ";
    // task->printAction(cout, actionIndex);
    // cout << endl;
    // initialQValue.print(cout);
    // node->children[actionIndex]->print(cout);
    // cout << endl;
}

/******************************************************************
                         Outcome selection
******************************************************************/

DPUCTNode* DPUCTSearch::selectOutcome(DPUCTNode* node, State& stateAsProbDistr, int& varIndex) {
    if(node->children.empty()) {
        node->children.resize(2,NULL);
    }

    double prob = stateAsProbDistr[varIndex];
    task->sampleVariable(stateAsProbDistr, varIndex);
    unsigned int childIndex = (unsigned int)stateAsProbDistr[varIndex];

    if(!node->children[childIndex]) {
        createChildNode(node, childIndex, prob);
    } else if(node->children[childIndex]->isSolved()) {
        childIndex = 1 - childIndex;
        stateAsProbDistr[varIndex] = childIndex;
        if(!node->children[childIndex]) {
            createChildNode(node, childIndex, prob);
        }
    }

    assert(!node->children[childIndex]->isSolved());
    return node->children[childIndex];
}

inline void DPUCTSearch::createChildNode(DPUCTNode* node, unsigned int const& childIndex, double const& prob) {
    assert(!node->children[childIndex]);

    if(childIndex == 0) {
        node->children[childIndex] = getDPUCTNode(1.0-prob);
    } else {
        node->children[childIndex] = getDPUCTNode(prob);
    }
}

/******************************************************************
                          Backup Functions
******************************************************************/

void DPUCTSearch::backupDecisionNodeLeaf(DPUCTNode* node, double const& immReward, double const& futReward) {
    node->children.clear();

    node->immediateReward = immReward;
    node->futureReward = futReward;
    node->solved = true;

    ++node->numberOfVisits;
    // cout << "updated dec node leaf with immediate reward " << immReward << endl;
    // node->print(cout);
    // cout << endl;
}

void DPUCTSearch::backupDecisionNode(DPUCTNode* node, double const& immReward, double const& /*futReward*/) {
    assert(!node->children.empty());

    node->immediateReward = immReward;

    if(selectedActionIndex() != -1) {
        ++node->numberOfVisits;
    }

    // set best child dependent values to noop child first
    node->futureReward = node->children[0]->getExpectedRewardEstimate();
    node->solved = node->children[0]->solved;

    // then check for better child
    for(unsigned int childIndex = 1; childIndex < node->children.size(); ++childIndex) {
        if(node->children[childIndex]) {
            node->solved &= node->children[childIndex]->solved;
            node->futureReward = std::max(node->futureReward, node->children[childIndex]->getExpectedRewardEstimate());
        }
    }

    // cout << "updated dec node with immediate reward " << immReward << endl;
    // node->print(cout);
    // cout << endl;    
}

void DPUCTSearch::backupChanceNode(DPUCTNode* node, double const& /*futReward*/) {
    assert(node->children.size() == 2);
    assert(MathUtils::doubleIsEqual(node->immediateReward, 0.0));

    ++node->numberOfVisits;

    // propagate values from children
    if(node->children[0] && node->children[1]) {
        node->futureReward = ((node->children[0]->prob * node->children[0]->getExpectedRewardEstimate()) + 
                              (node->children[1]->prob * node->children[1]->getExpectedRewardEstimate()));
        node->solved = node->children[0]->solved && node->children[1]->solved;
    } else if(node->children[0]) {
        node->futureReward = node->children[0]->getExpectedRewardEstimate();
        node->solved = node->children[0]->solved && MathUtils::doubleIsEqual(node->children[0]->prob, 1.0);
    } else {
        assert(node->children[1]);
        
        node->futureReward = node->children[1]->getExpectedRewardEstimate();
        node->solved = node->children[1]->solved && MathUtils::doubleIsEqual(node->children[1]->prob, 1.0);
    }

    // cout << "updated chance node:" << endl;
    // node->print(cout);
    // cout << endl;
}



