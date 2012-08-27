#include "caching_component.h"

#include "prost_planner.h"

CachingComponent::CachingComponent(ProstPlanner* planner) :
    cachingComponentAdmin(planner) {
    cachingComponentAdmin->registerCachingComponent(this);
}

CachingComponent::~CachingComponent() {
    cachingComponentAdmin->unregisterCachingComponent(this);
}
