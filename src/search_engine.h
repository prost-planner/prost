#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include "caching_component.h"
#include "learning_component.h"
#include "planning_task.h"

#include <sstream>
//#include <boost/thread.hpp>

class ProstPlanner;

class SearchEngine : public CachingComponent, public LearningComponent {
public:
    //create a SearchEngine
    static SearchEngine* fromString(std::string& desc, ProstPlanner* planner, std::vector<double>& searchEngineResult);

    //set parameters from string
    virtual bool setValueFromString(std::string& param, std::string& value);

    //ResultType defines the kind of result this search engine gives when run() is called
    enum ResultType {
        RELATIVE, // the values assigned to actions do not matter, just the ordering (used for toplevel search engines)
        PRUNED_ESTIMATE, // any q-value estimate for all actions applicable in that task, and -infty else (used if action pruning is possible with initializer)
        ESTIMATE, // any q-value estimate for all actions (used for initialization)
        LOWER_BOUND_ESTIMATE, // the q-value estimates are lower bound estimates in the planning task assigned to this search engine
        UPPER_BOUND_ESTIMATE // the q-value estimates are upper bound estimates in the planning task assigned to this search engine
    };

    //this is called before actual runs start
    virtual void learn(std::vector<State> const& /*trainingSet*/) {}

    //starts the search engine
    void run(State const& _rootState);

    //join the thread
    //void join();

    //this function is inherited from CachingComponent and called when caching is disabled because memory becomes sparse
    void disableCaching() {
        cachingEnabled = false;
    }

    //parameter setters
    virtual void setCachingEnabled(bool _cachingEnabled) {
        cachingEnabled = _cachingEnabled;
    }

    virtual void setPlanningTask(PlanningTask* _task) {
        task = _task;
        rootState = State(task->getStateSize());
        currentState = State(task->getStateSize());
        actionsToExpand = std::vector<int>(task->getNumberOfActions(),0);
    }
   
    virtual void setMaxSearchDepth(int _maxSearchDepth) {
        maxSearchDepth = _maxSearchDepth;
    }

    virtual void setResultType(SearchEngine::ResultType const _resultType) {
        resultType = _resultType;
    }

    //getter methods of various settings
    virtual bool const& cachingIsEnabled() const {
        return cachingEnabled;
    }

    virtual int const& getMaxSearchDepth() const {
        return maxSearchDepth;
    }

    virtual SearchEngine::ResultType const& getResultType() const {
        return resultType;
    }

    //reset the statistic variables. when you overwrite these, you
    //should usually call resetStats() of the super class
    virtual void resetStats() {}

    //printer
    virtual void print();
    virtual void printStats(std::string indent = "");

protected:
    SearchEngine(std::string _name, ProstPlanner* _planner, PlanningTask* _task, std::vector<double>& _result) :
        CachingComponent(_planner),
        LearningComponent(_planner),
        name(_name),
        planner(_planner), 
        task(_task), 
        rootState(task->getStateSize()),
        currentState(task->getStateSize()),
        result(_result),
        actionsToExpand(_task->getNumberOfActions(),0),
        resultType(SearchEngine::RELATIVE),
        cachingEnabled(true),
        maxSearchDepth(15) {}

    //main search function that is called from run(), implement the search code here.
    virtual void _run() = 0;

    //used for logging
    std::string name;

    //the main planner
    ProstPlanner* planner;

    //the used planning task
    PlanningTask* task;

    //initial state
    State rootState;

    //currentState
    State currentState;

    //the result of this search engine's run is written to this
    std::vector<double>& result;

    //action i is supposed to be expanded if actionsToExpand[i] == i; 
    //otherwise, it is equivalent to actionsToExpand[i];
    std::vector<int> actionsToExpand;

    //the type of result produced by this search engine
    SearchEngine::ResultType resultType;

    //parameters that can be set
    bool cachingEnabled;
    int maxSearchDepth;
    //bool runInThread;

    //stream for nicer printing (via print())
    std::stringstream outStream;

    //the thread to run this search engine
    //boost::thread thr;
};

#endif
