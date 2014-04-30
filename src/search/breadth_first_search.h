#ifndef BREADTH_FIRST_SEARCH_H
#define BREADTH_FIRST_SEARCH_H

// Implements a basic breadth first search. The nodes are basically the same as
// the DP-UCT nodes, but instead of UCT forumlae we use breadth first search to
// choose the next decision

#include "thts.h"

class BFSNode {
public:
    BFSNode() :
        children(),
        immediateReward(0.0),
        futureReward(-std::numeric_limits<double>::max()),
        numberOfVisits(0),
        prob(0.0),
        solved(false),
        rewardLock(false) {}

    ~BFSNode() {
        for (unsigned int i = 0; i < children.size(); ++i) {
            if (children[i]) {
                delete children[i];
            }
        }
    }

    friend class BreadthFirstSearch;

    void reset() {
        children.clear();
        immediateReward = 0.0;
        futureReward = -std::numeric_limits<double>::max();
        numberOfVisits = 0;
        prob = 0.0;
        solved = false;
        rewardLock = false;
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

    bool const& isARewardLock() const {
        return rewardLock;
    }

    void setRewardLock(bool const& _rewardLock) {
        rewardLock = _rewardLock;
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

private:
    double immediateReward;
    double futureReward;
    int numberOfVisits;
    double prob;
    bool solved;
    bool rewardLock;

#ifndef NDEBUG
    FRIEND_TEST(bfsSearchTest, testSelectAction);
#endif
};

class BreadthFirstSearch : public THTS<BFSNode> {
public:
    BreadthFirstSearch() :
        THTS<BFSNode>("Breadth-First-Search") {}

protected:

    // Initialization
    void initializeDecisionNodeChild(BFSNode* node,
            unsigned int const& actionIndex,
            double const& initialQValue);


    // Outcome selection
    BFSNode* selectOutcome(BFSNode*, PDState& nextState, int& varIndex);

    // Backup function
    void backupDecisionNodeLeaf(BFSNode* node, double const& immReward,
            double const& futureReward);
    void backupDecisionNode(BFSNode*, double const& immReward,
            double const& futureReward);
    void backupChanceNode(BFSNode*, double const& futureReward);

    // Action selection
    int selectAction(BFSNode* node);

    // Memory Management
    BFSNode* getRootNode() {
        return getBFSNode(1.0);
    }

#ifndef NDEBUG
    FRIEND_TEST(bfsSearchTest, testInitializeDecisionNodeChild);
    FRIEND_TEST(bfsSearchTest, testBackupDecisionNodeLeaf);
    FRIEND_TEST(bfsSearchTest, testBackupDecisionNode);
    FRIEND_TEST(bfsSearchTest, testBackupChanceNode);
    FRIEND_TEST(bfsSearchTest, testSelectAction);
#endif

private:
    // Memory management
    BFSNode* getBFSNode(double const& _prob) {
        BFSNode* res = THTS<BFSNode>::getSearchNode();
        res->prob = _prob;
        return res;
    }
};

#endif
