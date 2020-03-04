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

    // This is called when caching is disabled because memory becomes sparse
    virtual void disableCaching() {}

    virtual void initSession() {}
    virtual void initRound() {
        // Reset per round statistics
        percentageExplorationInInitialState = 0.0;
    }
    virtual void finishRound() {}
    virtual void initStep();
    virtual void finishStep();
    virtual void initTrial() {}

    // Action selection
    int selectAction(SearchNode* node);
    virtual void _selectAction(SearchNode* node) = 0;

    void selectGreedyAction(SearchNode* node);
    void selectLeastVisitedAction(SearchNode* node);
    void selectRandomAction(SearchNode* node);
    void selectActionBasedOnVisitDifference(SearchNode* node);

    // Prints statistics
    virtual void printConfig(std::string indent) const;
    virtual void printStepStatistics(std::string indent) const;
    virtual void printRoundStatistics(std::string indent) const;

protected:
    ActionSelection(THTS* _thts, std::string _name)
        : thts(_thts),
          name(_name),
          selectLeastVisitedActionInRoot(false),
          maxVisitDiff(50),
          currentRootNode(nullptr),
          numExplorationInRoot(0),
          numExploitationInRoot(0),
          percentageExplorationInInitialState(0.0) {}

    THTS* thts;

    // Name, used for output only
    std::string name;

    // Vector for decision node children of equal quality
    std::vector<int> bestActionIndices;

    // Variable to enable uniform action selection at root node
    bool selectLeastVisitedActionInRoot;

    // The maximal number of times an action may be selected more often than the
    // one with the lowest number of visits
    int maxVisitDiff;

    SearchNode const* currentRootNode;

    // Per step statistics
    int numExplorationInRoot;
    int numExploitationInRoot;

    // Per round statistics
    double percentageExplorationInInitialState;
};

class BFSActionSelection : public ActionSelection {
public:
    BFSActionSelection(THTS* _thts)
        : ActionSelection(_thts, "BFS action selection") {}

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
        : ActionSelection(_thts, "UCB1 action selection"),
          explorationRate(LOG),
          magicConstantScaleFactor(1.0) {}

    // Set parameters from command line
    bool setValueFromString(std::string& param, std::string& value) override;

    // Parameter setter
    virtual void setMagicConstantScaleFactor(double _magicConstantScaleFactor) {
        magicConstantScaleFactor = _magicConstantScaleFactor;
    }

    virtual void setExplorationRate(ExplorationRate _explorationRate) {
        explorationRate = _explorationRate;
    }

    // Action selection
    void _selectAction(SearchNode* node) override;

    // Printer
    void printConfig(std::string indent) const override;

protected:
    // Parameter
    ExplorationRate explorationRate;
    double magicConstantScaleFactor;
};

#endif
