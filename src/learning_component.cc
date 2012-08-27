#include "learning_component.h"

#include "prost_planner.h"

LearningComponent::LearningComponent(ProstPlanner* planner) :
    learningComponentAdmin(planner) {
    learningComponentAdmin->registerLearningComponent(this);
}

LearningComponent::~LearningComponent() {
    learningComponentAdmin->unregisterLearningComponent(this);
}
