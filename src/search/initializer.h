#ifndef INITIALIZER_H
#define INITIALIZER_H

#include <string>

#include "states.h"

class THTS;
class SearchEngine;
class SearchNode;

class Initializer {
public:
    // Create an Initializer
    static Initializer* fromString(std::string& desc, THTS* thts);

    // Set parameters from command line
    virtual bool setValueFromString(std::string& param, std::string& value);

    // Learns parameter values from a random training set.
    virtual void learn();

    // This is called when caching is disabled because memory becomes sparse.
    virtual void disableCaching();

    // Parameter setter
    virtual void setHeuristic(SearchEngine* _heuristic);

    virtual void setHeuristicWeight(double _heuristicWeight) {
        heuristicWeight = _heuristicWeight;
    }

    virtual void setNumberOfInitialVisits(int _numberOfInitialVisits) {
        numberOfInitialVisits = _numberOfInitialVisits;
    }

    // Functions that pass/return parameters to/from the used heuristic
    virtual void setMaxSearchDepth(int maxSearchDepth);

    // Print
    virtual void printStats(std::ostream& out,
                            bool const& printRoundStats,
                            std::string indent = "") const;

    // This is true if node has already been initialized
    virtual bool isInitialized(SearchNode* node) = 0;

    // This is called if isInitialized
    virtual void initialize(SearchNode* node, State const& current) = 0;

protected:
    Initializer(THTS* _thts, std::string _name) :
        thts(_thts),
        name(_name),
        heuristic(nullptr),
        heuristicWeight(0.5),
        numberOfInitialVisits(1) {}

    ~Initializer();

    THTS* thts;
    std::string name;

    // Parameter
    SearchEngine* heuristic;
    double heuristicWeight;
    int numberOfInitialVisits;
};

class ExpandNodeInitializer : public Initializer {
public:
    ExpandNodeInitializer(THTS* _thts) :
        Initializer(_thts, "Expand") {}

    virtual bool isInitialized(SearchNode* node) override;
    virtual void initialize(SearchNode* node, State const& current) override;
};

#endif
