#include "backup_function.h"

#include "thts.h"

#include "utils/logger.h"
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

    // Logger::logLine("updated dec node leaf:", Verbosity::DEBUG);
    // Logger::logLine(node->toString(), Verbosity::DEBUG);
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

    // Logger::logLine("updated dec node with " +
    //                 std::to_string(node->stepsToGo) +
    //                 " steps-to-go and immediate reward " +
    //                 std::to_string(node->immediateReward), Verbosity::DEBUG);
    // Logger::logLine(node->toString(), Verbosity::DEBUG);
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

    // Logger::logLine("updated chance node:", Verbosity::DEBUG);
    // Logger::logLine(node->toString(), Verbosity::DEBUG);
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

    // Logger::logLine("updated chance node:", Verbosity::DEBUG);
    // Logger::logLine(node->toString(), Verbosity::DEBUG);
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

    // Logger::logLine("updated chance node:", Verbosity::DEBUG);
    // Logger::logLine(node->toString(), Verbosity::DEBUG);
}

void BackupFunction::printConfig(std::string indent) const {
    Logger::logLine(indent + "Backup function: " + name, Verbosity::VERBOSE);

    indent += "  ";
    if (useSolveLabeling) {
        Logger::logLine(indent + "Solve labeling: enabled",
                        Verbosity::VERBOSE);
    } else {
        Logger::logLine(indent + "Solve labeling: disabled",
                        Verbosity::VERBOSE);
    }
    if (useBackupLock) {
        Logger::logLine(indent + "Backup lock: enabled",
                        Verbosity::VERBOSE);
    } else {
        Logger::logLine(indent + "Backup lock: disabled",
                        Verbosity::VERBOSE);
    }
}

void MCBackupFunction::printConfig(std::string indent) const {
    BackupFunction::printConfig(indent);

    indent += "  ";
    Logger::logLine(indent + "initial learning rate: " +
                    std::to_string(initialLearningRate),
                    Verbosity::VERBOSE);
    Logger::logLine(indent + "learning rate decay: " +
                    std::to_string(learningRateDecay),
                    Verbosity::VERBOSE);
}

void BackupFunction::printStepStatistics(std::string indent) const {
    if (useBackupLock) {
        Logger::logLine(
            indent + name + " step statistics:", Verbosity::VERBOSE);
        indent += "  ";
        Logger::logLine(
            indent + "Skipped backups: " + std::to_string(skippedBackups),
            Verbosity::VERBOSE);
        Logger::logLine("", Verbosity::VERBOSE);
    }
}
