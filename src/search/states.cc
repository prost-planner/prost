#include "states.h"

#include "logical_expressions.h"

#include "utils/string_utils.h"

#include <sstream>

using namespace std;

ActionState::ActionState(int _index, string stateDesc,  vector<ActionFluent*> const& actionFluents, string precondDesc, vector<Evaluatable*> const& _actionPreconditions) :
    index(_index) {
    vector<string> actionFluentVals;
    StringUtils::split(stateDesc, actionFluentVals, " ");
    for(unsigned int i = 0; i < actionFluentVals.size(); ++i) {
        int val = atoi(actionFluentVals[i].c_str());
        state.push_back(val);
        if(val == 1) {
            scheduledActionFluents.push_back(actionFluents[i]);
        }
    }
    assert(state.size() == actionFluents.size());

    vector<string> preconds;
    StringUtils::split(precondDesc, preconds, " ");
    for(unsigned int i = 0; i < preconds.size(); ++i) {
        int val = atoi(preconds[i].c_str());
        assert((val >= 0) && (val < _actionPreconditions.size()));
        actionPreconditions.push_back(_actionPreconditions[val]);
    }
}

string ActionState::name() const {
    if(scheduledActionFluents.empty()) {
        return "noop";
    }
    stringstream ss;
    for(unsigned int i = 0; i < scheduledActionFluents.size(); ++i) {
        ss << scheduledActionFluents[i]->name;
        if(i != scheduledActionFluents.size() -1) {
            ss << ", ";
        }
    }
    return ss.str();
}
