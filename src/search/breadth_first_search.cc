#include "breadth_first_search.h"

/******************************************************************
                     Initialization of Nodes
******************************************************************/

void BreadthFirstSearch::initializeDecisionNodeChild(BfsNode* node, unsigned int
        const& actionIndex, double const& initialQValue) {
    node->children[actionIndex] = getBfsNode(1.0);
    node->children[actionIndex]->futureReward = 
        (double) remainingConsideredSteps() * initialQValue;
    node->futureReward = std::max(node->futureReward, 
            node->children[actionIndex]->getExpectedRewardEstimate());

}

/******************************************************************
                         Outcome selection
******************************************************************/

BfsNode* BreadthFirstSearch::selectOutcome(
        BfsNode* node, PDState& nextState, int& varIndex) {
    // TODO: Prevent the case where nextPDState[varIndex] is deterministic
    DiscretePD& pd = nextState.probabilisticStateFluentAsPD(varIndex);
    assert(pd.isWellDefined());

    double probSum = 1.0;
    int childIndex = 0;

    if(node->children.empty()) {
        node->children.resize(
                SearchEngine::probabilisticCPFs[varIndex]->getDomainSize(),
                NULL);
    } else {
        // Determine the sum of the probabilities of unsolved outcomes
        for(unsigned int i = 0; i < pd.size(); ++i) {
            childIndex = pd.values[i];
            if(node->children[childIndex] && 
                    node->children[childIndex]->isSolved()) {
                probSum -= pd.probabilities[i];
            }
        }
    }
    assert(MathUtils::doubleIsGreater(probSum, 0.0)
            && MathUtils::doubleIsSmallerOrEqual(probSum, 1.0));

    double randNum = MathUtils::generateRandomNumber() * probSum;

    probSum = 0.0;
    double childProb = 0.0;

    for(unsigned int i = 0; i < pd.size(); ++i) {
        childIndex = pd.values[i];
        if(!node->children[childIndex] ||
                !node->children[childIndex]->isSolved()) {
            probSum += pd.probabilities[i];
            if(MathUtils::doubleIsSmaller(randNum, probSum)) {
                childProb = pd.probabilities[i];
                break;
            }
        }
    }

    assert((childIndex >= 0) && childIndex < node->children.size());

    if(!node->children[childIndex]) {
        node->children[childIndex] = getBfsNode(childProb);
    }

    assert(!node->children[childIndex]->isSolved());

    nextState.probabilisticStateFluent(varIndex) = childIndex;
    return node->children[childIndex];
}

/******************************************************************
                          Backup Functions
******************************************************************/


void BreadthFirstSearch::backupDecisionNodeLeaf(BfsNode* node,
       double const& immReward, double const& futReward) {
    node->children.clear();

    node->immediateReward = immReward;
    node->futureReward = futReward;
    node->solved = true;
    ++node->numberOfVisits;
}

void BreadthFirstSearch::backupDecisionNode(BfsNode* node, 
        double const& immReward, double const& /*futReward*/) {
    assert(!node->children.empty());

    node->immediateReward = immReward;

    if(selectedActionIndex() != -1) {
        ++node->numberOfVisits;
    }
    if(backupLock) {
        skippedBackups++;
        return;
    }

    double oldFutureReward = node->futureReward;

    // Propagate values from best child
    node->futureReward = -std::numeric_limits<double>::max();
    node->solved = true;
    for(unsigned int childIndex = 0; childIndex < node->children.size();
            ++childIndex) {
        if(node->children[childIndex]) {
            node->solved &= node->children[childIndex]->solved;
            node->futureReward = std::max(node->futureReward,
                    node->children[childIndex]->getExpectedRewardEstimate());
        }
    }

    // If the future reward did not change we did not find a better node and
    // therefore do not need to update the rewards in preceding parents.
    if (!node->solved &&
            remainingConsideredSteps() > maxLockDepth && 
            MathUtils::doubleIsEqual(oldFutureReward, node->futureReward)) { 
        backupLock = true;
    }
}

void BreadthFirstSearch::backupChanceNode(BfsNode* node,
        double const& /*futReward*/) {
    assert(MathUtils::doubleIsEqual(node->immediateReward, 0.0));

     ++node->numberOfVisits;
    if (backupLock) { 
        skippedBackups++;
        return;
    }

    // Propagate values from children
    node->futureReward = 0.0;
    double solvedSum = 0.0;
    double probSum = 0.0;

    for(unsigned int i = 0; i < node->children.size(); ++i) {
        if(node->children[i]) {
            node->futureReward += (node->children[i]->prob *
                    node->children[i]->getExpectedRewardEstimate());
            probSum += node->children[i]->prob;

            if(node->children[i]->solved) {
                solvedSum += node->children[i]->prob;
            }
        }
    }

    node->futureReward /= probSum;
    node->solved = MathUtils::doubleIsEqual(solvedSum, 1.0);

}

// Action selection
int BreadthFirstSearch::selectAction(BfsNode* node) {
    unsigned int i = 0;
    unsigned int j = 0;
    while(j < (node->children.size() - 1)) {
        ++j;
        if(node->children[i] && node->children[j]) {
            if(!node->children[j]->isSolved() &&
                    node->children[i]->getNumberOfVisits()
                    > node->children[j]->getNumberOfVisits()) {
                assert(!node->children[j]->isSolved());
                return j;
            }
            i = j;
        }
    } 

    // all actions were equally visited, return the first action.
    for(unsigned int k = 0; k < node->children.size(); ++k) {
        if (node->children[k] && !node->children[k]->isSolved()) { 
            assert(!node->children[k]->isSolved());
            return k;
        }
    }
    assert(false);
    return -1;
}
