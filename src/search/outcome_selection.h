#ifndef OUTCOME_SELECTION_H
#define OUTCOME_SELECTION_H

#include "states.h"

class THTS;
class SearchNode;

class OutcomeSelection {
public:
    virtual ~OutcomeSelection() {}

    // Create an outcome selection component
    static OutcomeSelection* fromString(std::string& desc, THTS* thts);

    // Set parameters from command line
    virtual bool setValueFromString(std::string& /*param*/,
                                    std::string& /*value*/) {
        return false;
    }

    // Learn parameter values from a random training set
    virtual void learn() {}

    // This is called when caching is disabled because memory becomes sparse
    virtual void disableCaching() {}

    virtual void initRound() {}
    virtual void initTrial() {}

    // Outcome selection
    virtual SearchNode* selectOutcome(SearchNode* node, PDState& nextState,
                                      int varIndex, int lastProbVarIndex) = 0;

    // Prints statistics
    virtual void printStats(std::ostream& /*out*/, std::string /*indent*/) {}

protected:
    OutcomeSelection(THTS* _thts) : thts(_thts) {}

    THTS* thts;
};

class MCOutcomeSelection : public OutcomeSelection {
public:
    MCOutcomeSelection(THTS* _thts) : OutcomeSelection(_thts) {}

    SearchNode* selectOutcome(SearchNode* node, PDState& nextState,
                              int varIndex, int lastProbVarIndex) override;

    // A blacklist indicates which values are ignored for outcome selection.
    // Basic MC Sampling does not ignore any values.
    virtual std::vector<int> computeBlacklist(SearchNode* /*node*/,
                                              PDState const& /*nextState*/,
                                              int /*varIndex*/) const {
        return std::vector<int>{};
    }
};

class UnsolvedMCOutcomeSelection : public MCOutcomeSelection {
public:
    UnsolvedMCOutcomeSelection(THTS* _thts) : MCOutcomeSelection(_thts) {}

    // Unsolved outcome selection ignores values which are already solved.
    std::vector<int> computeBlacklist(SearchNode* node,
                                      PDState const& nextState,
                                      int varIndex) const override;
};

#endif
