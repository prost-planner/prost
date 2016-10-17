#include "outcome_selection.h"

#include "thts.h"

#include "utils/string_utils.h"
#include "utils/system_utils.h"

#include <vector>

using std::vector;

/******************************************************************
                  Outcome Selection Creation
******************************************************************/

OutcomeSelection* OutcomeSelection::fromString(std::string& desc, THTS* thts) {
    StringUtils::trim(desc);
    assert(desc[0] == '[' && desc[desc.size() - 1] == ']');
    StringUtils::removeFirstAndLastCharacter(desc);
    StringUtils::trim(desc);

    OutcomeSelection* result = nullptr;

    if (desc.find("MC") == 0) {
        desc = desc.substr(2, desc.size());

        result = new MCOutcomeSelection(thts);
    } else if (desc.find("UMC") == 0) {
        desc = desc.substr(3, desc.size());

        result = new UnsolvedMCOutcomeSelection(thts);
    } else {
        SystemUtils::abort("Unknown Outcome Selection: " + desc);
    }

    assert(result);
    StringUtils::trim(desc);

    while (!desc.empty()) {
        std::string param;
        std::string value;
        StringUtils::nextParamValuePair(desc, param, value);

        if (!result->setValueFromString(param, value)) {
            SystemUtils::abort("Unused parameter value pair: " + param + " / " +
                               value);
        }
    }

    return result;
}

/******************************************************************
                        MC Outcome Selection
******************************************************************/

SearchNode* MCOutcomeSelection::selectOutcome(SearchNode* node,
                                              PDState& nextState, int varIndex,
                                              int lastProbVarIndex) {
    if (node->children.empty()) {
        node->children.resize(
            SearchEngine::probabilisticCPFs[varIndex]->getDomainSize(),
            nullptr);
    }
    vector<int> blacklist = computeBlacklist(node, nextState, varIndex);

    std::pair<double, double> sample = nextState.sample(varIndex, blacklist);
    int childIndex = static_cast<int>(sample.first);
    assert((childIndex >= 0) && childIndex < node->children.size());

    if (!node->children[childIndex]) {
        if (varIndex == lastProbVarIndex) {
            node->children[childIndex] =
                thts->createDecisionNode(sample.second);
        } else {
            node->children[childIndex] = thts->createChanceNode(sample.second);
        }
    }

    return node->children[childIndex];
}

/******************************************************************
             MC Outcome Selection with Solve Labeling
******************************************************************/

vector<int> UnsolvedMCOutcomeSelection::computeBlacklist(
    SearchNode* node, PDState const& nextState, int varIndex) const {
    vector<int> blacklist;
    // Determines the indices of all solved outcomes
    DiscretePD const& pd = nextState.probabilisticStateFluentAsPD(varIndex);
    for (size_t i = 0; i < pd.size(); ++i) {
        int childIndex = pd.values[i];
        if (node->children[childIndex] && node->children[childIndex]->solved) {
            blacklist.push_back(i);
        }
    }
    return blacklist;
}
