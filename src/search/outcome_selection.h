#ifndef OUTCOME_SELECTION_H
#define OUTCOME_SELECTION_H

#include "states.h"

template<typename SearchNode> class THTS;

template <class SearchNode>
class OutcomeSelection {
public:
    // Set parameters from command line
    virtual bool setValueFromString(std::string& /*param*/, std::string& /*value*/) {
        return false;
    }

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
    OutcomeSelection<SearchNode>(THTS<SearchNode>* _thts) :
        thts(_thts) {}

    THTS<SearchNode>* thts;
};

template <class SearchNode>
class MCOutcomeSelection : public OutcomeSelection<SearchNode> {
public:
    MCOutcomeSelection(THTS<SearchNode>* _thts) :
        OutcomeSelection<SearchNode>(_thts) {}

    virtual SearchNode* selectOutcome(SearchNode* node,
                                      PDState& nextState,
                                      int const& varIndex,
                                      int const& lastProbVarIndex);
};

template <class SearchNode>
class UnsolvedMCOutcomeSelection : public OutcomeSelection<SearchNode> {
public:
    UnsolvedMCOutcomeSelection(THTS<SearchNode>* _thts) :
        OutcomeSelection<SearchNode>(_thts) {}

    virtual SearchNode* selectOutcome(SearchNode* node,
                                      PDState& nextState,
                                      int const& varIndex,
                                      int const& lastProbVarIndex);
};

#endif
