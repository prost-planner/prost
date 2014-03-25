#ifndef CACHING_COMPONENT_H
#define CACHING_COMPONENT_H

class ProstPlanner;

class CachingComponent {
public:
    CachingComponent(ProstPlanner* planner);
    CachingComponent(CachingComponent const& other);
    virtual ~CachingComponent();

    virtual void disableCaching() = 0;

private:
    ProstPlanner* cachingComponentAdmin;
};

#endif
