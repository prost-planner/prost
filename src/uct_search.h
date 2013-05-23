#ifndef UCT_SEARCH_H
#define UCT_SEARCH_H

#include "prost_planner.h"
#include "thts.h"
#include "planning_task.h"

class UCTNode {
public:
    UCTNode() :
        children(),
        accumulatedReward(0.0),
        numberOfVisits(0),
        numberOfChildrenVisits(0),
        rewardLock(false) {}

    ~UCTNode() {
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

    std::vector<UCTNode*> children;

    double accumulatedReward;
    int numberOfVisits;
    int numberOfChildrenVisits;

    bool rewardLock;
};

class UCTSearch : public THTS<UCTNode> {
public:
    UCTSearch(ProstPlanner* _planner) :
        THTS<UCTNode>("UCT", _planner, _planner->getProbabilisticTask()), 
        numberOfInitialVisits(5),
        magicConstantScaleFactor(1.0) {}


    bool setValueFromString(std::string& param, std::string& value);

    virtual void setNumberOfInitialVisits(int _numberOfInitialVisits) {
        numberOfInitialVisits = _numberOfInitialVisits;
    }

    virtual void setInitializer(SearchEngine* _initializer);

    virtual void setMagicConstantScaleFactor(double _magicConstantScaleFactor) {
        magicConstantScaleFactor = _magicConstantScaleFactor;
    }

protected:
    // Initialization
    void initializeDecisionNodeChild(UCTNode* node, unsigned int const& index, double const& initialQValue);

    // Action selection
    void selectAction(UCTNode* node);
    void chooseUnvisitedChild(UCTNode* node);
    void chooseDecisionNodeSuccessorBasedOnVisitDifference(UCTNode* node);
    void chooseDecisionNodeSuccessorBasedOnUCTFormula(UCTNode* node);

    // Outcome selection
    void selectOutcome(UCTNode* node);

    // Backup functions
    void backupDecisionNodeLeaf(UCTNode* node, double const& immReward, double const& futReward) {
        // This is only different from backupDecisionNode if we want
        // to label nodes as solved, which is impossible in
        // MonteCarlo-UCT.
        backupDecisionNode(node, immReward, futReward);
    }
    void backupDecisionNode(UCTNode* node, double const& immReward, double const& futReward);
    void backupChanceNode(UCTNode* node, double const& futReward);

    //vector for decision node children of equal quality (wrt UCT formula)
    std::vector<int> bestDecisionNodeChildren;

    //variables needed to apply UCT formula
    double magicConstant;
    double numberOfChildrenVisitsLog;
    double visitPart;
    double UCTValue;
    double bestUCTValue;

    //variables to choose successor that has been tried too rarely from UCT
    int smallestNumVisits;
    int highestNumVisits;

    //parameter
    int numberOfInitialVisits;
    double magicConstantScaleFactor;
};

#endif
