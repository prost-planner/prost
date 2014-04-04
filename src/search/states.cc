#include "states.h"

#include "logical_expressions.h"

#include "utils/string_utils.h"

#include <sstream>

using namespace std;

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
