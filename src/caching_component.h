#ifndef CACHING_COMPONENT_H
#define CACHING_COMPONENT_H

class ProstPlanner;

class CachingComponent {
public:
    CachingComponent(ProstPlanner* planner);
    virtual ~CachingComponent();

    virtual void disableCaching() = 0;

private:
    //CachingComponent() {}

    ProstPlanner* cachingComponentAdmin;
};

#endif
