#include "../../doctest/doctest.h"

#include "../evaluatables.h"
#include "../logical_expressions.h"
#include "../rddl.h"
#include "../hashing/hash_keys.h"

using namespace std;

namespace prost {
namespace parser {
namespace hashing {
TEST_CASE("Hash keys with binary variables") {
    auto task = new RDDLTask();
    vector<Parameter*> params;
    string typeName = "fdr";
    Type* t = task->addType(typeName);
    task->addObject(typeName, "o0");
    task->addObject(typeName, "o1");
    task->addObject(typeName, "o2");
    auto fdrPVar = new ParametrizedVariable(
        "fdr", params, ParametrizedVariable::STATE_FLUENT, t, 0.0);
    task->addVariableSchematic(fdrPVar);
    auto pVar = new ParametrizedVariable(
        "pv", params, ParametrizedVariable::STATE_FLUENT,
        task->getType("bool"), 0.0);
    task->addVariableSchematic(pVar);

    auto s0 = new StateFluent(*pVar, params, 0.0, 0);
    auto s1 = new StateFluent(*pVar, params, 0.0, 1);
    auto s2 = new StateFluent(*pVar, params, 0.0, 2);
    auto s3 = new StateFluent(*fdrPVar, params, 0.0, 3);
    auto s4 = new StateFluent(*pVar, params, 0.0, 4);
    auto s5 = new StateFluent(*fdrPVar, params, 0.0, 5);

    auto c0 = new ConditionalProbabilityFunction(s0, nullptr);
    c0->setDomain(1);
    auto c1 = new ConditionalProbabilityFunction(s1, nullptr);
    c1->setDomain(1);
    auto c2 = new ConditionalProbabilityFunction(s2, nullptr);
    c2->setDomain(1);
    auto c3 = new ConditionalProbabilityFunction(s3, nullptr);
    c3->setDomain(2);
    auto c4 = new ConditionalProbabilityFunction(s4, nullptr);
    c4->setDomain(1);
    auto c5 = new ConditionalProbabilityFunction(s5, nullptr);
    c5->setDomain(2);

    task->rewardCPF = new RewardFunction(nullptr);

    auto a0 = new ActionFluent("a0", task->getType("bool"), 0);
    auto a1 = new ActionFluent("a1", task->getType("bool"), 1);
    auto a2 = new ActionFluent("a2", task->getType("bool"), 2);

    ActionState a000({0,0,0});
    a000.index = 0;
    ActionState a001({0,0,1});
    a001.index = 1;
    ActionState a010({0,1,0});
    a010.index = 2;
    ActionState a011({0,1,1});
    a011.index = 3;
    ActionState a100({1,0,0});
    a100.index = 4;
    ActionState a101({1,0,1});
    a101.index = 5;
    ActionState a110({1,1,0});
    a110.index = 6;
    ActionState a111({1,1,1});
    a111.index = 7;
    SUBCASE("State and Kleene state hashing are possible") {
        task->stateFluents = {s0, s1, s2, s3, s4, s5};
        task->CPFs = {c0, c1, c2, c3, c4, c5};
        HashKeyGenerator h(task);
        h.generateHashKeys(false);

        CHECK(!task->stateHashKeys.empty());
        CHECK(!task->kleeneStateHashKeyBases.empty());
        for (int i = 0; i < task->stateFluents.size(); ++i) {
            CHECK(task->stateHashKeys[i][0] == 0);
        }
        // Hash key base for s0 is 1
        CHECK(task->stateHashKeys[0][1] == 1);
        // Hash key base for s1 is |dom(s0)| = 2
        CHECK(task->stateHashKeys[1][1] == 2);
        // Hash key base for s2 is |dom(s0)| * |dom(s1)| = 4
        CHECK(task->stateHashKeys[2][1] == 4);
        // Hash key base for s3 is |dom(s0)| * ... * |dom(s2)| = 8
        CHECK(task->stateHashKeys[3][1] == 8);
        CHECK(task->stateHashKeys[3][2] == 16);
        // Hash key base for s4 is |dom(s1)| * ... * |dom(s3)| = 24
        CHECK(task->stateHashKeys[4][1] == 24);
        // Hash key base for s5 is |dom(s1)| * ... * |dom(s4)| = 48
        CHECK(task->stateHashKeys[5][1] == 48);
        CHECK(task->stateHashKeys[5][2] == 96);

        // The size of the Kleene domains ("kdom" below) is 3 for the binary
        // state variables s0, s1, s2 and s4, and 7 for s3 and s5
        CHECK(c0->kleeneDomainSize == 3);
        CHECK(c1->kleeneDomainSize == 3);
        CHECK(c2->kleeneDomainSize == 3);
        CHECK(c3->kleeneDomainSize == 7);
        CHECK(c4->kleeneDomainSize == 3);
        CHECK(c5->kleeneDomainSize == 7);

        // Kleene hash key base for s0 is 1
        CHECK(task->kleeneStateHashKeyBases[0] == 1);
        // Kleene hash key base for s1 is |kdom(s0)| = 3
        CHECK(task->kleeneStateHashKeyBases[1] == 3);
        // Kleene hash key base for s2 is |kdom(s0)| * |kdom(s1)| = 9
        CHECK(task->kleeneStateHashKeyBases[2] == 9);
        // Kleene hash key base for s3 is |kdom(s0)| * ... * |kdom(s2)| = 27
        CHECK(task->kleeneStateHashKeyBases[3] == 27);
        // Kleene hash key base for s4 is |kdom(s0)| * ... * |kdom(s3)| = 189
        CHECK(task->kleeneStateHashKeyBases[4] == 189);
        // Kleene hash key base for s5 is |kdom(s0)| * ... * |kdom(s4)| = 567
        CHECK(task->kleeneStateHashKeyBases[5] == 567);
    }

    SUBCASE("State and Kleene state hashing are impossible") {
        int numVars = (sizeof(long) * 8) - 1;
        for (int i = 0; i < numVars; ++i) {
            auto var = new StateFluent(*pVar, params, 0.0, i);
            auto cpf = new ConditionalProbabilityFunction(var, nullptr);
            cpf->setDomain(1);
            task->stateFluents.push_back(var);
            task->CPFs.push_back(cpf);
        }
        HashKeyGenerator h(task);
        h.generateHashKeys(false);
        CHECK(task->stateHashKeys.empty());
        CHECK(task->kleeneStateHashKeyBases.empty());
    }

    SUBCASE("Equivalence classes of actions") {
        task->actionFluents = {a0, a1, a2};
        task->actionStates = {a000, a001, a010, a011, a100, a101, a110, a111};
        task->stateFluents = {s0, s1, s2};
        c0->dependentActionFluents = {a0};
        c1->dependentActionFluents = {a0, a1};
        c2->dependentActionFluents = {a0, a1, a2};
        task->CPFs = {c0, c1, c2};

        HashKeyGenerator h(task);
        h.generateHashKeys(false);
        // c0 only depends on a0, so the 2 equivalence classes are
        // {a000, a001, a010, a011} and {a100, a101, a110, a111}
        CHECK(c0->actionHashKeyMap[0] == 0);
        CHECK(c0->actionHashKeyMap[1] == 0);
        CHECK(c0->actionHashKeyMap[2] == 0);
        CHECK(c0->actionHashKeyMap[3] == 0);
        CHECK(c0->actionHashKeyMap[4] == 1);
        CHECK(c0->actionHashKeyMap[5] == 1);
        CHECK(c0->actionHashKeyMap[6] == 1);
        CHECK(c0->actionHashKeyMap[7] == 1);

        // c1 depends on a0 and a1, so the 4 equivalence classes are
        // {a000, a001}, {a010, a011}, {a100, a101} and {a110, a111}
        CHECK(c1->actionHashKeyMap[0] == 0);
        CHECK(c1->actionHashKeyMap[1] == 0);
        CHECK(c1->actionHashKeyMap[2] == 1);
        CHECK(c1->actionHashKeyMap[3] == 1);
        CHECK(c1->actionHashKeyMap[4] == 2);
        CHECK(c1->actionHashKeyMap[5] == 2);
        CHECK(c1->actionHashKeyMap[6] == 3);
        CHECK(c1->actionHashKeyMap[7] == 3);

        // c2 depends on all action fluents, so every action is in its own
        // equivalence class
        CHECK(c2->actionHashKeyMap[0] == 0);
        CHECK(c2->actionHashKeyMap[1] == 1);
        CHECK(c2->actionHashKeyMap[2] == 2);
        CHECK(c2->actionHashKeyMap[3] == 3);
        CHECK(c2->actionHashKeyMap[4] == 4);
        CHECK(c2->actionHashKeyMap[5] == 5);
        CHECK(c2->actionHashKeyMap[6] == 6);
        CHECK(c2->actionHashKeyMap[7] == 7);
    }

    SUBCASE("State fluent hash keys") {
        task->actionFluents = {a0, a1, a2};
        task->actionStates = {a000, a001, a010, a011, a100, a101, a110, a111};
        task->stateFluents = {s0, s1, s2};
        c0->dependentStateFluents = {s0};
        c1->dependentStateFluents = {s0, s1};
        c2->dependentStateFluents = {s0, s1, s2};
        c2->dependentActionFluents = {a0, a1, a2};
        task->CPFs = {c0, c1, c2};

        HashKeyGenerator h(task);
        h.generateHashKeys(false);

        // for every j, entry task->indexToStateFluentHashKeyMap[i][j] means
        // that variable s_i affects variable
        // task->indexToStateFluentHashKeyMap[i][j].first and that the hash key
        // base is task->indexToStateFluentHashKeyMap[i][j].second

        // s0 affects s0 and the hash key base is 1 (no actions affect s0)
        CHECK(task->indexToStateFluentHashKeyMap[0][0].first == 0);
        CHECK(task->indexToStateFluentHashKeyMap[0][0].second == 1);
        // s0 affects s1 and the hash key base is 1 (no actions affect s0)
        CHECK(task->indexToStateFluentHashKeyMap[0][1].first == 1);
        CHECK(task->indexToStateFluentHashKeyMap[0][1].second == 1);
        // s0 affects s2 and the hash key base is 8 (all actions affect s0)
        CHECK(task->indexToStateFluentHashKeyMap[0][2].first == 2);
        CHECK(task->indexToStateFluentHashKeyMap[0][2].second == 8);
        // s1 affects s1 and hash key base is 2 (s0 also affects s1)
        CHECK(task->indexToStateFluentHashKeyMap[1][0].first == 1);
        CHECK(task->indexToStateFluentHashKeyMap[1][0].second == 2);
        // s1 affects s2 and hash key base is 16 (all actions and s0 also
        // affect s2)
        CHECK(task->indexToStateFluentHashKeyMap[1][1].first == 2);
        CHECK(task->indexToStateFluentHashKeyMap[1][1].second == 16);
        // s2 affects s2 and hash key base is 31 (all actions, s0 and s1 also
        // affect s2)
        CHECK(task->indexToStateFluentHashKeyMap[2][0].first == 2);
        CHECK(task->indexToStateFluentHashKeyMap[2][0].second == 32);
    }
}
} // namespace hashing
} // namespace parser
} // namespace prost