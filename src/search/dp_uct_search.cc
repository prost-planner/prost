#include "dp_uct_search.h"

using namespace std;

/******************************************************************
                     Initialization of Nodes
******************************************************************/

void DPUCTSearch::initializeDecisionNodeChild(DPUCTNode* node,
        unsigned int const& actionIndex,
        double const& initialQValue) {
    node->children[actionIndex] = getDPUCTNode(1.0);
    node->children[actionIndex]->futureReward = heuristicWeight *
                                                (double)
                                                remainingConsideredSteps() *
                                                initialQValue;
    node->children[actionIndex]->numberOfVisits = numberOfInitialVisits;

    node->numberOfVisits += numberOfInitialVisits;
    node->futureReward =
        std::max(node->futureReward,
                node->children[actionIndex]->getExpectedRewardEstimate());

    // cout << "initialized child ";
    // SearchEngine::actionStates[actionIndex].printCompact(cout);
    // cout << " with remaining steps " << remainingConsideredSteps() << " and initialQValue " << initialQValue << endl;
    // node->children[actionIndex]->print(cout);
    // cout << endl;
}

/******************************************************************
                         Outcome selection
******************************************************************/

DPUCTNode* DPUCTSearch::selectOutcome(DPUCTNode* node, PDState& nextState,
        int& varIndex) {
    // TODO: Prevent the case where nextPDState[varIndex] is deterministic
    DiscretePD& pd = nextState.probabilisticStateFluentAsPD(varIndex);
    assert(pd.isWellDefined());

    double probSum = 1.0;
    int childIndex = 0;

    if (node->children.empty()) {
        node->children.resize(
                SearchEngine::probabilisticCPFs[varIndex]->getDomainSize(),
                NULL);
    } else {
        // Determine the sum of the probabilities of unsolved outcomes
        for (unsigned int i = 0; i < pd.size(); ++i) {
            childIndex = pd.values[i];
            if (node->children[childIndex] &&
                node->children[childIndex]->isSolved()) {
                probSum -= pd.probabilities[i];
            }
        }
    }
    assert(MathUtils::doubleIsGreater(probSum,
                    0.0) && MathUtils::doubleIsSmallerOrEqual(probSum, 1.0));

    double randNum = MathUtils::generateRandomNumber() * probSum;
    //cout << "ProbSum is " << probSum << endl;
    //cout << "RandNum is " << randNum << endl;

    probSum = 0.0;
    double childProb = 0.0;

    for (unsigned int i = 0; i < pd.size(); ++i) {
        childIndex = pd.values[i];
        if (!node->children[childIndex] ||
            !node->children[childIndex]->isSolved()) {
            probSum += pd.probabilities[i];
            if (MathUtils::doubleIsSmaller(randNum, probSum)) {
                childProb = pd.probabilities[i];
                break;
            }
        }
    }

    // cout << "Chosen child is " << childIndex << " and prob is " << childProb << endl;

    assert((childIndex >= 0) && childIndex < node->children.size());

    if (!node->children[childIndex]) {
        node->children[childIndex] = getDPUCTNode(childProb);
    }

    assert(!node->children[childIndex]->isSolved());

    nextState.probabilisticStateFluent(varIndex) = childIndex;
    return node->children[childIndex];
}

/******************************************************************
                          Backup Functions
******************************************************************/

void DPUCTSearch::backupDecisionNodeLeaf(DPUCTNode* node,
        double const& immReward,
        double const& futReward) {
    node->children.clear();

    node->immediateReward = immReward;
    node->futureReward = futReward;
    node->solved = true;

    ++node->numberOfVisits;
    // cout << "updated dec node leaf with immediate reward " << immReward << endl;
    // node->print(cout);
    // cout << endl;
}

void DPUCTSearch::backupDecisionNode(DPUCTNode* node, double const& immReward,
        double const& /*futReward*/) {
    assert(!node->children.empty());

    node->immediateReward = immReward;

    if (selectedActionIndex() != -1) {
        ++node->numberOfVisits;
    }

    if (backupLock) {
        ++skippedBackups;
        return;
    }

    double oldFutureReward = node->futureReward;

    // Propagate values from best child
    node->futureReward = -numeric_limits<double>::max();
    node->solved = true;
    for (unsigned int childIndex = 0; childIndex < node->children.size();
         ++childIndex) {
        if (node->children[childIndex]) {
            node->solved &= node->children[childIndex]->solved;
            node->futureReward =
                std::max(node->futureReward,
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

    //cout << "updated dec node with immediate reward " << immReward << endl;
    //node->print(cout);
    //cout << endl;
}

void DPUCTSearch::backupChanceNode(DPUCTNode* node,
        double const& /*futReward*/) {
    assert(MathUtils::doubleIsEqual(node->immediateReward, 0.0));

    ++node->numberOfVisits;
    if (backupLock) {
        ++skippedBackups;
        return;
    }

    // Propagate values from children
    node->futureReward = 0.0;
    double solvedSum = 0.0;
    double probSum = 0.0;

    for (unsigned int i = 0; i < node->children.size(); ++i) {
        if (node->children[i]) {
            node->futureReward +=
                (node->children[i]->prob *
                 node->children[i]->getExpectedRewardEstimate());
            probSum += node->children[i]->prob;

            if (node->children[i]->solved) {
                solvedSum += node->children[i]->prob;
            }
        }
    }

    node->futureReward /= probSum;
    node->solved = MathUtils::doubleIsEqual(solvedSum, 1.0);

    // cout << "updated chance node:" << endl;
    // node->print(cout);
    // cout << endl;
}
