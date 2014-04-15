#include "states.h"

#include "search_engine.h"
#include "evaluatables.h"

#include "utils/string_utils.h"

#include <sstream>

using namespace std;

void State::printCompact(ostream& out) const {
    for(unsigned int index = 0; index < State::stateSize; ++index) {
        out << state[index] << " ";
    }
    out << endl;
}

void State::print(ostream& out) const {
    for(unsigned int index = 0; index < State::stateSize; ++index) {
        out << SearchEngine::probCPFs[index]->name << ": ";
        //if(CPFs[index]->head->parent->valueType->type == Type::OBJECT) {
        //    out << CPFs[index]->head->parent->valueType->domain[state[index]]->name << endl;
        //} else {
        out << state[index] << endl;
        //}
    }
    out << "Remaining Steps: " << remSteps << endl;
    out << "StateHashKey: " << hashKey << endl;
}

void PDState::print(ostream& out) const {
    for(unsigned int index = 0; index < State::stateSize; ++index) {
        out << SearchEngine::probCPFs[index]->name << ": ";
        state[index].print(out);
    }
    out << "Remaining Steps: " << remSteps << endl;
}

void KleeneState::print(ostream& out) const {
    for(unsigned int index = 0; index < KleeneState::stateSize; ++index) {
        out << SearchEngine::probCPFs[index]->name << ": { ";
        for(set<double>::iterator it = state[index].begin(); it != state[index].end(); ++it) {
            cout << *it << " ";
        }
        cout << "}" << endl;
    }
}

void ActionState::printCompact(ostream& out) const {
    if(scheduledActionFluents.empty()) {
        out << "noop() ";
    } else {
        for(unsigned int index = 0; index < scheduledActionFluents.size(); ++index) {
            out << scheduledActionFluents[index]->name << " ";
        }
    }
}

void ActionState::print(ostream& out) const {
    printCompact(out);
    out << ": " << endl;
    out << "Index : " << index << endl;
    out << "Relevant preconditions: " << endl;
    for(unsigned int i = 0; i < actionPreconditions.size(); ++i) {
        out << actionPreconditions[i]->name << endl;
    }
}
