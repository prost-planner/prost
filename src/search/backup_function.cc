#include "backup_function.h"

#include "thts.h"

#include "utils/math_utils.h"
#include "utils/string_utils.h"
#include "utils/system_utils.h"

#include <iostream>

/******************************************************************
                     Backup Function Creation
******************************************************************/

BackupFunction* BackupFunction::fromString(std::string& desc, THTS* thts) {
    StringUtils::trim(desc);
    assert(desc[0] == '[' && desc[desc.size() - 1] == ']');
    StringUtils::removeFirstAndLastCharacter(desc);
    StringUtils::trim(desc);

    BackupFunction* result = nullptr;

    if (desc.find("MC") == 0) {
        desc = desc.substr(2, desc.size());

        result = new MCBackupFunction(thts);
    } else if (desc.find("MaxMC") == 0) {
        desc = desc.substr(5, desc.size());

        result = new MaxMCBackupFunction(thts);
    } else if (desc.find("PB") == 0) {
        desc = desc.substr(2, desc.size());

        result = new PBBackupFunction(thts);
    } else {
        SystemUtils::abort("Unknown Backup Function: " + desc);
    }

    assert(result);
    StringUtils::trim(desc);

    while (!desc.empty()) {
        std::string param;
        std::string value;
        StringUtils::nextParamValuePair(desc, param, value);

        if (!result->setValueFromString(param, value)) {
            SystemUtils::abort("Unused parameter value pair: " + param + " / " +
                               value);
        }
    }

    return result;
}

/******************************************************************
                         Backup Function
******************************************************************/

void BackupFunction::backupDecisionNodeLeaf(SearchNode* node,
                                            double const& futReward) {
    ++node->numberOfVisits;
    node->futureReward = futReward;
    node->solved = useSolveLabeling;

    // std::cout << "updated dec node leaf:" << std::endl;
    // node->print(std::cout);
    // std::cout << std::endl;
}

void BackupFunction::backupDecisionNode(SearchNode* node) {
    assert(!node->children.empty());
    assert(thts->getTipNodeOfTrial());

    ++node->numberOfVisits;

    if (lockBackup) {
        ++skippedBackups;
        return;
    }

    double oldFutureReward = node->futureReward;

    // Propagate values from best child
    node->futureReward = -std::numeric_limits<double>::max();
    node->solved = useSolveLabeling;
    for (SearchNode* child : node->children) {
        if (child) {
            if (child->initialized) {
                node->solved &= child->solved;
                node->futureReward = std::max(
                    node->futureReward, child->getExpectedRewardEstimate());
            } else {
                node->solved = false;
            }
        }
    }

    // If the future reward did not change we did not find a better node and
    // therefore do not need to update the rewards in preceding parents.
    if (!node->solved &&
        (node->stepsToGo > thts->getTipNodeOfTrial()->stepsToGo) &&
        MathUtils::doubleIsEqual(oldFutureReward, node->futureReward)) {
        lockBackup = useBackupLock;
    }

    // std::cout << "updated dec node with " << node->stepsToGo
    //           << " steps-to-go and immediate reward " <<
    //           node->immediateReward << std::endl;
    // node->print(std::cout);
    // std::cout << std::endl;
}

void BackupFunction::printStats(std::ostream& out, std::string indent) {
    if (useBackupLock) {
        out << indent << "Skipped backups: " << skippedBackups << std::endl;
    }
}

/******************************************************************
                       Monte-Carlo Backups
******************************************************************/

bool MCBackupFunction::setValueFromString(std::string& param,
                                          std::string& value) {
    if (param == "-ilr") {
        setInitialLearningRate(atof(value.c_str()));
        return true;
    } else if (param == "-lrd") {
        setLearningRateDecay(atof(value.c_str()));
        return true;
    }
    return false;
}

void MCBackupFunction::backupChanceNode(SearchNode* node,
                                        double const& futReward) {
    ++node->numberOfVisits;

    node->futureReward =
        node->futureReward +
        initialLearningRate * (futReward - node->futureReward) /
            (1.0 + (learningRateDecay * (double)node->numberOfVisits));

    // std::cout << "updated chance node:" << std::endl;
    // node->print(std::cout);
    // std::cout << std::endl;
}

/******************************************************************
                   MaxMonte-Carlo Backups
******************************************************************/

void MaxMCBackupFunction::backupChanceNode(SearchNode* node,
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

void PBBackupFunction::backupChanceNode(SearchNode* node,
                                        double const& /*futReward*/) {
    assert(MathUtils::doubleIsEqual(node->immediateReward, 0.0));

    ++node->numberOfVisits;
    if (lockBackup) {
        ++skippedBackups;
        return;
    }

    // Propagate values from children
    node->futureReward = 0.0;
    double solvedSum = 0.0;
    double probSum = 0.0;

    for (SearchNode* child : node->children) {
        if (child) {
            node->futureReward +=
                (child->prob * child->getExpectedRewardEstimate());
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
