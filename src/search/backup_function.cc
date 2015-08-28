#include "backup_function.h"

#include "thts.h"

#include "utils/math_utils.h"

#include <iostream>

/******************************************************************
                         Backup Function
******************************************************************/

template <class SearchNode>
void BackupFunction<SearchNode>::backupDecisionNodeLeaf(SearchNode* node,
                                                        double const& futReward) {
    ++node->numberOfVisits;
    node->futureReward = futReward;
    node->solved = useSolveLabeling;

    // std::cout << "updated dec node leaf:" << std::endl;
    // node->print(std::cout);
    // std::cout << std::endl;
}

template <class SearchNode>
void BackupFunction<SearchNode>::backupDecisionNode(SearchNode* node) {
    assert(!node->children.empty());

    ++node->numberOfVisits;

    if (thts->backupLock) {
        ++skippedBackups;
        return;
    }

    double oldFutureReward = node->futureReward;

    // Propagate values from best child
    node->futureReward = -std::numeric_limits<double>::max();
    node->solved = useSolveLabeling;
    for (SearchNode* child : node->children) {
        if (child) {
            node->solved &= child->solved;
            node->futureReward =
                std::max(node->futureReward, child->getExpectedRewardEstimate());
        }
    }

    // If the future reward did not change we did not find a better node and
    // therefore do not need to update the rewards in preceding parents.
    if (!node->solved &&
        (node->remainingSteps > thts->maxLockDepth) &&
        MathUtils::doubleIsEqual(oldFutureReward, node->futureReward)) {
        thts->backupLock = useBackupLock;
    }

    // std::cout << "updated dec node with " << node->remainingSteps 
    //           << " steps-to-go and immediate reward " << node->immediateReward << std::endl;
    // node->print(std::cout);
    // std::cout << std::endl;
}

template <class SearchNode>
void BackupFunction<SearchNode>::printStats(std::ostream& out, std::string indent) {
    if (useBackupLock) {
        out << indent << "Skipped backups: " << this->skippedBackups << std::endl;
    }
}

/******************************************************************
                       Monte-Carlo Backups
******************************************************************/

template <class SearchNode>
bool MCBackupFunction<SearchNode>::setValueFromString(std::string& param, std::string& value) {
    if (param == "-ilr") {
        setInitialLearningRate(atof(value.c_str()));
        return true;
    } else if (param == "-lrd") {
        setLearningRateDecay(atof(value.c_str()));
        return true;
    }
    return false;
}

template <class SearchNode>
void MCBackupFunction<SearchNode>::backupChanceNode(SearchNode* node,
                                                     double const& futReward) {
    ++node->numberOfVisits;

    node->futureReward = node->futureReward +
        initialLearningRate * (futReward - node->futureReward) / 
        (1.0 + (learningRateDecay * (double)node->numberOfVisits));

    // std::cout << "updated chance node:" << std::endl;
    // node->print(std::cout);
    // std::cout << std::endl;
}

/******************************************************************
                   MaxMonte-Carlo Backups
******************************************************************/

template <class SearchNode>
void MaxMCBackupFunction<SearchNode>::backupChanceNode(SearchNode* node,
                                                       double const& /*futReward*/) {
    assert(MathUtils::doubleIsEqual(node->immediateReward, 0.0));

    ++node->numberOfVisits;
    node->futureReward = 0.0;
    int numberOfChildVisits = 0;

    // Propagate values from children
    for (SearchNode* child : node->children) {
        if (child) {
            node->futureReward +=
                (child->numberOfVisits * child->getExpectedRewardEstimate());
            numberOfChildVisits += child->numberOfVisits;
        }
    }

    node->futureReward /= numberOfChildVisits;

    // std::cout << "updated chance node:" << std::endl;
    // node->print(std::cout);
    // std::cout << std::endl;
}

/******************************************************************
                      Partial Bellman Backups
******************************************************************/

template <class SearchNode>
void PBBackupFunction<SearchNode>::backupChanceNode(SearchNode* node,
                                                    double const& /*futReward*/) {
    assert(MathUtils::doubleIsEqual(node->immediateReward, 0.0));

    ++node->numberOfVisits;
    if (this->thts->backupLock) {
        ++this->skippedBackups;
        return;
    }

    // Propagate values from children
    node->futureReward = 0.0;
    double solvedSum = 0.0;
    double probSum = 0.0;

    for (SearchNode* child : node->children) {
        if (child) {
            node->futureReward += (child->prob * child->getExpectedRewardEstimate());
            probSum += child->prob;

            if (child->solved) {
                solvedSum += child->prob;
            }
        }
    }

    node->futureReward /= probSum;
    node->solved = MathUtils::doubleIsEqual(solvedSum, 1.0);

    // std::cout << "updated chance node:" << std::endl;
    // node->print(std::cout);
    // std::cout << std::endl;
}


// force compilation of required template classes
template class MCBackupFunction<THTSSearchNode>;
template class MaxMCBackupFunction<THTSSearchNode>;
template class PBBackupFunction<THTSSearchNode>;


