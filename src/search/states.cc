#include "states.h"

#include "evaluatables.h"
#include "search_engine.h"

#include "utils/string_utils.h"

#include <sstream>

using namespace std;

string State::toCompactString() const {
    stringstream ss;
    for (double val : deterministicStateFluents) {
        ss << val << " ";
    }
    ss << "| ";
    for (double val : probabilisticStateFluents) {
        ss << val << " ";
    }
    return ss.str();
}

string State::toString() const {
    stringstream ss;
    for (size_t i = 0; i < State::numberOfDeterministicStateFluents; ++i) {
        ss << SearchEngine::deterministicCPFs[i]->name << ": "
           << deterministicStateFluents[i] << endl;
    }
    ss << endl;
    for (size_t i = 0; i < State::numberOfProbabilisticStateFluents; ++i) {
        ss << SearchEngine::probabilisticCPFs[i]->name << ": "
           << probabilisticStateFluents[i] << endl;
    }
    ss << "Remaining Steps: " << remSteps << endl
       << "StateHashKey: " << hashKey << endl;
    return ss.str();
}

string PDState::toCompactString() const {
    stringstream ss;
    for (double val : deterministicStateFluents) {
        ss << val << " ";
    }
    for (DiscretePD const& pd : probabilisticStateFluentsAsPD) {
        ss << pd.toString();
    }
    return ss.str();
}


string KleeneState::toString() const {
    stringstream ss;
    for (unsigned int index = 0; index < KleeneState::stateSize; ++index) {
        ss << SearchEngine::allCPFs[index]->name << ": { ";
        for (double val : state[index]) {
            ss << val << " ";
        }
        ss << "}" << endl;
    }
    return ss.str();
}

vector<string> ActionState::getScheduledActionFluents() const {
    vector<string> scheduledActionFluents;
    for (size_t i = 0; i < state.size(); ++i) {
        int valueIndex = state[i];
        if (SearchEngine::actionFluents[i]->values.size() == 2) {
            if (valueIndex) {
                scheduledActionFluents.push_back(
                    SearchEngine::actionFluents[i]->name);
            }
        } else {
            if (SearchEngine::actionFluents[i]->values[valueIndex] != "none-of-those") {
                scheduledActionFluents.push_back(
                    SearchEngine::actionFluents[i]->values[valueIndex]);
            }
        }
    }
    return scheduledActionFluents;
}

string ActionState::toCompactString() const {
    vector<string> scheduledActionFluents = getScheduledActionFluents();
    if (scheduledActionFluents.empty()) {
        return "noop()";
    } else {
        stringstream ss;
        for (string const& scheduledActionFluent : scheduledActionFluents) {
            ss << scheduledActionFluent << " ";
        }
        return ss.str();
    }
}

string ActionState::toString() const {
    stringstream ss;
    ss << toCompactString() << ": " << endl
       << "Index : " << index << endl
       << "Relevant preconditions: " << endl;
    for (DeterministicEvaluatable const* precond : actionPreconditions) {
        ss << precond->name << endl;
    }
    return ss.str();
}
