#ifndef MAX_MC_UCT_SEARCH_H
#define MAX_MC_UCT_SEARCH_H

// MaxMCUCTSearch is the UCT variant described in the ICAPS 2013 paper
// by Keller and Helmert that uses MaxMonte-Carlo backups and
// Monte-Carlo sampling for outcome selection.

#include "uct_base.h"

class MaxMCUCTNode { //Max Monte Carlo UCTNode
public:
    MaxMCUCTNode() :
        children(),
        immediateReward(0.0),
        futureReward(-std::numeric_limits<double>::max()),
        numberOfVisits(0),
        rewardLock(false) {}

    ~MaxMCUCTNode() {
        for(unsigned int i = 0; i < children.size(); ++i) {
            if(children[i]) {
                delete children[i];
            }
        }
    }

    friend class MaxMCUCTSearch;

    void reset() {
        children.clear();
        immediateReward = 0.0;
        futureReward = -std::numeric_limits<double>::max();
        numberOfVisits = 0;
        rewardLock = false;
    }

    double getExpectedRewardEstimate() const {
        return (immediateReward + futureReward);
    }

    double getExpectedFutureRewardEstimate() const {
        return futureReward;
    }

    bool isSolved() const {
        return false;
    }

    bool const& isARewardLock() const {
        return rewardLock;
    }

    void setRewardLock(bool const& _rewardLock) {
        rewardLock = _rewardLock;
    }

    int const& getNumberOfVisits() const {
        return numberOfVisits;
    }

    // Print
    void print(std::ostream& out, std::string indent = "") const {
        out << indent << getExpectedRewardEstimate() << " (in " << numberOfVisits << " visits)" << std::endl;
    }

    std::vector<MaxMCUCTNode*> children;

private:
    double immediateReward;
    double futureReward;
    int numberOfVisits;

    bool rewardLock;
};

class MaxMCUCTSearch : public UCTBase<MaxMCUCTNode> {
public:
    MaxMCUCTSearch(ProstPlanner* _planner) :
        UCTBase<MaxMCUCTNode>("MaxMC-UCT", _planner),
        heuristicWeight(0.5) {}

    // Set parameters from command line
    bool setValueFromString(std::string& param, std::string& value) {
        if(param == "-hw") {
            setHeuristicWeight(atof(value.c_str()));
            return true;
        }

        return UCTBase<MaxMCUCTNode>::setValueFromString(param, value);
    }

    // Parameter setter
    virtual void setHeuristicWeight(double _heuristicWeight) {
        heuristicWeight = _heuristicWeight;
    }

protected:
    // Initialization
    void initializeDecisionNodeChild(MaxMCUCTNode* node, unsigned int const& index, double const& initialQValue);

    // Outcome selection
    MaxMCUCTNode* selectOutcome(MaxMCUCTNode* node, PDState& nextPDState, State& nextState, int& varIndex);

    // Backup functions
    void backupDecisionNodeLeaf(MaxMCUCTNode* node, double const& immReward, double const& futReward);
    void backupDecisionNode(MaxMCUCTNode* node, double const& immReward, double const& futReward);
    void backupChanceNode(MaxMCUCTNode* node, double const& futReward);

    // Parameter
    double heuristicWeight;
};

#endif
