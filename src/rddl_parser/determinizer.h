#ifndef DETERMINIZER_H
#define DETERMINIZER_H

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
