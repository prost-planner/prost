#ifndef DETERMINIZER_H
#define DETERMINIZER_H

/*
  The purpose of the Determinizer is to compute the most likely determinization
  of all probabilistic conditional probability functions.
*/

namespace prost {
namespace parser {
struct RDDLTask;

class Determinizer {
public:
    Determinizer(RDDLTask* task)
        : task(task) {}

    void determinize();

private:
    RDDLTask* task;
};
} // namespace parser
} // namespace prost

#endif
