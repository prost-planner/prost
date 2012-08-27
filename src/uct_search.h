#ifndef UCT_SEARCH_H
#define UCT_SEARCH_H

#include "search_engine.h"
#include "planning_task.h"

#include <sstream>

class UCTNode {
public:
    UCTNode() :
        children(), accumulatedReward(0.0), numberOfVisits(0), numberOfChildrenVisits(0), isARewardLock(false) {}
    ~UCTNode();

    std::vector<UCTNode*> children;
    double accumulatedReward;
    int numberOfVisits;
    int numberOfChildrenVisits;
    bool isARewardLock;

    double expectedRewardEstimate() {
        return accumulatedReward / (double) numberOfVisits;
    }
};

class UCTSearch : public SearchEngine {
public:
    enum TimeoutMethod {
        TIME, //stop after timeout ms
        NUMBER_OF_ROLLOUTS, //stop after maxNumberOfRollouts rollouts
        TIME_AND_NUMBER_OF_ROLLOUTS //stop after timeout ms or maxNumberOfRollouts rollouts, whichever comes first
    };

    enum RandomNumberGenerationMethod {
        STANDARD,
        EXOGENOUS
    };

    UCTSearch(ProstPlanner* _planner, std::vector<double>& _result);

    bool setValueFromString(std::string& param, std::string& value);

    virtual void setTimeoutMethod(UCTSearch::TimeoutMethod _timeoutMethod) {
        timeoutMethod = _timeoutMethod;
    }

    virtual void setTimeout(double _timeout) {
        timeout = _timeout;
    }

    virtual void setMaxNumberOfRollouts(int _maxNumberOfRollouts) {
        maxNumberOfRollouts = _maxNumberOfRollouts;
    }

    virtual void setNumberOfInitialVisits(int _numberOfInitialVisits) {
        numberOfInitialVisits = _numberOfInitialVisits;
    }

    virtual void setInitializer(SearchEngine* _initializer);

    virtual void setMagicConstantScaleFactor(double _magicConstantScaleFactor) {
        magicConstantScaleFactor = _magicConstantScaleFactor;
    }

    virtual void setAllowDecisionNodeSuccessorChoiceBasedOnVisitDifference(bool _allowDecisionNodeSuccessorChoiceBasedOnVisitDifference) {
        allowDecisionNodeSuccessorChoiceBasedOnVisitDifference = _allowDecisionNodeSuccessorChoiceBasedOnVisitDifference;
    }

    //learn
    void learn(std::vector<State> const& trainingSet);

    void resetStats();
    void print();
    void printStats(std::string indent = "");

protected:
    //main search functions
    void _run();
    void search();
    double rolloutDecisionNode(UCTNode* node);
    double rolloutChanceNodes(UCTNode* node);

    //(re-)setters of variables in various phases
    void initStep();
    void initRollout();
    void initRolloutStep();

    //successor choice
    void chooseDecisionNodeSuccessor(UCTNode* node);
    void chooseUnvisitedChild(UCTNode* node);
    void chooseDecisionNodeSuccessorBasedOnVisitDifference(UCTNode* node);
    void chooseDecisionNodeSuccessorBasedOnUCTFormula(UCTNode* node);

    //initialization of decision node and its children
    void initializeDecisionNode(UCTNode* node);
    void initializeDecisionNodeChild(UCTNode* node, unsigned int const& index, double const& initialReward);

    //if root state is a reward lock or has only one reasonable actions, it is returned here
    int getUniquePolicy();

    //memory management
    UCTNode* getUCTNode();
    void resetNodePool();

    //the root node
    UCTNode* currentRootNode;

    //the successor of the current node
    UCTNode* chosenChild;

    //UCT related variables
    double magicConstant;

    //vector for decision node children of equal quality (wrt UCT formula)
    std::vector<int> bestDecisionNodeChildren;

    //variable counting the rollouts
    int currentRollout;

    //Successor state
    State nextState;

    //the last chosen action
    int chosenActionIndex;

    //variables needed to apply UCT formula
    double numberOfChildrenVisitsLog;
    double visitPart;
    double UCTValue;
    double bestUCTValue;

    //search engine for initialization of decison node children
    SearchEngine* initializer;
    std::vector<double> initialRewards;    

    //memory management (nodePool)
    int  lastUsedNodePoolIndex;
    std::vector<UCTNode*> nodePool;

    //variables to choose successor that has been tried too rarely from UCT
    int smallestNumVisits;
    int highestNumVisits;

    //if the task is pruning equivalent to its determinization, we use the initializer to prune
    bool pruneWithInitialization;

    //parameter
    UCTSearch::TimeoutMethod timeoutMethod;
    double timeout;
    int maxNumberOfRollouts;
    int numberOfInitialVisits;
    double magicConstantScaleFactor;
    bool allowDecisionNodeSuccessorChoiceBasedOnVisitDifference;

    //statistics
    int numberOfRuns;

    //Caching
    int rewardCacheIndexForThisStep;
    static std::vector<std::map<State, std::vector<double>, State::CompareIgnoringRemainingSteps> > rewardCache;
};

#endif
