#ifndef HASH_KEY_GENERATOR_H
#define HASH_KEY_GENERATOR_H

class RDDLTask;

class HashKeyGenerator {
public:
    HashKeyGenerator(RDDLTask* task)
    : task(task) {}

    void generateHashKeys(bool output = true);

private:
    RDDLTask* task;

    void prepareStateHashKeys();
    void prepareKleeneStateHashKeys();
    void prepareEvaluatableHashKeys();
};

#endif
