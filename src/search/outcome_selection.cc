#include "outcome_selection.h"

#include "thts.h"

#include "utils/system_utils.h"
#include "utils/string_utils.h"

#include <vector>

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
            SystemUtils::abort(
                    "Unused parameter value pair: " + param + " / " + value);
        }
    }

    return result;
}

/******************************************************************
                        MC Outcome Selection
******************************************************************/

SearchNode* MCOutcomeSelection::selectOutcome(SearchNode* node,
                                              PDState& nextState,
                                              int const& varIndex,
                                              int const& lastProbVarIndex) {

    if (node->children.empty()) {
        node->children.resize(
            SearchEngine::probabilisticCPFs[varIndex]->getDomainSize(), nullptr);
    }

    int childIndex = static_cast<int>(nextState.sample(varIndex));

    if (!node->children[childIndex]) {
        if (varIndex == lastProbVarIndex) {
            node->children[childIndex] = thts->createDecisionNode(1.0);
        } else {
            node->children[childIndex] = thts->createChanceNode(1.0);
        }
    }

    return node->children[childIndex];
}

/******************************************************************
             MC Outcome Selection with Solve Labeling
******************************************************************/

SearchNode* UnsolvedMCOutcomeSelection::selectOutcome(
    SearchNode* node, PDState& nextState, int const& varIndex,
    int const& lastProbVarIndex) {

    DiscretePD& pd = nextState.probabilisticStateFluentAsPD(varIndex);
    std::vector<int> solvedIndices;

    if (node->children.empty()) {
        node->children.resize(
            SearchEngine::probabilisticCPFs[varIndex]->getDomainSize(), nullptr);
    } else {
        // Determine the indices of solved outcomes
        for (size_t i = 0; i < pd.size(); ++i) {
            int childIndex = pd.values[i];
            if (node->children[childIndex] && node->children[childIndex]->solved) {
                solvedIndices.push_back(i);
            }
        }
    }

    std::pair<double, double> sample = pd.sample(solvedIndices);
    int childIndex = static_cast<int>(sample.first);
    double childProb = sample.second;
    // cout << "Chosen child is " << childIndex << " and prob is " << childProb << endl;

    assert((childIndex >= 0) && childIndex < node->children.size());

    if (!node->children[childIndex]) {
        if (varIndex == lastProbVarIndex) {
            node->children[childIndex] = thts->createDecisionNode(childProb);
        } else {
            node->children[childIndex] = thts->createChanceNode(childProb);
        }
    }

    assert(!node->children[childIndex]->solved);

    nextState.probabilisticStateFluent(varIndex) = childIndex;
    return node->children[childIndex];
}

