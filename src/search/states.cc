#include "states.h"

#include "evaluatables.h"
#include "search_engine.h"

#include "utils/string_utils.h"

#include <sstream>

using namespace std;

void State::printCompact(ostream& out) const {
    for (unsigned int index = 0;
         index < State::numberOfDeterministicStateFluents; ++index) {
        out << deterministicStateFluents[index] << " ";
    }
    out << "| ";
    for (unsigned int index = 0;
         index < State::numberOfProbabilisticStateFluents; ++index) {
        out << probabilisticStateFluents[index] << " ";
    }
    out << endl;
}

void State::print(ostream& out) const {
    for (unsigned int index = 0;
         index < State::numberOfDeterministicStateFluents; ++index) {
        out << SearchEngine::deterministicCPFs[index]->name << ": ";
        // if(CPFs[index]->head->parent->valueType->type == Type::OBJECT) {
        //    out <<
        //    CPFs[index]->head->parent->valueType->domain[state[index]]->name
        //    << endl;
        //} else {
        out << deterministicStateFluents[index] << endl;
        //}
    }
    out << endl;

    for (unsigned int index = 0;
         index < State::numberOfProbabilisticStateFluents; ++index) {
        out << SearchEngine::probabilisticCPFs[index]->name << ": ";
        // if(CPFs[index]->head->parent->valueType->type == Type::OBJECT) {
        //    out <<
        //    CPFs[index]->head->parent->valueType->domain[state[index]]->name
        //    << endl;
        //} else {
        out << probabilisticStateFluents[index] << endl;
        //}
    }

    out << "Remaining Steps: " << remSteps << endl;
    out << "StateHashKey: " << hashKey << endl;
}

void PDState::printPDState(ostream& out) const {
    for (unsigned int index = 0;
         index < State::numberOfDeterministicStateFluents; ++index) {
        out << SearchEngine::deterministicCPFs[index]->name << ": "
            << deterministicStateFluents[index] << endl;
    }

    for (unsigned int index = 0;
         index < State::numberOfProbabilisticStateFluents; ++index) {
        out << SearchEngine::probabilisticCPFs[index]->name << ": ";
        probabilisticStateFluentsAsPD[index].print(out);
    }
    out << "Remaining Steps: " << remSteps << endl;
}

void PDState::printPDStateCompact(ostream& out) const {
    for (unsigned int index = 0;
         index < State::numberOfDeterministicStateFluents; ++index) {
        out << deterministicStateFluents[index] << " ";
    }

    for (unsigned int index = 0;
         index < State::numberOfProbabilisticStateFluents; ++index) {
        out << "[ ";
        for (unsigned int i = 0;
             i < probabilisticStateFluentsAsPD[index].values.size(); ++i) {
            out << probabilisticStateFluentsAsPD[index].values[i] << ":"
                << probabilisticStateFluentsAsPD[index].probabilities[i] << " ";
        }
        out << "] ";
    }
}

void KleeneState::print(ostream& out) const {
    for (unsigned int index = 0; index < KleeneState::stateSize; ++index) {
        out << SearchEngine::allCPFs[index]->name << ": { ";
        for (set<double>::iterator it = state[index].begin();
             it != state[index].end(); ++it) {
            cout << *it << " ";
        }
        cout << "}" << endl;
    }
}

void ActionState::printCompact(ostream& out) const {
    if (scheduledActionFluents.empty()) {
        out << "noop() ";
    } else {
        for (unsigned int index = 0; index < scheduledActionFluents.size();
             ++index) {
            out << scheduledActionFluents[index]->name << " ";
        }
    }
}

void ActionState::print(ostream& out) const {
    printCompact(out);
    out << ": " << endl;
    out << "Index : " << index << endl;
    out << "Relevant preconditions: " << endl;
    for (unsigned int i = 0; i < actionPreconditions.size(); ++i) {
        out << actionPreconditions[i]->name << endl;
    }
}
