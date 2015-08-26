#ifndef MC_UCT_SEARCH_H
#define MC_UCT_SEARCH_H

// MCUCTSearch is the "standard" UCT variant that uses Monte-Carlo backups and
// Monte-Carlo sampling for outcome selection.

#include "uct_base.h"

class MCUCTNode { //Monte Carlo UCTNode
public:
    MCUCTNode(double const& _prob, int const& _remainingSteps) :
        children(),
        immediateReward(0.0),
        prob(_prob),
        remainingSteps(_remainingSteps),
        futureReward(-std::numeric_limits<double>::max()),
        numberOfVisits(0),
        solved(false) {}

    ~MCUCTNode() {
        for (unsigned int i = 0; i < children.size(); ++i) {
            if (children[i]) {
                delete children[i];
            }
        }
    }

    void reset(double const& _prob, int const& _remainingSteps) {
        children.clear();
        immediateReward = 0.0;
        prob = _prob;
        remainingSteps = _remainingSteps;
        futureReward = -std::numeric_limits<double>::max();
        numberOfVisits = 0;
        solved = false;
    }

    double getExpectedRewardEstimate() const {
        return immediateReward + futureReward;
    }

    double getExpectedFutureRewardEstimate() const {
        return futureReward;
    }

    bool isSolved() const {
        return false;
    }

    int const& getNumberOfVisits() const {
        return numberOfVisits;
    }

    // Print
    void print(std::ostream& out, std::string indent = "") const {
        out << indent << getExpectedRewardEstimate() << " (in " <<
        numberOfVisits << " visits)" << std::endl;
    }

    std::vector<MCUCTNode*> children;

    double immediateReward;
    double prob;
    int remainingSteps;
     
    double futureReward;
    int numberOfVisits;

    bool solved;
};

class MCUCTSearch : public UCTBase<MCUCTNode> {
public:
    MCUCTSearch() :
        UCTBase<MCUCTNode>("MC-UCT"),
        initialLearningRate(1.0),
        learningRateDecay(1.0) {}

protected:
    // Outcome selection
    MCUCTNode* selectOutcome(MCUCTNode* node, PDState& nextState,
                             int const& varIndex, int const& lastProbVarIndex);

    // Backup functions
    void backupDecisionNodeLeaf(MCUCTNode* node, double const& futReward);
    void backupDecisionNode(MCUCTNode* node, double const& futReward);
    void backupChanceNode(MCUCTNode* node, double const& futReward);

    double initialLearningRate;
    double learningRateDecay;
};

#endif
