#ifndef LEARNING_COMPONENT_H
#define LEARNING_COMPONENT_H

class ProstPlanner;
class State;

#include <vector>

class LearningComponent {
public:
    LearningComponent(ProstPlanner* planner);
    LearningComponent(LearningComponent const& other);
    virtual ~LearningComponent();

    virtual bool learn(std::vector<State> const& trainingSet);

    bool learningFinished() const {
        return hasLearned;
    }

private:
    LearningComponent() {}

    ProstPlanner* learningComponentAdmin;
    bool hasLearned;
};

#endif
