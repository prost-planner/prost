#ifndef DETERMINIZER_H
#define DETERMINIZER_H

class RDDLTask;

class Determinizer {
public:
    Determinizer(RDDLTask* task)
        : task(task) {}

    void determinize();

private:
    RDDLTask* task;
};

#endif
