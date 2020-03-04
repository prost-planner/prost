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

    // This is called when caching is disabled because memory becomes sparse
    virtual void disableCaching() {}

    virtual void initSession() {}
    virtual void initRound() {}
    virtual void finishRound() {}
    virtual void initStep() {}
    virtual void finishStep() {}
    virtual void initTrial() {}

    // Outcome selection
    virtual SearchNode* selectOutcome(SearchNode* node, PDState& nextState,
                                      int varIndex, int lastProbVarIndex) = 0;

    // Prints statistics
    virtual void printConfig(std::string indent) const;
    virtual void printStepStatistics(std::string /*indent*/) const {}
    virtual void printRoundStatistics(std::string /*indent*/) const {}

protected:
    OutcomeSelection(THTS* _thts, std::string _name)
        : thts(_thts),
          name(_name) {}

    THTS* thts;

    // Name, used for output only
    std::string name;
};

class MCOutcomeSelection : public OutcomeSelection {
public:
    MCOutcomeSelection(THTS* _thts)
        : OutcomeSelection(_thts, "MonteCarlo outcome selection") {}

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
    UnsolvedMCOutcomeSelection(THTS* _thts)
        : MCOutcomeSelection(_thts) {
        name = "UnsolvedMonteCarlo outcome selection";
    }

    // Unsolved outcome selection ignores values which are already solved.
    std::vector<int> computeBlacklist(SearchNode* node,
                                      PDState const& nextState,
                                      int varIndex) const override;
};

#endif
