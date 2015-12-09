#ifndef OUTCOME_SELECTION_H
#define OUTCOME_SELECTION_H

#include "states.h"

class THTS;
class SearchNode;

class OutcomeSelection {
public:
    // Create an outcome selection component
    static OutcomeSelection* fromString(std::string& desc, THTS* thts);
    
    // Set parameters from command line
    virtual bool setValueFromString(std::string& /*param*/, std::string& /*value*/) {
        return false;
    }

    // Learn parameter values from a random training set.
    virtual void learn() {}

    // This is called when caching is disabled because memory becomes sparse.
    virtual void disableCaching() {}

    virtual void initRound() {}
    virtual void initTrial() {}

    // Outcome selection
    virtual SearchNode* selectOutcome(SearchNode* node,
                                      PDState& nextState,
                                      int const& varIndex,
                                      int const& lastProbVarIndex) = 0;

    // Prints statistics
    virtual void printStats(std::ostream& /*out*/, std::string /*indent*/) {}

protected:
    OutcomeSelection(THTS* _thts) :
        thts(_thts) {}

    THTS* thts;
};

class MCOutcomeSelection : public OutcomeSelection {
public:
    MCOutcomeSelection(THTS* _thts) :
        OutcomeSelection(_thts) {}

    virtual SearchNode* selectOutcome(SearchNode* node,
                                      PDState& nextState,
                                      int const& varIndex,
                                      int const& lastProbVarIndex);
};

class UnsolvedMCOutcomeSelection : public OutcomeSelection {
public:
    UnsolvedMCOutcomeSelection(THTS* _thts) :
        OutcomeSelection(_thts) {}

    virtual SearchNode* selectOutcome(SearchNode* node,
                                      PDState& nextState,
                                      int const& varIndex,
                                      int const& lastProbVarIndex);
};

#endif
