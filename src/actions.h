#ifndef ACTIONS_H
#define ACTIONS_H

#include <string>
#include <vector>
#include <cassert>

#include "state.h"

class ActionFluent;
class Evaluatable;

class ActionState {
public:
    ActionState(int size) :
        state(size,0), index(-1) {}

    ActionState(ActionState const& other) :
        state(other.state),
        scheduledActionFluents(other.scheduledActionFluents),
        relevantSACs(other.relevantSACs),
        index(other.index) {}

    int& operator[](int const& index) {
        return state[index];
    }

    const int& operator[](int const& index) const {
        return state[index];
    }

    void getActions(std::vector<std::string>& result) const;
    void calculateProperties(std::vector<ActionFluent*> const& actionFluents, std::vector<Evaluatable*> const& sacs);
    bool isApplicable(State const& current) const; 

    std::vector<int> state;
    std::vector<ActionFluent*> scheduledActionFluents;
    std::vector<Evaluatable*> relevantSACs;
    int index;

private:
    ActionState() {}

    bool containsNegativeActionFluent(Evaluatable* sac) const;
    bool containsAdditionalPositiveActionFluent(Evaluatable* sac) const;
};

inline bool operator<(ActionState const& lhs, ActionState const& rhs) {
    assert(lhs.state.size() == rhs.state.size());
    if(lhs.index >= 0) {
        assert(rhs.index >= 0);
        return (lhs.index < rhs.index);
    }

    //This is only used in preprocessor to order the actions (later, all action states have an index)

    int lhs_num = 0;
    int rhs_num = 0;
    for(unsigned int i = 0; i < lhs.state.size(); ++i) {
        lhs_num += lhs[i];
        rhs_num += rhs[i];
    }

    if(lhs_num < rhs_num) {
        return true;
    } else if(rhs_num < lhs_num) {
        return false;
    }

    return (lhs.state < rhs.state);
}

#endif
