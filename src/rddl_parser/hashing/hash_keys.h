#ifndef PARSER_HASHING_HASH_KEYS_H
#define PARSER_HASHING_HASH_KEYS_H

/*
  Prost uses perfect hash keys to speed up access to information that was
  computed once like, for instance, actions that are applicable in a state or
  KleeneState, state values of solved states, or the result of the evaluation
  of a precondition or CPF. Given a set of variables {v_1,...,v_n} with domains
  dom(v_1),....,dom(v_n) that consist of the first |dom(v_i)| natural numbers
  (including 0), we set the hash key base base(v_1) = 1 and compute base(v_i) as

  base(v_i) = |dom(v_1)| * |dom(v_2)| * ... * |dom(v_{i-1})|.

  The perfect hash key of state s = {v_1 -> d_1, v_2 > d_2, ..., v_n -> d_n}
  with d_i \in dom(v_i) for i = 1, ..., n is then computed as

  hash(s) = base(v_1) * d_1 + base(v_2) * d_2 + ... + base(v_n) * d_n

  The hash key generator class prepares information on different hash keys:

  1. In order to reuse computed information on States, we need to be able to
     look it up quickly. We do so with the help of perfect hash keys for states,
     which can be computed as long as the number of state variables is such that
     the largest perfect hash key fits into a long int.

  2. It is possible to compute perfect hash keys for KleeneStates if the number
     of state variables is such that the largest perfect hash key fits into a
     long int. In a KleeneState, every state variable v_i can take any number
     (larger than 0) of values from dom(v_i) at the same time, so the domain
     that is relevant for the computation of the hash key bases corresponds to
     the power set of dom(v_i) without the empty set.

  3. To minimize the number of formula evaluations during search, we store and
     reuse the results. The hash keys for the evaluation of such Evaluatables
     (i.e., CPFs, action preconditions or the reward function) depend not only
     on state variables, but also on action variables. On the other hand, it is
     sufficient to only consider those variables that actually occur in the
     formula. Hash key bases for each formula are computed as described above,
     and the set of variables only contains those state variables that actually
     occur in the formula.
     Action variables are treated as if they were a single variable: to compute
     the domain of that variable, all actions are partitioned into equivalence
     classes s.t. two actions are in the same equivalence class iff they share
     the assignment on all action variables relevant for the Evaluatable. Each
     equivalence class is then assigned a value in the domain.

  4. Akin to 3., we compute the same kind of hash keys for Kleene evaluation.
*/

#include <vector>

namespace prost::parser {
class ActionState;
struct Evaluatable;
struct RDDLTask;

namespace hashing {
class HashKeyGenerator {
public:
    HashKeyGenerator(RDDLTask* task) : task(task) {}

    void generateHashKeys(bool output = true);

private:
    RDDLTask* task;

    /*
      Determines if it is possible to compute perfect state hash keys. If state
      hashing is possible,  all summands of hash(s) (i.e., base(v_i) * d_1,
      base(v_i) * d2, ..., base(v_i) * (|dom(v_i| - 1)) are stored in the vector
      stateHashKeys of the RDDLTask object.
    */
    void prepareHashKeysForStates();

    /*
      Determines if it is possible to compute perfect hash keys for Kleene
      states. If KleeneState hashing is possible, all hash key bases are stored
      in the vector kleeneStateHashKeyBases of the RDDLTask object.
    */
    void prepareHashKeysForKleeneStates();

    /*
      Computes the hash keys for evaluate, evaluteToPD and evaluateToKleene of
      all Evaluatables.
    */
    void prepareHashKeysForEvaluatables();

    /*
      Computes the hash keys for evaluate, evaluteToPD and evaluateToKleene of
      Evaluatable eval.
    */
    void prepareHashKeysForEvaluatable(Evaluatable* eval, int hashIndex);

    /*
      Computes equivalence classes over actions for eval and assigns hash keys
    */
    long determineActionHashKeys(Evaluatable* eval);

    /*
      Checks for the action given by actionIndex if it is in the same
      equivalence class for eval as a previously considered action
    */
    long getActionHashKey(Evaluatable* eval, int actionIndex);

    /*
      Computes hash key bases for all state variables that affect eval
    */
    void prepareStateFluentHashKeys(Evaluatable* eval, long hashKeyBase);

    /*
      Computes Kleene hash key bases for all state variables that affect eval
    */
    void prepareKleeneStateFluentHashKeys(Evaluatable* eval, long hashKeyBase);
};
} // namespace hashing
} // namespace prost::parser

#endif // PARSER_HASHING_HASH_KEYS_H
