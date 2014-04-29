#ifndef BREADTH_FIRST_SEARCH_H
#define BREADTH_FIRST_SEARCH_H

// Implements a basic breadth first search. The nodes are basically the same as
// the DP-UCT nodes, but instead of UCT forumlae we use breadth first search to
// choose the next decision

#include "thts.h"

class BfsNode {
public:
    BfsNode() :
        children(),
        immediateReward(0.0),
        futureReward(-std::numeric_limits<double>::max()),
        numberOfVisits(0),
        prob(0.0),
        solved(false),
        rewardLock(false) {}

    ~BfsNode() {
        for(unsigned int i = 0; i < children.size(); ++i) {
            if(children[i]) {
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
        return (immediateReward + futureReward);
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
        if(solved) {
            out << indent << "SOLVED with: " << getExpectedRewardEstimate() << " (in " << numberOfVisits << " real visits)" << std::endl;
        } else {
            out << indent << getExpectedRewardEstimate() << " (in " << numberOfVisits << " real visits)" << std::endl;
        }
    }

    std::vector<BfsNode*> children;

private:
    double immediateReward;
    double futureReward;
    int numberOfVisits;
    double prob;
    bool solved; 
    bool rewardLock;

    FRIEND_TEST(bfsSearchTest, testSelectAction);
};

class BreadthFirstSearch : public THTS<BfsNode> {
public:
    BreadthFirstSearch() :
        THTS<BfsNode>("Breadth-First-Search") {}

protected: 

    // Initialization
    void initializeDecisionNodeChild(BfsNode* node, 
            unsigned int const& actionIndex, double const& initialQValue);


    // Outcome selection
    BfsNode* selectOutcome(BfsNode*, PDState& nextState, int& varIndex);

    // Backup function
    void backupDecisionNodeLeaf(BfsNode* node, double const& immReward,
            double const& futureReward);
    void backupDecisionNode(BfsNode*, double const& immReward,
            double const& futureReward);
    void backupChanceNode(BfsNode*, double const& futureReward);    

    // Action selection
    int selectAction(BfsNode* node);

    // Memory Management
    BfsNode* getRootNode() {
        return getBfsNode(1.0);
    }

    // TODO Do above methods have to be protected or is public ok?
    FRIEND_TEST(bfsSearchTest, testInitializeDecisionNodeChild);
    FRIEND_TEST(bfsSearchTest, testBackupDecisionNodeLeaf);
    FRIEND_TEST(bfsSearchTest, testBackupDecisionNode);
    FRIEND_TEST(bfsSearchTest, testBackupChanceNode);
    FRIEND_TEST(bfsSearchTest, testSelectAction);

private:
    // Memory management
    BfsNode* getBfsNode(double const& _prob) {
        BfsNode* res = THTS<BfsNode>::getSearchNode();
        res->prob = _prob;
        return res;
    }
};

#endif 
