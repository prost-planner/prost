#include "learning_component.h"

#include "prost_planner.h"

LearningComponent::LearningComponent(ProstPlanner* planner) :
    learningComponentAdmin(planner),
    hasLearned(false) {
    learningComponentAdmin->registerLearningComponent(this);
}

LearningComponent::LearningComponent(LearningComponent const& other) :
    learningComponentAdmin(other.learningComponentAdmin),
    hasLearned(false) {
    learningComponentAdmin->registerLearningComponent(this);
}

LearningComponent::~LearningComponent() {
    learningComponentAdmin->unregisterLearningComponent(this);
}
