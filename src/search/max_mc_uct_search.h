#ifndef MAX_MC_UCT_SEARCH_H
#define MAX_MC_UCT_SEARCH_H

// MaxMCUCTSearch is the UCT variant described in the ICAPS 2013 paper
// by Keller and Helmert that uses MaxMonte-Carlo backups and
// Monte-Carlo sampling for outcome selection.

#include "uct_base.h"

class MaxMCUCTNode { //Max Monte Carlo UCTNode
public:
    MaxMCUCTNode(double const& _prob, int const& _remainingSteps) :
        children(),
        immediateReward(0.0),
        prob(_prob),
        remainingSteps(_remainingSteps),
        futureReward(-std::numeric_limits<double>::max()),
        numberOfVisits(0),
        solved(false) {}

    ~MaxMCUCTNode() {
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

    std::vector<MaxMCUCTNode*> children;

    double immediateReward;
    double prob;
    int remainingSteps;
    
    double futureReward;
    int numberOfVisits;

    bool solved;
};

class MaxMCUCTSearch : public UCTBase<MaxMCUCTNode> {
public:
    MaxMCUCTSearch() :
        UCTBase<MaxMCUCTNode>("MaxMC-UCT") {
        setHeuristicWeight(0.5);
    }

protected:
    // Outcome selection
    MaxMCUCTNode* selectOutcome(MaxMCUCTNode* node, PDState& nextState,
                                int const& varIndex, int const& lastProbVarIndex);

    // Backup functions
    void backupDecisionNodeLeaf(MaxMCUCTNode* node, double const& futReward);
    void backupDecisionNode(MaxMCUCTNode* node, double const& futReward);
    void backupChanceNode(MaxMCUCTNode* node, double const& futReward);
};

#endif
