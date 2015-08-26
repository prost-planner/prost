#ifndef DP_UCT_SEARCH_H
#define DP_UCT_SEARCH_H

// DPUCTSearch is used for two of the UCT variants described in the ICAPS 2013
// paper by Keller and Helmert. If called with IDS as initializers, it
// corresponds to the search engine labelled DP-UCT in that paper which uses
// Partial Bellman backups and Monte-Carlo sampling for outcome selection. If
// the number of new decision nodes is limited to 1, it is UCT*.

#include "uct_base.h"

class DPUCTSearch : public UCTBase<THTSSearchNode> {
public:
    DPUCTSearch() :
        UCTBase<THTSSearchNode>("DP-UCT") {
        setHeuristicWeight(0.5);
    }

protected:
    // Outcome selection
    THTSSearchNode* selectOutcome(THTSSearchNode* node, PDState& nextState,
                                  int const& varIndex, int const& lastProbVarIndex);

    // Backup functions
    void backupDecisionNodeLeaf(THTSSearchNode* node, double const& futReward);
    void backupDecisionNode(THTSSearchNode* node, double const& futReward);
    void backupChanceNode(THTSSearchNode* node, double const& futReward);
};

#endif
