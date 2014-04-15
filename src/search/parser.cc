#include "parser.h"

#include "search_engine.h"

#include "utils/string_utils.h"
#include "utils/system_utils.h"

using namespace std;

void Parser::parseTask(map<string,int>& stateVariableIndices, vector<vector<string> >& stateVariableValues) {
    // Read the parser output file
    string problemDesc;
    if(!SystemUtils::readFile(problemFileName, problemDesc, "#")) {
        SystemUtils::abort("Error: Unable to read problem file: " + problemFileName);
    }
    stringstream desc(problemDesc);

    // Parse general task properties
    desc >> SearchEngine::taskName;
    desc >> SearchEngine::horizon;
    desc >> SearchEngine::discountFactor;

    // Parse relevant numbers
    int numberOfActionFluents;
    desc >> numberOfActionFluents;

    int numberOfDetStateFluents;
    desc >> numberOfDetStateFluents;
    SearchEngine::firstProbabilisticVarIndex = numberOfDetStateFluents;

    int numberOfProbStateFluents;
    desc >> numberOfProbStateFluents;

    int numberOfPreconds;
    desc >> numberOfPreconds;

    desc >> SearchEngine::numberOfActions;
    desc >> State::numberOfStateFluentHashKeys;
    KleeneState::numberOfStateFluentHashKeys = State::numberOfStateFluentHashKeys;

    State::stateSize = numberOfDetStateFluents + numberOfProbStateFluents;
    KleeneState::stateSize = State::stateSize;

    // Parse initial state
    vector<double> initialVals(State::stateSize, 0.0);
    for(unsigned int i = 0; i < State::stateSize; ++i) {
        desc >> initialVals[i];
    }
    SearchEngine::initialState = State(initialVals, SearchEngine::horizon);

    // Parse task properties
    desc >> SearchEngine::taskIsDeterministic;
    desc >> State::stateHashingPossible;
    desc >> KleeneState::stateHashingPossible;

    string finalReward;
    desc >> finalReward;
    if(finalReward == "NOOP") {
        SearchEngine::finalRewardCalculationMethod = SearchEngine::NOOP;
    } else if(finalReward == "FIRST_APPLICABLE") {
        SearchEngine::finalRewardCalculationMethod = SearchEngine::FIRST_APPLICABLE;
    } else {
        assert(finalReward == "BEST_OF_CANDIDATE_SET");
        SearchEngine::finalRewardCalculationMethod = SearchEngine::BEST_OF_CANDIDATE_SET;

        int sizeOfCandidateSet;
        desc >> sizeOfCandidateSet;
        SearchEngine::candidatesForOptimalFinalAction.resize(sizeOfCandidateSet);
        for(unsigned int i = 0; i < sizeOfCandidateSet; ++i) {
            desc >> SearchEngine::candidatesForOptimalFinalAction[i];
        }
    }
    desc >> SearchEngine::useRewardLockDetection;
    if(SearchEngine::useRewardLockDetection) {
        SearchEngine::goalTestActionIndex = 0;
    } else {
        SearchEngine::goalTestActionIndex = -1;
    }

    // Parse action fluents
    for(unsigned int i = 0; i < numberOfActionFluents; ++i) {
        parseActionFluent(desc);
    }

    // Parse state fluents and CPFs (Since we cannot create the CPF formulas
    // before all fluents have been parsed, we collect them as strings and
    // create them later)
    vector<string> formulas;
    vector<string> detFormulas;
    for(unsigned int i = 0; i < numberOfDetStateFluents; ++i) {
        parseCPF(desc, formulas, detFormulas, false);
    }

    for(unsigned int i = 0; i < numberOfProbStateFluents; ++i) {
        parseCPF(desc, formulas, detFormulas, true);
    }

    // All fluents have been created -> create the CPF formulas
    for(unsigned int i = 0; i < SearchEngine::probCPFs.size(); ++i) {
        SearchEngine::probCPFs[i]->formula = LogicalExpression::createFromString(SearchEngine::stateFluents, SearchEngine::actionFluents, formulas[i]);
        if(i < numberOfDetStateFluents) {
            assert(detFormulas[i].empty());
            assert(SearchEngine::probCPFs[i] == SearchEngine::detCPFs[i]);
        } else {
            SearchEngine::detCPFs[i]->formula = LogicalExpression::createFromString(SearchEngine::stateFluents, SearchEngine::actionFluents, detFormulas[i]);
        }
    }

    // Parse reward function
    parseRewardFunction(desc);

    // Parse action preconditions
    for(unsigned int i = 0; i < numberOfPreconds; ++i) {
        parseActionPrecondition(desc);
    }

    // Parse action states
    for(unsigned int i = 0; i < SearchEngine::numberOfActions; ++i) {
        parseActionState(desc);
    }

    // Parse hash keys
    if(State::stateHashingPossible) {
        State::stateHashKeys.resize(State::stateSize);
    }
    State::indexToStateFluentHashKeyMap.resize(State::stateSize);

    if(KleeneState::stateHashingPossible) {
        KleeneState::hashKeyBases.resize(KleeneState::stateSize);
    }
    KleeneState::indexToStateFluentHashKeyMap.resize(KleeneState::stateSize);

    parseHashKeys(desc);

    // Calculate hash keys of initial state
    State::calcStateFluentHashKeys(SearchEngine::initialState);
    State::calcStateHashKey(SearchEngine::initialState);

    // Parse training set
    parseTrainingSet(desc);

    // Set mapping of variables to variable names and of values as strings to
    // internal values for communication between planner and environment
    for(unsigned int i = 0; i < SearchEngine::probCPFs.size(); ++i) {
        assert(stateVariableIndices.find(SearchEngine::probCPFs[i]->name) == stateVariableIndices.end());
        stateVariableIndices[SearchEngine::probCPFs[i]->name] = i;
        stateVariableValues.push_back(SearchEngine::probCPFs[i]->head->values);
    }
}

void Parser::parseActionFluent(stringstream& desc) const {
    int index;
    desc >> index;

    string name;
    desc.ignore(1, '\n');
    getline(desc, name, '\n');

    int numberOfValues;
    desc >> numberOfValues;

    vector<string> values;
    for(unsigned int j = 0; j < numberOfValues; ++j) {
        int val;
        string value;

        desc >> val;
        desc >> value;
        assert(val == j);
        values.push_back(value);
    }

    SearchEngine::actionFluents.push_back(new ActionFluent(index, name, values));
}

void Parser::parseCPF(stringstream& desc,
                      vector<string>& formulas,
                      vector<string>& detFormulas,
                      bool const& isProbabilistic) const {
    int index;
    desc >> index;

    string name;
    desc.ignore(1, '\n');
    getline(desc, name, '\n');

    int numberOfValues;
    desc >> numberOfValues;

    vector<string> values;
    for(unsigned int j = 0; j < numberOfValues; ++j) {
        int val;
        string value;

        desc >> val;
        desc >> value;
        assert(val == j);
        values.push_back(value);
    }

    string formula;
    desc.ignore(1, '\n');
    getline(desc, formula, '\n');
    formulas.push_back(formula);
    if(isProbabilistic) {
        getline(desc, formula, '\n');
        detFormulas.push_back(formula);
    } else {
        detFormulas.push_back("");
    }

    int hashIndex;
    desc >> hashIndex;

    StateFluent* sf = new StateFluent(index, name, values);
    SearchEngine::stateFluents.push_back(sf);

    if(isProbabilistic) {
        ConditionalProbabilityFunction* probCPF = new ConditionalProbabilityFunction(isProbabilistic, hashIndex, sf);
        ConditionalProbabilityFunction* detCPF = new ConditionalProbabilityFunction(isProbabilistic, hashIndex, sf);
        parseCachingType(desc, probCPF, detCPF);
        parseActionHashKeyMap(desc, probCPF, detCPF);

        SearchEngine::probCPFs.push_back(probCPF);
        SearchEngine::detCPFs.push_back(detCPF);
        
    } else {
        ConditionalProbabilityFunction* cpf = new ConditionalProbabilityFunction(isProbabilistic, hashIndex, sf);
        parseCachingType(desc, cpf, NULL);
        parseActionHashKeyMap(desc, cpf, NULL);

        SearchEngine::probCPFs.push_back(cpf);
        SearchEngine::detCPFs.push_back(cpf);
    }
}

void Parser::parseRewardFunction(stringstream& desc) const {
    string formulaAsString;
    desc.ignore(1, '\n');
    getline(desc, formulaAsString, '\n');
    LogicalExpression* rewardFormula = LogicalExpression::createFromString(SearchEngine::stateFluents, SearchEngine::actionFluents, formulaAsString);

    double minVal;
    desc >> minVal;

    double maxVal;
    desc >> maxVal;

    int hashIndex;
    desc >> hashIndex;

    SearchEngine::rewardCPF = new RewardFunction(rewardFormula, hashIndex, minVal, maxVal);

    parseCachingType(desc, SearchEngine::rewardCPF, NULL);
    parseActionHashKeyMap(desc, SearchEngine::rewardCPF, NULL);
}

void Parser::parseActionPrecondition(stringstream& desc) const {
    int index;
    desc >> index;

    stringstream name;
    name << "Precond " << index;

    string formulaAsString;
    desc.ignore(1, '\n');
    getline(desc, formulaAsString, '\n');

    LogicalExpression* formula = LogicalExpression::createFromString(SearchEngine::stateFluents, SearchEngine::actionFluents, formulaAsString);

    int hashIndex;
    desc >> hashIndex;

    Evaluatable* precond = new Evaluatable(name.str(), formula, false, hashIndex);

    assert(SearchEngine::actionPreconditions.size() == index);
    SearchEngine::actionPreconditions.push_back(precond);

    parseCachingType(desc, precond, NULL);
    parseActionHashKeyMap(desc, precond, NULL);
}

void Parser::parseCachingType(stringstream& desc, Evaluatable* probEval, Evaluatable* detEval) const {
    string cachingType;
    desc >> cachingType;

    if(cachingType == "NONE") {
        probEval->cachingType = Evaluatable::NONE;
        if(detEval) {
            detEval->cachingType = Evaluatable::NONE;
        }
    } else if(cachingType == "VECTOR") {
        probEval->cachingType = Evaluatable::VECTOR;

        int cachingVecSize;
        desc >> cachingVecSize;

        if(detEval) {
            detEval->cachingType = Evaluatable::VECTOR;
            detEval->evaluationCacheVector.resize(cachingVecSize, -numeric_limits<double>::max());
            probEval->pdEvaluationCacheVector.resize(cachingVecSize);

            for(unsigned int i = 0; i < cachingVecSize; ++i) {
                int key;
                desc >> key;
                assert(key == i);
                desc >> detEval->evaluationCacheVector[i];

                int sizeOfPD;
                desc >> sizeOfPD;

                probEval->pdEvaluationCacheVector[i].values.resize(sizeOfPD);
                probEval->pdEvaluationCacheVector[i].probabilities.resize(sizeOfPD);
                for(unsigned int j = 0; j < sizeOfPD; ++j) {
                    desc >> probEval->pdEvaluationCacheVector[i].values[j];
                    desc >> probEval->pdEvaluationCacheVector[i].probabilities[j];
                }
                assert(probEval->pdEvaluationCacheVector[i].isWellDefined());
            }
        } else {
            probEval->evaluationCacheVector.resize(cachingVecSize, -numeric_limits<double>::max());
            for(unsigned int i = 0; i < cachingVecSize; ++i) {
                int key;
                desc >> key;
                assert(key == i);
                desc >> probEval->evaluationCacheVector[i];
            }
        }
    } else if(cachingType == "MAP") {
        probEval->cachingType = Evaluatable::MAP;
        if(detEval) {
            detEval->cachingType = Evaluatable::MAP;
        }
    }

    desc >> cachingType;
    if(cachingType == "NONE") {
        probEval->kleeneCachingType = Evaluatable::NONE;
    } else if(cachingType == "VECTOR") {
        probEval->kleeneCachingType = Evaluatable::VECTOR;

        int cachingVecSize;
        desc >> cachingVecSize;
        probEval->kleeneEvaluationCacheVector.resize(cachingVecSize);
    } else if(cachingType == "MAP") {
        probEval->kleeneCachingType = Evaluatable::MAP;
    }

    // We don't perform Kleene operations on the determinization
    if(detEval) {
        detEval->kleeneCachingType = Evaluatable::NONE;
    }
}

void Parser::parseActionHashKeyMap(stringstream& desc, Evaluatable* probEval, Evaluatable* detEval) const {
    probEval->actionHashKeyMap.resize(SearchEngine::numberOfActions);
    if(detEval) {
        detEval->actionHashKeyMap.resize(SearchEngine::numberOfActions);
    }

    for(unsigned int i = 0; i < SearchEngine::numberOfActions; ++i) {
        int actionIndex;
        desc >> actionIndex;
        assert(actionIndex == i);
        desc >> probEval->actionHashKeyMap[i];
        if(detEval) {
            detEval->actionHashKeyMap[i] = probEval->actionHashKeyMap[i];
        }
    }
}

void Parser::parseActionState(stringstream& desc) const {
    int index;
    desc >> index;

    vector<int> values(SearchEngine::actionFluents.size());
    vector<ActionFluent*> scheduledActionFluents;

    for(unsigned int j = 0; j < SearchEngine::actionFluents.size(); ++j) {
        desc >> values[j];
        if(values[j] == 1) {
            scheduledActionFluents.push_back(SearchEngine::actionFluents[j]);
        }
    }

    int numberOfRelevantPreconditions;
    desc >> numberOfRelevantPreconditions;
    vector<Evaluatable*> relevantPreconditions(numberOfRelevantPreconditions);

    for(unsigned int j = 0; j < numberOfRelevantPreconditions; ++j) {
        int precondIndex;
        desc >> precondIndex;
        relevantPreconditions[j] = SearchEngine::actionPreconditions[precondIndex];
    }

    SearchEngine::actionStates.push_back(ActionState(index, values, scheduledActionFluents, relevantPreconditions));
}

void Parser::parseHashKeys(stringstream& desc) const {
    for(unsigned int i = 0; i < SearchEngine::probCPFs.size(); ++i) {
        int index;
        desc >> index;
        assert(index == i);

        if(State::stateHashingPossible) {
            State::stateHashKeys[index].resize(SearchEngine::probCPFs[index]->head->values.size());
            for(unsigned int j = 0; j < SearchEngine::probCPFs[index]->head->values.size(); ++j) {
                desc >> State::stateHashKeys[index][j];
            }
        }

        if(KleeneState::stateHashingPossible) {
            desc >> KleeneState::hashKeyBases[index];
        }

        int numberOfKeys;
        desc >> numberOfKeys;
        for(unsigned int j = 0; j < numberOfKeys; ++j) {
            int var;
            long key;
            desc >> var >> key;
            State::indexToStateFluentHashKeyMap[index].push_back(make_pair(var, key));
        }

        desc >> numberOfKeys;
        for(unsigned int j = 0; j < numberOfKeys; ++j) {
            int var;
            long key;
            desc >> var >> key;
            KleeneState::indexToStateFluentHashKeyMap[index].push_back(make_pair(var, key));
        }
    }
}

void Parser::parseTrainingSet(stringstream& desc) const {
    int numberOfTrainingStates;
    desc >> numberOfTrainingStates;
    for(unsigned int i = 0; i < numberOfTrainingStates; ++i) {
        vector<double> values(State::stateSize);
        for(unsigned int j = 0; j < State::stateSize; ++j) {
            desc >> values[j];
        }
        State trainingState(values, SearchEngine::horizon);
        State::calcStateFluentHashKeys(trainingState);
        State::calcStateHashKey(trainingState);
        SearchEngine::trainingSet.push_back(trainingState);
    }
}
