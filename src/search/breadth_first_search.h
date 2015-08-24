#ifndef BREADTH_FIRST_SEARCH_H
#define BREADTH_FIRST_SEARCH_H

// Implements a basic breadth first search. The nodes are basically the same as
// the DP-UCT nodes, but instead of UCT forumlae we use breadth first search to
// choose the next decision

#include "thts.h"

class BFSNode {
public:
    BFSNode(double const& _prob, int const& _remainingSteps) :
        children(),
        immediateReward(0.0),
        prob(_prob),
        remainingSteps(_remainingSteps),
        futureReward(-std::numeric_limits<double>::max()),
        numberOfVisits(0),
        solved(false) {}

    ~BFSNode() {
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

    int const& getNumberOfVisits() const {
        return numberOfVisits;
    }

    bool isSolved() const {
        return solved;
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

    std::vector<BFSNode*> children;

    double immediateReward;
    double prob;
    int remainingSteps;
    
    double futureReward;
    int numberOfVisits;

    bool solved;
};

class BreadthFirstSearch : public THTS<BFSNode> {
public:
    BreadthFirstSearch() :
        THTS<BFSNode>("Breadth-First-Search") {
        setHeuristicWeight(1.0);
    }

protected:
    // Outcome selection
    BFSNode* selectOutcome(BFSNode*, PDState& nextState,
                           int const& varIndex, int const& lastProbVarIndex);

    // Backup function
    void backupDecisionNodeLeaf(BFSNode* node, double const& futureReward);
    void backupDecisionNode(BFSNode*, double const& futureReward);
    void backupChanceNode(BFSNode*, double const& futureReward);

    // Action selection
    int selectAction(BFSNode* node);

private:
    // Vector for decision node children of equal quality
    std::vector<int> bestActionIndices;
};

#endif
