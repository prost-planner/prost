#ifndef BREADTH_FIRST_SEARCH_H
#define BREADTH_FIRST_SEARCH_H

// Implements breadth first search within the THTS framework.

#include "thts.h"

class BreadthFirstSearch : public THTS<THTSSearchNode> {
public:
    BreadthFirstSearch() :
        THTS<THTSSearchNode>("Breadth-First-Search") {
        setHeuristicWeight(1.0);
    }

protected:
    // Outcome selection
    THTSSearchNode* selectOutcome(THTSSearchNode*, PDState& nextState,
                                  int const& varIndex, int const& lastProbVarIndex);

    // Backup function
    void backupDecisionNodeLeaf(THTSSearchNode* node, double const& futureReward);
    void backupDecisionNode(THTSSearchNode*, double const& futureReward);
    void backupChanceNode(THTSSearchNode*, double const& futureReward);

    // Action selection
    int selectAction(THTSSearchNode* node);

private:
    // Vector for decision node children of equal quality
    std::vector<int> bestActionIndices;
};

#endif
