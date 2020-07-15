#ifndef DETERMINIZER_H
#define DETERMINIZER_H

struct RDDLTask;

class Determinizer {
public:
    Determinizer(RDDLTask* task)
        : task(task) {}

    void determinize();

private:
    RDDLTask* task;
};

#endif
