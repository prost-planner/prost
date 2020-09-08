#include "../../doctest/doctest.h"

#include "../evaluatables.h"
#include "../logical_expressions.h"
#include "../rddl.h"

namespace prost::parser {
TEST_CASE("Generation of boolen and FDR action variables") {
    auto task = new RDDLTask();
    Type* varType = task->addType("dummy");
    task->addObject("dummy", "0");
    task->addObject("dummy", "1");
    task->addObject("dummy", "2");
    task->addObject("dummy", "3");
    auto a0 = new ActionFluent("a0", varType, 0);
    auto a1 = new ActionFluent("a1", task->getType("bool"), 1);
    task->actionFluents = {a0, a1};

    CHECK(a0->isFDR);
    CHECK(!a1->isFDR);
    CHECK(a0->valueType->objects.size() == 4);
    CHECK(a1->valueType->objects.size() == 2);
}
} // namespace prost::parser
