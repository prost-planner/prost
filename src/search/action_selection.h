#ifndef ACTION_SELECTION_H
#define ACTION_SELECTION_H

#include <string>
#include <vector>

template<class SearchNode> class THTS;

template <class SearchNode>
class ActionSelection {
public:
    // Set parameters from command line
    virtual bool setValueFromString(std::string& param, std::string& value);

    // Parameter setter
    virtual void setSelectLeastVisitedActionInRoot(bool _selectLeastVisitedActionInRoot) {
        selectLeastVisitedActionInRoot = _selectLeastVisitedActionInRoot;
    }

    virtual void setMaxVisitDiff(int _maxVisitDiff) {
        maxVisitDiff = _maxVisitDiff;
    }

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
    ActionSelection<SearchNode>(THTS<SearchNode>* _thts) :
        thts(_thts),
        selectLeastVisitedActionInRoot(false),
        maxVisitDiff(50),
        exploreInRoot(0),
        exploitInRoot(0) {}

    THTS<SearchNode>* thts;

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

template <class SearchNode>
class BFSActionSelection : public ActionSelection<SearchNode> {
public:
    BFSActionSelection(THTS<SearchNode>* _thts) :
        ActionSelection<SearchNode>(_thts) {}

    // Action selection
    void _selectAction(SearchNode* node) {
        return this->selectLeastVisitedAction(node);
    }
};

template <class SearchNode>
class UCB1ActionSelection : public ActionSelection<SearchNode> {
public:
    // Possible types for the exploration-rate function
    enum ExplorationRate {
        LOG,
        SQRT,
        LIN,
        LNQUAD
    };

    UCB1ActionSelection(THTS<SearchNode>* _thts) :
        ActionSelection<SearchNode>(_thts),
        explorationRate(LOG),
        magicConstantScaleFactor(1.0) {}

    // Set parameters from command line
    bool setValueFromString(std::string& param, std::string& value);

    // Parameter setter
    virtual void setMagicConstantScaleFactor( double _magicConstantScaleFactor) {
        magicConstantScaleFactor = _magicConstantScaleFactor;
    }
    
    virtual void setExplorationRate(ExplorationRate _explorationRate) {
        explorationRate = _explorationRate;
    }

    // Action selection
    void _selectAction(SearchNode* node);

protected:
    // Parameter
    ExplorationRate explorationRate;
    double magicConstantScaleFactor;
};

#endif
