#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

// This is the abstract interface all search engines are based on. It
// only implements a basic method to estimate best actions by calling
// estimateQValues, and defines a couple of member variables.

#include "caching_component.h"
#include "learning_component.h"
#include "planning_task.h"

#include <sstream>

class ProstPlanner;

class SearchEngine : public CachingComponent, public LearningComponent {
public:
    // Create a SearchEngine
    static SearchEngine* fromString(std::string& desc, ProstPlanner* planner);

    // Set parameters from command line
    virtual bool setValueFromString(std::string& param, std::string& value);

    // Learn parameter values from a training set
    bool learn(std::vector<State> const& trainingSet);

    // Start the search engine as main search engine. By default, this
    // calls estimateQValues and returns those with the highest
    // result.
    virtual void estimateBestActions(State const& _rootState, std::vector<int>& result);

    // Start the search engine for Q-value estimation
    virtual void estimateQValues(State const& _rootState, std::vector<double>& result, bool const& pruneResult) = 0;

    // This is inherited from CachingComponent and called when caching
    // is disabled because memory becomes sparse.
    void disableCaching() {
        cachingEnabled = false;
    }

    // Parameter setter
    virtual void setCachingEnabled(bool _cachingEnabled) {
        cachingEnabled = _cachingEnabled;
    }

    virtual void setPlanningTask(PlanningTask* _task) {
        task = _task;
    }
   
    virtual void setMaxSearchDepth(int _maxSearchDepth) {
        maxSearchDepth = _maxSearchDepth;
    }

    // Parameter getter
    virtual bool const& cachingIsEnabled() const {
        return cachingEnabled;
    }

    virtual int const& getMaxSearchDepth() const {
        return maxSearchDepth;
    }

    // Reset statistic variables
    virtual void resetStats() {}

    // Printer
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

    // Used for debug output only
    std::string name;

    // The main planner
    ProstPlanner* planner;

    // The used planning task
    PlanningTask* task;

    // Parameter
    bool cachingEnabled;
    int maxSearchDepth;

    // Stream for nicer (and better timed) printing
    std::stringstream outStream;
};

#endif
