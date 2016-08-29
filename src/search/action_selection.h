#ifndef ACTION_SELECTION_H
#define ACTION_SELECTION_H

#include <string>
#include <vector>

class THTS;
class SearchNode;

class ActionSelection {
public:
    virtual ~ActionSelection() {}

    // Create an action selection component
    static ActionSelection* fromString(std::string& desc, THTS* thts);

    // Set parameters from command line
    virtual bool setValueFromString(std::string& param, std::string& value);

    // Parameter setter
    virtual void setSelectLeastVisitedActionInRoot(
        bool _selectLeastVisitedActionInRoot) {
        selectLeastVisitedActionInRoot = _selectLeastVisitedActionInRoot;
    }

    virtual void setMaxVisitDiff(int _maxVisitDiff) {
        maxVisitDiff = _maxVisitDiff;
    }

    // Learns parameter values from a random training set
    virtual void learn() {}

    // This is called when caching is disabled because memory becomes sparse
    virtual void disableCaching() {}

    virtual void initRound() {
        exploreInRoot = 0;
        exploitInRoot = 0;
    }
    virtual void initTrial() {}

    // Action selection
    int selectAction(SearchNode* node);
    virtual void _selectAction(SearchNode* node) = 0;

    void selectGreedyAction(SearchNode* node);
    void selectLeastVisitedAction(SearchNode* node);
    void selectRandomAction(SearchNode* node);
    void selectActionBasedOnVisitDifference(SearchNode* node);

    // Prints statistics
    virtual void printStats(std::ostream& /*out*/, std::string /*indent*/);

protected:
    ActionSelection(THTS* _thts)
        : thts(_thts),
          selectLeastVisitedActionInRoot(false),
          maxVisitDiff(50),
          exploreInRoot(0),
          exploitInRoot(0) {}

    THTS* thts;

    // Vector for decision node children of equal quality
    std::vector<int> bestActionIndices;

    // Variable to enable uniform action selection at root node
    bool selectLeastVisitedActionInRoot;

    // The maximal number of times an action may be selected more often than the
    // one with the lowest number of visits
    int maxVisitDiff;

    int exploreInRoot;
    int exploitInRoot;
};

class BFSActionSelection : public ActionSelection {
public:
    BFSActionSelection(THTS* _thts) : ActionSelection(_thts) {}

    // Action selection
    void _selectAction(SearchNode* node) override {
        return selectLeastVisitedAction(node);
    }
};

class UCB1ActionSelection : public ActionSelection {
public:
    // Possible types for the exploration-rate function
    enum ExplorationRate { LOG, SQRT, LIN, LNQUAD };

    UCB1ActionSelection(THTS* _thts)
        : ActionSelection(_thts),
          explorationRate(LOG),
          magicConstantScaleFactor(1.0) {}

    // Set parameters from command line
    bool setValueFromString(std::string& param, std::string& value);

    // Parameter setter
    virtual void setMagicConstantScaleFactor(double _magicConstantScaleFactor) {
        magicConstantScaleFactor = _magicConstantScaleFactor;
    }

    virtual void setExplorationRate(ExplorationRate _explorationRate) {
        explorationRate = _explorationRate;
    }

    // Action selection
    void _selectAction(SearchNode* node) override;

protected:
    // Parameter
    ExplorationRate explorationRate;
    double magicConstantScaleFactor;
};

#endif
