#ifndef MC_UCT_SEARCH_H
#define MC_UCT_SEARCH_H

// MCUCTSearch is the "standard" UCT variant that uses Monte-Carlo backups and
// Monte-Carlo sampling for outcome selection.

#include "uct_base.h"

class MCUCTSearch : public UCTBase<THTSSearchNode> {
public:
    MCUCTSearch() :
        UCTBase<THTSSearchNode>("MC-UCT"),
        initialLearningRate(1.0),
        learningRateDecay(1.0) {}

protected:
    // Outcome selection
    THTSSearchNode* selectOutcome(THTSSearchNode* node, PDState& nextState,
                                  int const& varIndex, int const& lastProbVarIndex);

    // Backup functions
    void backupDecisionNodeLeaf(THTSSearchNode* node, double const& futReward);
    void backupDecisionNode(THTSSearchNode* node, double const& futReward);
    void backupChanceNode(THTSSearchNode* node, double const& futReward);

    double initialLearningRate;
    double learningRateDecay;
};

#endif
