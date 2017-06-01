#ifndef RANDOM_H
#define RANDOM_H

#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <random>

// Base class for common functions regarding random number generation.
// URNG is the c++ concept UniformRandomBitGenerator.
template <class URNG = std::mt19937>
class Random {
public:
    virtual ~Random() = default;
    virtual int genInt(int min, int max) = 0;
    virtual double genDouble(double min, double max) = 0;
    virtual double genReal() = 0;

    // Returns an iterator to a randomly selected element
    template <typename Iter>
    Iter select(Iter start, Iter end) {
        std::advance(start, genInt(0, std::distance(start, end) - 1));
        return start;
    }

    // Returns a random element from a container that supports begin() and end()
    template <typename Container>
    auto randomElement(const Container& c) -> decltype(*begin(c))& {
        return *select(begin(c), end(c));
    }

    // Reinitialize the engine with a new seed
    virtual void seed(int value) = 0;

    // Some classes, like distributions, require a generator to produce random
    // numbers
    URNG getGenerator() const {
        return generator;
    }

protected:
    URNG generator;
};

// Random number generation with a mersenne twister generator
class RandomMT : public Random<std::mt19937> {
public:
    // Default constructor using a random seed.
    //
    // Note 1:
    // This is actually the proper way to seed a mt19937 engine.
    // The common initialization with
    //  RandomMT(Generator g = Generator(std::random_device{}())) : gen(g) {}
    // results in a single 32-bit integer for seeding, but the mt19937 engine
    // uses 624 32-bit integers for seeding. Therefore it would have only 2^32
    // possible output sequences (instead of 2^19937).
    //
    // Note 2:
    // This implementation uses std::random_device. Despite it's name, there
    // exist platforms (i.e. compilers), where this will result in deterministic
    // output. If you get the exact same numbers every run, check if
    // std::random_device is truly non-deterministic on your platform.
    RandomMT() {
        std::array<int, std::mt19937::state_size> seed_data;
        std::random_device r;
        std::generate_n(seed_data.data(), seed_data.size(), std::ref(r));
        std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
        generator.seed(seq);
    }

    // Generates a random int between [min, max]
    int genInt(int min, int max) override {
        return std::uniform_int_distribution<>{min, max}(generator);
    }

    // Generates a random double between [min, max]
    double genDouble(double min, double max) override {
        return std::uniform_real_distribution<>{min, max}(generator);
    }

    // Generates a random number between [0, 1)
    double genReal() override {
        return std::generate_canonical<double, 10>(generator);
    }

    // Reinitialize the engine with a new seed
    void seed(int value) override {
        generator.seed(value);
    }

};

#endif /* RANDOM_H */
