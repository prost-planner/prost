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

    // This is called when caching is disabled because memory becomes sparse
    virtual void disableCaching() {}

    virtual void initSession() {}
    virtual void initRound() {}
    virtual void finishRound() {}
    virtual void initStep() {
        // Reset per step statistics
        skippedBackups = 0;
    }
    virtual void finishStep() {}
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
    virtual void printConfig(std::string indent) const;
    virtual void printStepStatistics(std::string indent) const;
    virtual void printRoundStatistics(std::string /*indent*/) const {}

protected:
    BackupFunction(THTS* _thts,
                   std::string _name,
                   bool _useSolveLabeling = false,
                   bool _useBackupLock = false)
        : thts(_thts),
          name(_name),
          useSolveLabeling(_useSolveLabeling),
          useBackupLock(_useBackupLock) {}

    THTS* thts;

    // Name, used for output only
    std::string name;

    // If this is true, no further nodes a rebacked up in this trial
    bool lockBackup;

    // Parameter
    bool useSolveLabeling;
    bool useBackupLock;

    // Per step statistics
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
        : BackupFunction(_thts, "MonteCarlo backup"),
          initialLearningRate(1.0),
          learningRateDecay(1.0) {}

    // Set parameters from command line
    bool setValueFromString(std::string& param, std::string& value) override;

    // Parameter setter
    void setInitialLearningRate(double _initialLearningRate) {
        initialLearningRate = _initialLearningRate;
    }

    void setLearningRateDecay(double _learningRateDecay) {
        learningRateDecay = _learningRateDecay;
    }

    // Backup functions
    void backupChanceNode(SearchNode* node, double const& futReward) override;

    // Prints statistics
    void printConfig(std::string indent) const override;

private:
    double initialLearningRate;
    double learningRateDecay;
};

/******************************************************************
                   MaxMonte-Carlo Backups
******************************************************************/

class MaxMCBackupFunction : public BackupFunction {
public:
    MaxMCBackupFunction(THTS* _thts) : BackupFunction(_thts, "MaxMonteCarlo backup") {}

    // Backup functions
    void backupChanceNode(SearchNode* node, double const& futReward) override;
};

/******************************************************************
                      Partial Bellman Backups
******************************************************************/

class PBBackupFunction : public BackupFunction {
public:
    PBBackupFunction(THTS* _thts) : BackupFunction(_thts, "PartialBellman backup", true, true) {}

    // Backup functions
    void backupChanceNode(SearchNode* node, double const& futReward) override;
};

#endif
