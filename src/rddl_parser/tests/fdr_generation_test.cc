#include "../../doctest/doctest.h"

#include "../fdr_generation.h"
#include "../logical_expressions.h"
#include "../mutex_detection.h"
#include "../rddl.h"

using namespace std;

namespace prost {
namespace parser {
namespace fdr {
namespace tests {
TEST_CASE("Greedy Partitioning and FDR generation") {
    auto task = new RDDLTask();
    auto a0 = new ActionFluent("a0", task->getType("bool"), 0);
    auto a1 = new ActionFluent("a1", task->getType("bool"), 1);
    auto a2 = new ActionFluent("a2", task->getType("bool"), 2);
    auto a3 = new ActionFluent("a3", task->getType("bool"), 3);
    auto a4 = new ActionFluent("a4", task->getType("bool"), 4);
    Simplifications dummy;
    FDRGenerator fdrGen(task);

    SUBCASE("FDR generation with empty partitioning") {
        TaskMutexInfo mutexInfo(task);
        auto emptyPartitioning = [](TaskMutexInfo const&) {
            return VarPartitioning();
        };
        vector<ActionFluent*> fdrVars =
            fdrGen.generateFDRVars(mutexInfo, dummy, emptyPartitioning);
        CHECK(fdrVars.empty());
    }

    // We test greedy partitioning and FDR generation at the same time, because
    // they have similar test cases
    SUBCASE("Greedy FDR generation when no pair of variables is mutex") {
        task->actionFluents = {a0, a1, a2};
        GreedyPartitioning partitionGen;
        TaskMutexInfo mutexInfo(task);
        VarPartitioning partitioning = partitionGen(mutexInfo);
        auto it = partitioning.begin();
        // When no variables are mutex a partition contains exactly one action
        // fluent
        for (size_t i = 0; i < task->actionFluents.size(); ++i) {
            VarPartition partition = *it++;
            CHECK(partition.size() == 1);
            CHECK(*partition.begin() == task->actionFluents[i]);
        }
        CHECK(it == partitioning.end());
        // When no variables are mutex the FDR variables correspond to the
        // original ones
        vector<ActionFluent*> fdrVars =
            fdrGen.generateFDRVars(mutexInfo, dummy, GreedyPartitioning());
        CHECK(fdrVars.size() == 3);
        CHECK(fdrVars[0] == a0);
        CHECK(fdrVars[1] == a1);
        CHECK(fdrVars[2] == a2);
    }

    SUBCASE("Greedy FDR generation when all variables are pairwise mutex") {
        task->actionFluents = {a0, a1, a2};
        TaskMutexInfo mutexInfo(task);
        mutexInfo.addMutexInfo(a0, a1);
        mutexInfo.addMutexInfo(a0, a2);
        mutexInfo.addMutexInfo(a1, a2);

        GreedyPartitioning partitionGen;
        VarPartitioning partitioning = partitionGen(mutexInfo);
        // When all variables are pairwise mutex there should exist a single
        // partition that contains all variables.
        VarPartition partition = *(partitioning.begin());
        CHECK(partition.size() == 3);
        auto it = partition.begin();
        for (ActionFluent const* af : task->actionFluents) {
            CHECK(*(it++) == af);
        }
        CHECK(std::next(partitioning.begin()) == partitioning.end());

        vector<ActionFluent*> fdrVars =
            fdrGen.generateFDRVars(mutexInfo, dummy, GreedyPartitioning());
        CHECK(fdrVars.size() == 1);
        ActionFluent const* var = fdrVars[0];
        CHECK(var->isFDR);
        // The assignment where all variables are 0 is valid, this is
        // represented by the additional domain value.
        CHECK(var->domainSize() == 4);
    }

    SUBCASE("Greedy FDR generation when some variable pairs are mutex") {
        // Of the five action variables, the first 3 are pairwise mutex and the
        // last two are. There are other mutex pairs, but not in a way that
        // these sets of pairwise mutex variables can be extended, so the result
        // are two FDR variables.
        task->actionFluents = {a0, a1, a2, a3, a4};
        TaskMutexInfo mutexInfo(task);
        mutexInfo.addMutexInfo(a0, a1);
        mutexInfo.addMutexInfo(a0, a2);
        mutexInfo.addMutexInfo(a1, a2);
        mutexInfo.addMutexInfo(a1, a3);
        mutexInfo.addMutexInfo(a1, a4);
        mutexInfo.addMutexInfo(a3, a4);

        vector<ActionFluent*> fdrVars =
            fdrGen.generateFDRVars(mutexInfo, dummy, GreedyPartitioning());
        CHECK(fdrVars.size() == 2);
        CHECK(fdrVars[0]->isFDR);
        CHECK(fdrVars[0]->domainSize() == 4);
        CHECK(fdrVars[1]->isFDR);
        CHECK(fdrVars[1]->domainSize() == 3);
    }
}
} // namespace tests
} // namespace fdr
} // namespace parser
} // namespace prost
