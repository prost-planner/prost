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

    virtual void initSession() {}

    // This is called when caching is disabled because memory becomes sparse
    virtual void disableCaching() {}

    virtual void recommend(SearchNode const* rootNode,
                           std::vector<int>& bestActions) = 0;
    // Prints statistics
    virtual void printConfig(std::string indent) const;

protected:
    RecommendationFunction(THTS* _thts, std::string _name)
        : thts(_thts),
          name(_name) {}

    THTS* thts;

    // Name, used for output only
    std::string name;
};

class ExpectedBestArmRecommendation : public RecommendationFunction {
public:
    ExpectedBestArmRecommendation(THTS* _thts)
        : RecommendationFunction(_thts, "EBA recommendation function") {}

    void recommend(SearchNode const* rootNode,
                   std::vector<int>& bestActions) override;
};

class MostPlayedArmRecommendation : public RecommendationFunction {
public:
    MostPlayedArmRecommendation(THTS* _thts)
        : RecommendationFunction(_thts, "MPA recommendation function") {}

    void recommend(SearchNode const* rootNode,
                   std::vector<int>& bestActions) override;
};

#endif
