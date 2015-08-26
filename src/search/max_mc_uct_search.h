#ifndef MAX_MC_UCT_SEARCH_H
#define MAX_MC_UCT_SEARCH_H

// MaxMCUCTSearch is the UCT variant described in the ICAPS 2013 paper by Keller
// and Helmert that uses MaxMonte-Carlo backups and Monte-Carlo sampling for
// outcome selection.

#include "uct_base.h"

class MaxMCUCTSearch : public UCTBase<THTSSearchNode> {
public:
    MaxMCUCTSearch() :
        UCTBase<THTSSearchNode>("MaxMC-UCT") {
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
