#ifndef DP_UCT_SEARCH_H
#define DP_UCT_SEARCH_H

// DPUCTSearch is used for two of the UCT variant described in the
// ICAPS 2013 paper by Keller and Helmert. If called with IDS as
// initializers, it corresponds to the search engine labelled DP-UCT
// in that paper which uses Partial Bellman backups and Monte-Carlo
// sampling for outcome selection. If the number of new decision nodes
// is limited to 1, it is UCT*.

#include "uct_base.h"

class DPUCTNode {
public:
    DPUCTNode() :
        children(),
        immediateReward(0.0),
        futureReward(-std::numeric_limits<double>::max()),
        numberOfVisits(0),
        prob(0.0),
        solved(false),
        rewardLock(false) {}

    ~DPUCTNode() {
        for(unsigned int i = 0; i < children.size(); ++i) {
            if(children[i]) {
                delete children[i];
            }
        }
    }

    friend class DPUCTSearch;

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
        return futureReward + immediateReward;
    }

    double getExpectedFutureRewardEstimate() const {
        return futureReward;
    }

    bool const& isSolved() const {
        return solved;
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

    void print(std::ostream& out, std::string indent = "") const {
        if(solved) {
            out << indent << "SOLVED with: " << getExpectedRewardEstimate() << " (in " << numberOfVisits << " real visits)" << std::endl;
        } else {
            out << indent << getExpectedRewardEstimate() << " (in " << numberOfVisits << " real visits)" << std::endl;
        }
    }

    std::vector<DPUCTNode*> children;

private:
    double immediateReward;
    double futureReward;

    int numberOfVisits;

    double prob;
    bool solved;
    bool rewardLock;
};

class DPUCTSearch : public UCTBase<DPUCTNode> {
public:
    DPUCTSearch() :
        UCTBase<DPUCTNode>("DP-UCT"),
        heuristicWeight(0.5) {}

    // Set parameters from command line
    bool setValueFromString(std::string& param, std::string& value) {
        if(param == "-hw") {
            setHeuristicWeight(atof(value.c_str()));
            return true;
        }

        return UCTBase<DPUCTNode>::setValueFromString(param, value);
    }

    // Parameter setter
    virtual void setHeuristicWeight(double _heuristicWeight) {
        heuristicWeight = _heuristicWeight;
    }

protected:
    // Initialization of nodes
    void initializeDecisionNodeChild(DPUCTNode* node, unsigned int const& actionIndex, double const& initialQValue);

    // Outcome selection
    DPUCTNode* selectOutcome(DPUCTNode* node, PDState& nextPDState, State& nextState, int& varIndex);

    // Backup functions
    void backupDecisionNodeLeaf(DPUCTNode* node, double const& immReward, double const& futReward);
    void backupDecisionNode(DPUCTNode* node, double const& immReward, double const& futReward);
    void backupChanceNode(DPUCTNode* node, double const& futReward);

    // Memory management
    DPUCTNode* getRootNode() {
        return getDPUCTNode(1.0);
    }

private:
    // Memory management
    DPUCTNode* getDPUCTNode(double const& prob) {
        DPUCTNode* res = UCTBase<DPUCTNode>::getSearchNode();
        res->prob = prob;
        return res;
    }

    // Parameter
    double heuristicWeight;
};

#endif
