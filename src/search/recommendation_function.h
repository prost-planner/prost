#ifndef RECOMMENDATION_FUNCTION_H
#define RECOMMENDATION_FUNCTION_H

#include <string>
#include <vector>

class THTS;
class SearchNode;

class RecommendationFunction {
public:
    virtual ~RecommendationFunction() {}

    // Create a recommendation function component
    static RecommendationFunction* fromString(std::string& desc, THTS* thts);

    // Set parameters from command line
    virtual bool setValueFromString(std::string& /*param*/,
                                    std::string& /*value*/) {
        return false;
    }

    // Learn parameter values from a random training set
    virtual void learn() {}

    // This is called when caching is disabled because memory becomes sparse
    virtual void disableCaching() {}

    virtual void initRound() {}

    virtual void recommend(SearchNode const* rootNode,
                           std::vector<int>& bestActions) = 0;

protected:
    RecommendationFunction(THTS* thts) : thts(thts) {}

    THTS* thts;
};

class ExpectedBestArmRecommendation : public RecommendationFunction {
public:
    ExpectedBestArmRecommendation(THTS* thts) : RecommendationFunction(thts) {}

    void recommend(SearchNode const* rootNode,
                   std::vector<int>& bestActions) override;
};

class MostPlayedArmRecommendation : public RecommendationFunction {
public:
    MostPlayedArmRecommendation(THTS* thts) : RecommendationFunction(thts) {}

    void recommend(SearchNode const* rootNode,
                   std::vector<int>& bestActions) override;
};

#endif
