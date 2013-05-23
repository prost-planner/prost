#ifndef MC_UCT_SEARCH_H
#define MC_UCT_SEARCH_H

// MCUCTSearch is the "standard" UCT variant that uses Monte-Carlo
// backups and Monote-Carlo sampling for outcome selection.

#include "uct_base.h"

class MCUCTNode {
public:
    MCUCTNode() :
        children(),
        accumulatedReward(0.0),
        numberOfVisits(0),
        numberOfChildrenVisits(0),
        rewardLock(false) {}

    ~MCUCTNode() {
        for(unsigned int i = 0; i < children.size(); ++i) {
            if(children[i]) {
                delete children[i];
            }
        }
    }

    // Reset is called when a node is reused
    void reset() {
        children.clear();
        accumulatedReward = 0.0;
        numberOfVisits = 0;
        numberOfChildrenVisits = 0;
        rewardLock = false;
    }

    // This is used in the UCT formula as the Q-value estimate and in
    // the decision which successor node is the best
    double getExpectedRewardEstimate() const {
        return accumulatedReward / (double) numberOfVisits;
    }

    // This is only used for solve labeling which is not supported
    double getExpectedReward() const {
        assert(false);
        return -std::numeric_limits<double>::max();
    }

    // Plain MonteCarlo-UCT does not support solve labeling
    bool isSolved() const {
        return false;
    }

    // A label that is true if the assigned state is a reward lock.
    // Then, this node doesn't have any children.
    bool& isARewardLock() {
        return rewardLock;
    }

    // Print
    void print(std::ostream& out, std::string indent = "") const {
        out << indent << getExpectedRewardEstimate() << " (in " << numberOfVisits << " visits)" << std::endl;
    }

    std::vector<MCUCTNode*> children;

    double accumulatedReward;
    int numberOfVisits;
    int numberOfChildrenVisits;

    bool rewardLock;
};

class MCUCTSearch : public UCTBase<MCUCTNode> {
public:
    MCUCTSearch(ProstPlanner* _planner) :
        UCTBase<MCUCTNode>("MC-UCT", _planner) {}

protected:
    // Initialization
    void initializeDecisionNodeChild(MCUCTNode* node, unsigned int const& index, double const& initialQValue);

    // Outcome selection
    MCUCTNode* selectOutcome(MCUCTNode* node, State& stateAsProbDistr, int& varIndex);

    // Backup functions
    void backupDecisionNodeLeaf(MCUCTNode* node, double const& immReward, double const& futReward) {
        // This is only different from backupDecisionNode if we want
        // to label nodes as solved, which is impossible in
        // MonteCarlo-UCT.
        backupDecisionNode(node, immReward, futReward);
    }
    void backupDecisionNode(MCUCTNode* node, double const& immReward, double const& futReward);
    void backupChanceNode(MCUCTNode* node, double const& futReward);
};

#endif
