#ifndef BACKUP_FUNCTION_H
#define BACKUP_FUNCTION_H

#include <string>

template<class SearchNode> class THTS;

/******************************************************************
                         Backup Function
******************************************************************/

template <class SearchNode>
class BackupFunction {
public:
    // Set parameters from command line
    virtual bool setValueFromString(std::string& /*param*/, std::string& /*value*/) {
        return false;
    }

    virtual void initRound() {}
    virtual void initTrial() {}

    // Backup functions
    virtual void backupDecisionNodeLeaf(SearchNode* node, double const& futReward);
    virtual void backupDecisionNode(SearchNode* node);
    virtual void backupChanceNode(SearchNode* node, double const& futReward) = 0;

    // Prints statistics
    virtual void printStats(std::ostream& out, std::string indent);

protected:
    BackupFunction<SearchNode>(THTS<SearchNode>* _thts,
                               bool _useSolveLabeling = false,
                               bool _useBackupLock = false) :
        thts(_thts),
        useSolveLabeling(_useSolveLabeling),
        useBackupLock(_useBackupLock) {}

    THTS<SearchNode>* thts;
    bool useSolveLabeling;
    bool useBackupLock;

    // Statistics
    int skippedBackups;
};

/******************************************************************
                       Monte-Carlo Backups
******************************************************************/

template <class SearchNode>
class MCBackupFunction : public BackupFunction<SearchNode> {
public:
    MCBackupFunction<SearchNode>(THTS<SearchNode>* _thts) :
        BackupFunction<SearchNode>(_thts),
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
    virtual void backupChanceNode(SearchNode* node, double const& futReward);

private:
    double initialLearningRate;
    double learningRateDecay;
};

/******************************************************************
                   MaxMonte-Carlo Backups
******************************************************************/

template <class SearchNode>
class MaxMCBackupFunction : public BackupFunction<SearchNode> {
public:
    MaxMCBackupFunction<SearchNode>(THTS<SearchNode>* _thts) :
        BackupFunction<SearchNode>(_thts) {}

    // Backup functions
    virtual void backupChanceNode(SearchNode* node, double const& futReward);
};

/******************************************************************
                      Partial Bellman Backups
******************************************************************/

template <class SearchNode>
class PBBackupFunction : public BackupFunction<SearchNode> {
public:
    PBBackupFunction<SearchNode>(THTS<SearchNode>* _thts) :
    BackupFunction<SearchNode>(_thts, true, true) {}

    // Backup functions
    virtual void backupChanceNode(SearchNode* node, double const& futReward);
};

#endif
