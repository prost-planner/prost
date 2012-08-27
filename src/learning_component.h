#ifndef LEARNING_COMPONENT_H
#define LEARNING_COMPONENT_H

#include "state.h"

class ProstPlanner;

class LearningComponent {
public:
    LearningComponent(ProstPlanner* planner);
    virtual ~LearningComponent();

    virtual void learn(std::vector<State> const& trainingSet) = 0;

private:
    LearningComponent() {}

    ProstPlanner* learningComponentAdmin;
};

#endif
