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
    static SearchEngine* fromString(std::string& desc, ProstPlanner* planner);

    //set parameters from string
    virtual bool setValueFromString(std::string& param, std::string& value);

    //this is called before the first run starts
    bool learn(std::vector<State> const& trainingSet);

    //estimate the q-values of all (pruned) actions 
    virtual void estimateQValues(State const& _rootState, std::vector<double>& result, bool const& pruneResult) = 0;

    //estimate the best actions (no q-value estimates necessary)
    virtual void estimateBestActions(State const& _rootState, std::vector<int>& result) = 0;

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
    }
   
    virtual void setMaxSearchDepth(int _maxSearchDepth) {
        maxSearchDepth = _maxSearchDepth;
    }

    //getter methods of various settings
    virtual bool const& cachingIsEnabled() const {
        return cachingEnabled;
    }

    virtual int const& getMaxSearchDepth() const {
        return maxSearchDepth;
    }

    //reset statistic variables.
    virtual void resetStats() {}

    //printer
    virtual void print(std::ostream& out);
    virtual void printStats(std::ostream& out, bool const& printRoundStats, std::string indent = "");

protected:
    SearchEngine(std::string _name, ProstPlanner* _planner, PlanningTask* _task) :
        CachingComponent(_planner),
        LearningComponent(_planner),
        name(_name),
        planner(_planner), 
        task(_task),
        cachingEnabled(true),
        maxSearchDepth(_task->getHorizon()) {}

    //used for logging
    std::string name;

    //the main planner
    ProstPlanner* planner;

    //the used planning task
    PlanningTask* task;

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
