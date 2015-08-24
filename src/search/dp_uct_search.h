#ifndef DP_UCT_SEARCH_H
#define DP_UCT_SEARCH_H

// DPUCTSearch is used for two of the UCT variants described in the
// ICAPS 2013 paper by Keller and Helmert. If called with IDS as
// initializers, it corresponds to the search engine labelled DP-UCT
// in that paper which uses Partial Bellman backups and Monte-Carlo
// sampling for outcome selection. If the number of new decision nodes
// is limited to 1, it is UCT*.

#include "uct_base.h"

class DPUCTNode {
public:
    DPUCTNode(double const& _prob, int const& _remainingSteps) :
        children(),
        immediateReward(0.0),
        prob(_prob),
        remainingSteps(_remainingSteps),
        futureReward(-std::numeric_limits<double>::max()),
        numberOfVisits(0),
        solved(false) {}

    ~DPUCTNode() {
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

    bool const& isSolved() const {
        return solved;
    }

    int const& getNumberOfVisits() const {
        return numberOfVisits;
    }

    void print(std::ostream& out, std::string indent = "") const {
        if (solved) {
            out << indent << "SOLVED with: " << getExpectedRewardEstimate() <<
            " (in " << numberOfVisits << " real visits)" << std::endl;
        } else {
            out << indent << getExpectedRewardEstimate() << " (in " <<
            numberOfVisits << " real visits)" << std::endl;
        }
    }

    std::vector<DPUCTNode*> children;

    double immediateReward;
    double prob;
    int remainingSteps;
    
    double futureReward;
    int numberOfVisits;

    bool solved;
};

class DPUCTSearch : public UCTBase<DPUCTNode> {
public:
    DPUCTSearch() :
        UCTBase<DPUCTNode>("DP-UCT") {
        setHeuristicWeight(0.5);
    }

protected:
    // Outcome selection
    DPUCTNode* selectOutcome(DPUCTNode* node, PDState& nextState,
                             int const& varIndex, int const& lastProbVarIndex);

    // Backup functions
    void backupDecisionNodeLeaf(DPUCTNode* node, double const& futReward);
    void backupDecisionNode(DPUCTNode* node, double const& futReward);
    void backupChanceNode(DPUCTNode* node, double const& futReward);
};

#endif
