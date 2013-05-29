#ifndef MAX_MC_UCT_SEARCH_H
#define MAX_MC_UCT_SEARCH_H

// MaxMCUCTSearch is the UCT variant described in the ICAPS 2013 paper
// that uses MaxMonte-Carlo backups and Monte-Carlo sampling for
// outcome selection.

#include "uct_base.h"

class MaxMCUCTNode { //Max Monte Carlo UCTNode
public:
    MaxMCUCTNode() :
        children(),
        reward(0.0),
        numberOfVisits(0),
        numberOfChildrenVisits(0),
        rewardLock(false) {}

    ~MaxMCUCTNode() {
        for(unsigned int i = 0; i < children.size(); ++i) {
            if(children[i]) {
                delete children[i];
            }
        }
    }

    // Reset is called when a node is reused
    void reset() {
        children.clear();
        reward = 0.0;
        numberOfVisits = 0;
        numberOfChildrenVisits = 0;
        rewardLock = false;
    }

    // This is used in the UCT formula as the Q-value estimate and in
    // the decision which successor node is the best
    double getExpectedRewardEstimate() const {
        return reward;
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

    std::vector<MaxMCUCTNode*> children;

    double reward;
    int numberOfVisits;
    int numberOfChildrenVisits;

    bool rewardLock;
};

class MaxMCUCTSearch : public UCTBase<MaxMCUCTNode> {
public:
    MaxMCUCTSearch(ProstPlanner* _planner) :
        UCTBase<MaxMCUCTNode>("MaxMC-UCT", _planner),
        heuristicWeight(0.5) {}

    // Search engine creation
    bool setValueFromString(std::string& param, std::string& value) {
        if(param == "-hw") {
            setHeuristicWeight(atof(value.c_str()));
            return true;
        }

        return UCTBase<MaxMCUCTNode>::setValueFromString(param, value);
    }

    // Parameter setters: new parameters
    virtual void setHeuristicWeight(double _heuristicWeight) {
        heuristicWeight = _heuristicWeight;
    }

protected:
    // Initialization
    void initializeDecisionNodeChild(MaxMCUCTNode* node, unsigned int const& index, double const& initialQValue);

    // Outcome selection
    MaxMCUCTNode* selectOutcome(MaxMCUCTNode* node, State& stateAsProbDistr, int& varIndex);

    // Backup functions
    void backupDecisionNodeLeaf(MaxMCUCTNode* node, double const& immReward, double const& futReward);
    void backupDecisionNode(MaxMCUCTNode* node, double const& immReward, double const& futReward);
    void backupChanceNode(MaxMCUCTNode* node, double const& futReward);

    // Parameter
    double heuristicWeight;
};

#endif
