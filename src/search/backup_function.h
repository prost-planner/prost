#ifndef BACKUP_FUNCTION_H
#define BACKUP_FUNCTION_H

#include <string>

class THTS;
class SearchNode;

/******************************************************************
                         Backup Function
******************************************************************/

class BackupFunction {
public:
    virtual ~BackupFunction() {}

    // Create a backup function component
    static BackupFunction* fromString(std::string& desc, THTS* thts);

    // Set parameters from command line
    virtual bool setValueFromString(std::string& /*param*/,
                                    std::string& /*value*/) {
        return false;
    }

    // Learns parameter values from a random training set
    virtual void learn() {}

    // This is called when caching is disabled because memory becomes sparse
    virtual void disableCaching() {}

    virtual void initRound() {}
    virtual void initTrial() {
        lockBackup = false;
    }

    // Backup functions
    virtual void backupDecisionNodeLeaf(SearchNode* node,
                                        double const& futReward);
    virtual void backupDecisionNode(SearchNode* node);
    virtual void backupChanceNode(SearchNode* node,
                                  double const& futReward) = 0;

    // Prints statistics
    virtual void printStats(std::ostream& out, std::string indent);

protected:
    BackupFunction(THTS* _thts, bool _useSolveLabeling = false,
                   bool _useBackupLock = false)
        : thts(_thts),
          useSolveLabeling(_useSolveLabeling),
          useBackupLock(_useBackupLock) {}

    THTS* thts;

    // If this is true, no further nodes a rebacked up in this trial
    bool lockBackup;

    // Parameter
    bool useSolveLabeling;
    bool useBackupLock;

    // Statistics
    int skippedBackups;

    // Tests which access private members
    friend class BFSTestSearch;
};

/******************************************************************
                       Monte-Carlo Backups
******************************************************************/

class MCBackupFunction : public BackupFunction {
public:
    MCBackupFunction(THTS* _thts)
        : BackupFunction(_thts),
          initialLearningRate(1.0),
          learningRateDecay(1.0) {}

    // Set parameters from command line
    virtual bool setValueFromString(std::string& param, std::string& value);

    // Parameter setter
    virtual void setInitialLearningRate(double _initialLearningRate) {
        initialLearningRate = _initialLearningRate;
    }

    virtual void setLearningRateDecay(double _learningRateDecay) {
        learningRateDecay = _learningRateDecay;
    }

    // Backup functions
    void backupChanceNode(SearchNode* node, double const& futReward) override;

private:
    double initialLearningRate;
    double learningRateDecay;
};

/******************************************************************
                   MaxMonte-Carlo Backups
******************************************************************/

class MaxMCBackupFunction : public BackupFunction {
public:
    MaxMCBackupFunction(THTS* _thts) : BackupFunction(_thts) {}

    // Backup functions
    void backupChanceNode(SearchNode* node, double const& futReward) override;
};

/******************************************************************
                      Partial Bellman Backups
******************************************************************/

class PBBackupFunction : public BackupFunction {
public:
    PBBackupFunction(THTS* _thts) : BackupFunction(_thts, true, true) {}

    // Backup functions
    void backupChanceNode(SearchNode* node, double const& futReward) override;
};

#endif
