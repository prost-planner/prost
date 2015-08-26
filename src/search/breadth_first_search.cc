#include "breadth_first_search.h"

/******************************************************************
                         Action Selection
******************************************************************/

int BreadthFirstSearch::selectAction(THTSSearchNode* node) {
    int minVisits = std::numeric_limits<int>::max();
    for(unsigned int childIndex = 0; childIndex < node->children.size(); ++childIndex) {
        int const& numVisits = node->children[childIndex]->getNumberOfVisits();
        if(node->children[childIndex] && 
           !node->children[childIndex]->isSolved()) {
            if(numVisits < minVisits) {
                bestActionIndices.clear();
                bestActionIndices.push_back(childIndex);
                minVisits = numVisits;
            } else if(numVisits == minVisits) {
                bestActionIndices.push_back(childIndex);
            }
        }
    }

    assert(!bestActionIndices.empty());
    return bestActionIndices[std::rand() % bestActionIndices.size()];
}

/******************************************************************
                         Outcome selection
******************************************************************/

THTSSearchNode* BreadthFirstSearch::selectOutcome(THTSSearchNode* node,
                                                  PDState& nextState,
                                                  int const& varIndex,
                                                  int const& lastProbVarIndex) {
    DiscretePD& pd = nextState.probabilisticStateFluentAsPD(varIndex);
    assert(pd.isWellDefined());

    double probSum = 1.0;
    int childIndex = 0;

    if (node->children.empty()) {
        node->children.resize(
            SearchEngine::probabilisticCPFs[varIndex]->getDomainSize(), nullptr);
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
    assert(MathUtils::doubleIsGreater(probSum, 0.0) &&
           MathUtils::doubleIsSmallerOrEqual(probSum, 1.0));

    double randNum = MathUtils::generateRandomNumber() * probSum;

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

    assert((childIndex >= 0) && childIndex < node->children.size());

    if (!node->children[childIndex]) {
        if (varIndex == lastProbVarIndex) {
            node->children[childIndex] = getDecisionNode(childProb);
        } else {
            node->children[childIndex] = getChanceNode(childProb);
        }
    }

    assert(!node->children[childIndex]->isSolved());

    nextState.probabilisticStateFluent(varIndex) = childIndex;
    return node->children[childIndex];
}

/******************************************************************
                          Backup Functions
******************************************************************/


void BreadthFirstSearch::backupDecisionNodeLeaf(THTSSearchNode* node,
                                                double const& futReward) {
    node->children.clear();

    node->futureReward = futReward;
    node->solved = true;

    ++node->numberOfVisits;
}

void BreadthFirstSearch::backupDecisionNode(THTSSearchNode* node,
                                            double const& /*futReward*/) {
    assert(!node->children.empty());

    ++node->numberOfVisits;

    if (backupLock) {
        skippedBackups++;
        return;
    }

    double oldFutureReward = node->futureReward;

    // Propagate values from best child
    node->futureReward = -std::numeric_limits<double>::max();
    node->solved = true;
    for (unsigned int childIndex = 0; childIndex < node->children.size();
         ++childIndex) {
        if (node->children[childIndex]) {
            node->solved &= node->children[childIndex]->solved;
            node->futureReward = std::max(node->futureReward,
                    node->children[childIndex]->getExpectedRewardEstimate());
        }
    }

    // If the future reward did not change we did not find a better node and
    // therefore do not need to update the rewards in preceding parents.
    if (!node->solved &&
        (node->remainingSteps > maxLockDepth) &&
        MathUtils::doubleIsEqual(oldFutureReward, node->futureReward)) {
        backupLock = true;
    }
}

void BreadthFirstSearch::backupChanceNode(THTSSearchNode* node,
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
            node->futureReward += (node->children[i]->prob *
                                   node->children[i]->getExpectedRewardEstimate());
            probSum += node->children[i]->prob;

            if (node->children[i]->solved) {
                solvedSum += node->children[i]->prob;
            }
        }
    }

    node->futureReward /= probSum;
    node->solved = MathUtils::doubleIsEqual(solvedSum, 1.0);
}

