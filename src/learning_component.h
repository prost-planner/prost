#ifndef LEARNING_COMPONENT_H
#define LEARNING_COMPONENT_H

#include "state.h"

class ProstPlanner;

class LearningComponent {
public:
    LearningComponent(ProstPlanner* planner);
    virtual ~LearningComponent();

    virtual bool learn(std::vector<State> const& /*trainingSet*/) {
        hasLearned = true;
        return true;
    }

    bool learningFinished() {
        return hasLearned;
    }

private:
    LearningComponent() {}

    ProstPlanner* learningComponentAdmin;
    bool hasLearned;
};

#endif
