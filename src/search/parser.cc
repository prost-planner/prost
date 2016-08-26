#include "parser.h"

#include "search_engine.h"

#include "utils/string_utils.h"
#include "utils/system_utils.h"

using namespace std;

void Parser::parseTask(map<string, int>& stateVariableIndices,
                       vector<vector<string>>& stateVariableValues) {
    // Read the parser output file
    string problemDesc;
    if (!SystemUtils::readFile(problemFileName, problemDesc, "#")) {
        SystemUtils::abort("Error: Unable to read problem file: " +
                           problemFileName);
    }
    stringstream desc(problemDesc);
    resetStatics();
    // Parse general task properties
    desc >> SearchEngine::taskName;
    desc >> SearchEngine::horizon;
    desc >> SearchEngine::discountFactor;

    // Parse numbers of fluents and evaluatables
    int numberOfActionFluents;
    desc >> numberOfActionFluents;

    desc >> State::numberOfDeterministicStateFluents;
    desc >> State::numberOfProbabilisticStateFluents;

    int numberOfPreconds;
    desc >> numberOfPreconds;

    desc >> SearchEngine::numberOfActions;
    desc >> State::numberOfStateFluentHashKeys;
    KleeneState::numberOfStateFluentHashKeys =
        State::numberOfStateFluentHashKeys;

    KleeneState::stateSize = State::numberOfDeterministicStateFluents +
                             State::numberOfProbabilisticStateFluents;

    // Parse initial state
    vector<double> initialValsOfDeterministicStateFluents(
        State::numberOfDeterministicStateFluents, 0.0);
    for (size_t i = 0; i < State::numberOfDeterministicStateFluents; ++i) {
        desc >> initialValsOfDeterministicStateFluents[i];
    }
    vector<double> initialValsOfProbabilisticStateFluents(
        State::numberOfProbabilisticStateFluents, 0.0);
    for (size_t i = 0; i < State::numberOfProbabilisticStateFluents; ++i) {
        desc >> initialValsOfProbabilisticStateFluents[i];
    }
    SearchEngine::initialState =
        State(initialValsOfDeterministicStateFluents,
              initialValsOfProbabilisticStateFluents, SearchEngine::horizon);

    // Parse task properties
    desc >> SearchEngine::taskIsDeterministic;
    desc >> State::stateHashingPossible;
    desc >> KleeneState::stateHashingPossible;

    string finalReward;
    desc >> finalReward;
    if (finalReward == "NOOP") {
        SearchEngine::finalRewardCalculationMethod = SearchEngine::NOOP;
    } else if (finalReward == "FIRST_APPLICABLE") {
        SearchEngine::finalRewardCalculationMethod =
            SearchEngine::FIRST_APPLICABLE;
    } else {
        assert(finalReward == "BEST_OF_CANDIDATE_SET");
        SearchEngine::finalRewardCalculationMethod =
            SearchEngine::BEST_OF_CANDIDATE_SET;

        int sizeOfCandidateSet;
        desc >> sizeOfCandidateSet;
        SearchEngine::candidatesForOptimalFinalAction.resize(
            sizeOfCandidateSet);
        for (size_t i = 0; i < sizeOfCandidateSet; ++i) {
            desc >> SearchEngine::candidatesForOptimalFinalAction[i];
        }
    }
    desc >> SearchEngine::rewardLockDetected;
    if (SearchEngine::rewardLockDetected) {
        SearchEngine::goalTestActionIndex = 0;
    } else {
        SearchEngine::goalTestActionIndex = -1;
    }

    desc >> ProbabilisticSearchEngine::hasUnreasonableActions;
    desc >> DeterministicSearchEngine::hasUnreasonableActions;

    int encounteredStates;
    int encounteredUnqiueStates;
    int encounteredStatesWithUniqueAction;
    int encounteredUnqiueStatesWithUniqueAction;
    desc >> encounteredStates >> encounteredUnqiueStates >>
        encounteredStatesWithUniqueAction >>
        encounteredUnqiueStatesWithUniqueAction;
    // TODO: These are currently ignored, but we could use they are interesting
    // properties of planning tasks that could be used, e.g., for time
    // management

    // Parse action fluents
    for (size_t i = 0; i < numberOfActionFluents; ++i) {
        parseActionFluent(desc);
    }

    // Parse state fluents and CPFs (Since we cannot create the CPF formulas
    // before all fluents have been parsed, we collect them as strings and
    // create them later)
    vector<string> deterministicFormulas;
    vector<string> probabilisticFormulas;
    vector<string> determinizedFormulas;
    for (size_t i = 0; i < State::numberOfDeterministicStateFluents; ++i) {
        parseCPF(desc, deterministicFormulas, probabilisticFormulas,
                 determinizedFormulas, false);
    }

    for (size_t i = 0; i < State::numberOfProbabilisticStateFluents; ++i) {
        parseCPF(desc, deterministicFormulas, probabilisticFormulas,
                 determinizedFormulas, true);
    }

    assert(deterministicFormulas.size() ==
           State::numberOfDeterministicStateFluents);
    assert(SearchEngine::deterministicCPFs.size() ==
           State::numberOfDeterministicStateFluents);

    assert(probabilisticFormulas.size() ==
           State::numberOfProbabilisticStateFluents);
    assert(SearchEngine::probabilisticCPFs.size() ==
           State::numberOfProbabilisticStateFluents);

    assert(determinizedFormulas.size() ==
           State::numberOfProbabilisticStateFluents);
    assert(SearchEngine::determinizedCPFs.size() ==
           State::numberOfProbabilisticStateFluents);

    assert(SearchEngine::allCPFs.size() == KleeneState::stateSize);

    // All fluents have been created -> create the CPF formulas
    for (size_t i = 0; i < State::numberOfDeterministicStateFluents; ++i) {
        SearchEngine::deterministicCPFs[i]->formula =
            LogicalExpression::createFromString(deterministicFormulas[i]);
    }

    for (size_t i = 0; i < State::numberOfProbabilisticStateFluents; ++i) {
        SearchEngine::probabilisticCPFs[i]->formula =
            LogicalExpression::createFromString(probabilisticFormulas[i]);
        SearchEngine::determinizedCPFs[i]->formula =
            LogicalExpression::createFromString(determinizedFormulas[i]);
    }

    // Parse reward function
    parseRewardFunction(desc);

    // Parse action preconditions
    for (size_t i = 0; i < numberOfPreconds; ++i) {
        parseActionPrecondition(desc);
    }

    // Parse action states
    for (size_t i = 0; i < SearchEngine::numberOfActions; ++i) {
        parseActionState(desc);
    }

    // Parse hash keys
    if (State::stateHashingPossible) {
        State::stateHashKeysOfDeterministicStateFluents.resize(
            State::numberOfDeterministicStateFluents);
        State::stateHashKeysOfProbabilisticStateFluents.resize(
            State::numberOfProbabilisticStateFluents);
    }
    State::stateFluentHashKeysOfDeterministicStateFluents.resize(
        State::numberOfDeterministicStateFluents);
    State::stateFluentHashKeysOfProbabilisticStateFluents.resize(
        State::numberOfProbabilisticStateFluents);

    if (KleeneState::stateHashingPossible) {
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
    for (size_t i = 0; i < State::numberOfDeterministicStateFluents; ++i) {
        assert(stateVariableIndices.find(
                   SearchEngine::deterministicCPFs[i]->name) ==
               stateVariableIndices.end());
        stateVariableIndices[SearchEngine::deterministicCPFs[i]->name] = i;
        stateVariableValues.push_back(
            SearchEngine::deterministicCPFs[i]->head->values);
    }
    for (size_t i = 0; i < State::numberOfProbabilisticStateFluents; ++i) {
        assert(stateVariableIndices.find(
                   SearchEngine::probabilisticCPFs[i]->name) ==
               stateVariableIndices.end());
        stateVariableIndices[SearchEngine::probabilisticCPFs[i]->name] =
            State::numberOfDeterministicStateFluents + i;
        stateVariableValues.push_back(
            SearchEngine::probabilisticCPFs[i]->head->values);
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
    for (size_t j = 0; j < numberOfValues; ++j) {
        int val;
        string value;

        desc >> val;
        desc >> value;
        assert(val == j);
        values.push_back(value);
    }
    SearchEngine::actionFluents.push_back(
        new ActionFluent(index, name, values));
}

void Parser::parseCPF(stringstream& desc, vector<string>& deterministicFormulas,
                      vector<string>& probabilisticFormulas,
                      vector<string>& determinizedFormulas,
                      bool const& isProbabilistic) const {
    int index;
    desc >> index;

    string name;
    desc.ignore(1, '\n');
    getline(desc, name, '\n');

    int numberOfValues;
    desc >> numberOfValues;

    vector<string> values;
    for (size_t j = 0; j < numberOfValues; ++j) {
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
    if (isProbabilistic) {
        probabilisticFormulas.push_back(formula);
        getline(desc, formula, '\n');
        determinizedFormulas.push_back(formula);
    } else {
        deterministicFormulas.push_back(formula);
    }

    int hashIndex;
    desc >> hashIndex;

    if (isProbabilistic) {
        ProbabilisticStateFluent* sf =
            new ProbabilisticStateFluent(index, name, values);
        SearchEngine::stateFluents.push_back(sf);

        ProbabilisticCPF* probCPF = new ProbabilisticCPF(hashIndex, sf);
        DeterministicCPF* detCPF = new DeterministicCPF(hashIndex, sf);
        parseCachingType(desc, probCPF, detCPF);
        parseActionHashKeyMap(desc, probCPF, detCPF);

        SearchEngine::probabilisticCPFs.push_back(probCPF);
        SearchEngine::allCPFs.push_back(probCPF);

        SearchEngine::determinizedCPFs.push_back(detCPF);
    } else {
        DeterministicStateFluent* sf =
            new DeterministicStateFluent(index, name, values);
        SearchEngine::stateFluents.push_back(sf);

        DeterministicCPF* cpf = new DeterministicCPF(hashIndex, sf);
        parseCachingType(desc, nullptr, cpf);
        parseActionHashKeyMap(desc, nullptr, cpf);

        SearchEngine::deterministicCPFs.push_back(cpf);
        SearchEngine::allCPFs.push_back(cpf);
    }
}

void Parser::parseRewardFunction(stringstream& desc) const {
    string formulaAsString;
    desc.ignore(1, '\n');
    getline(desc, formulaAsString, '\n');
    LogicalExpression* rewardFormula =
        LogicalExpression::createFromString(formulaAsString);

    double minVal;
    desc >> minVal;

    double maxVal;
    desc >> maxVal;

    bool actionIndependent;
    desc >> actionIndependent;

    int hashIndex;
    desc >> hashIndex;

    SearchEngine::rewardCPF = new RewardFunction(
        rewardFormula, hashIndex, minVal, maxVal, actionIndependent);

    parseCachingType(desc, nullptr, SearchEngine::rewardCPF);
    parseActionHashKeyMap(desc, nullptr, SearchEngine::rewardCPF);
}

void Parser::parseActionPrecondition(stringstream& desc) const {
    int index;
    desc >> index;

    stringstream name;
    name << "Precond " << index;

    string formulaAsString;
    desc.ignore(1, '\n');
    getline(desc, formulaAsString, '\n');

    LogicalExpression* formula =
        LogicalExpression::createFromString(formulaAsString);

    int hashIndex;
    desc >> hashIndex;

    DeterministicEvaluatable* precond =
        new DeterministicEvaluatable(name.str(), formula, hashIndex);

    assert(SearchEngine::actionPreconditions.size() == index);
    SearchEngine::actionPreconditions.push_back(precond);

    parseCachingType(desc, nullptr, precond);
    parseActionHashKeyMap(desc, nullptr, precond);
}

void Parser::parseCachingType(stringstream& desc,
                              ProbabilisticEvaluatable* probEval,
                              DeterministicEvaluatable* detEval) const {
    string cachingType;
    desc >> cachingType;

    if (cachingType == "NONE") {
        detEval->cachingType = Evaluatable::NONE;
        if (probEval) {
            probEval->cachingType = Evaluatable::NONE;
        }
    } else if (cachingType == "VECTOR") {
        int cachingVecSize;
        desc >> cachingVecSize;

        detEval->cachingType = Evaluatable::VECTOR;
        detEval->evaluationCacheVector.resize(cachingVecSize,
                                              -numeric_limits<double>::max());

        if (probEval) {
            probEval->cachingType = Evaluatable::VECTOR;
            probEval->evaluationCacheVector.resize(cachingVecSize);

            for (size_t i = 0; i < cachingVecSize; ++i) {
                int key;
                desc >> key;
                assert(key == i);
                desc >> detEval->evaluationCacheVector[i];

                int sizeOfPD;
                desc >> sizeOfPD;

                probEval->evaluationCacheVector[i].values.resize(sizeOfPD);
                probEval->evaluationCacheVector[i].probabilities.resize(
                    sizeOfPD);
                for (size_t j = 0; j < sizeOfPD; ++j) {
                    desc >> probEval->evaluationCacheVector[i].values[j];
                    desc >> probEval->evaluationCacheVector[i].probabilities[j];
                }
                assert(probEval->evaluationCacheVector[i].isWellDefined());
            }
        } else {
            for (size_t i = 0; i < cachingVecSize; ++i) {
                int key;
                desc >> key;
                assert(key == i);
                desc >> detEval->evaluationCacheVector[i];
            }
        }
    } else {
        assert(cachingType == "MAP");
        detEval->cachingType = Evaluatable::MAP;
        detEval->evaluationCacheMap.reserve(256279);
        if (probEval) {
            probEval->cachingType = Evaluatable::MAP;
            probEval->evaluationCacheMap.reserve(256279);
        }
    }

    desc >> cachingType;
    if (cachingType == "NONE") {
        detEval->kleeneCachingType = Evaluatable::NONE;
        if (probEval) {
            probEval->kleeneCachingType = Evaluatable::NONE;
        }
    } else if (cachingType == "VECTOR") {
        int cachingVecSize;
        desc >> cachingVecSize;

        if (probEval) {
            probEval->kleeneCachingType = Evaluatable::VECTOR;
            probEval->kleeneEvaluationCacheVector.resize(cachingVecSize);
            detEval->kleeneCachingType = Evaluatable::NONE;
        } else {
            detEval->kleeneCachingType = Evaluatable::VECTOR;
            detEval->kleeneEvaluationCacheVector.resize(cachingVecSize);
        }
    } else {
        assert(cachingType == "MAP");
        if (probEval) {
            probEval->kleeneCachingType = Evaluatable::MAP;
            probEval->evaluationCacheMap.reserve(256279);
            detEval->kleeneCachingType = Evaluatable::NONE;
        } else {
            detEval->kleeneCachingType = Evaluatable::MAP;
            detEval->evaluationCacheMap.reserve(256279);
        }
    }
}

void Parser::parseActionHashKeyMap(stringstream& desc,
                                   ProbabilisticEvaluatable* probEval,
                                   DeterministicEvaluatable* detEval) const {
    detEval->actionHashKeyMap.resize(SearchEngine::numberOfActions);
    if (probEval) {
        probEval->actionHashKeyMap.resize(SearchEngine::numberOfActions);
    }

    for (size_t i = 0; i < SearchEngine::numberOfActions; ++i) {
        int actionIndex;
        desc >> actionIndex;
        assert(actionIndex == i);
        desc >> detEval->actionHashKeyMap[i];
        if (probEval) {
            probEval->actionHashKeyMap[i] = detEval->actionHashKeyMap[i];
        }
    }
}

void Parser::parseActionState(stringstream& desc) const {
    int index;
    desc >> index;

    vector<int> values(SearchEngine::actionFluents.size());
    vector<ActionFluent*> scheduledActionFluents;

    for (size_t j = 0; j < SearchEngine::actionFluents.size(); ++j) {
        desc >> values[j];
        if (values[j] == 1) {
            scheduledActionFluents.push_back(SearchEngine::actionFluents[j]);
        }
    }

    int numberOfRelevantPreconditions;
    desc >> numberOfRelevantPreconditions;
    vector<DeterministicEvaluatable*> relevantPreconditions(
        numberOfRelevantPreconditions);

    for (size_t j = 0; j < numberOfRelevantPreconditions; ++j) {
        int precondIndex;
        desc >> precondIndex;
        relevantPreconditions[j] =
            SearchEngine::actionPreconditions[precondIndex];
    }

    SearchEngine::actionStates.push_back(ActionState(
        index, values, scheduledActionFluents, relevantPreconditions));
}

void Parser::parseHashKeys(stringstream& desc) const {
    for (size_t i = 0; i < State::numberOfDeterministicStateFluents; ++i) {
        int index;
        desc >> index;
        assert(index == i);

        if (State::stateHashingPossible) {
            State::stateHashKeysOfDeterministicStateFluents[index].resize(
                SearchEngine::deterministicCPFs[index]->head->values.size());
            for (size_t j = 0;
                 j <
                 SearchEngine::deterministicCPFs[index]->head->values.size();
                 ++j) {
                desc >>
                    State::stateHashKeysOfDeterministicStateFluents[index][j];
            }
        }

        if (KleeneState::stateHashingPossible) {
            desc >> KleeneState::hashKeyBases[index];
        }

        int numberOfKeys;
        desc >> numberOfKeys;
        for (size_t j = 0; j < numberOfKeys; ++j) {
            int var;
            long key;
            desc >> var >> key;
            State::stateFluentHashKeysOfDeterministicStateFluents[index]
                .push_back(make_pair(var, key));
        }

        desc >> numberOfKeys;
        for (size_t j = 0; j < numberOfKeys; ++j) {
            int var;
            long key;
            desc >> var >> key;
            KleeneState::indexToStateFluentHashKeyMap[index].push_back(
                make_pair(var, key));
        }
    }

    for (size_t i = 0; i < State::numberOfProbabilisticStateFluents; ++i) {
        int index;
        desc >> index;
        assert(index == i);

        if (State::stateHashingPossible) {
            State::stateHashKeysOfProbabilisticStateFluents[index].resize(
                SearchEngine::probabilisticCPFs[index]->head->values.size());
            for (size_t j = 0;
                 j <
                 SearchEngine::probabilisticCPFs[index]->head->values.size();
                 ++j) {
                desc >>
                    State::stateHashKeysOfProbabilisticStateFluents[index][j];
            }
        }

        if (KleeneState::stateHashingPossible) {
            desc >> KleeneState::hashKeyBases
                        [index + State::numberOfDeterministicStateFluents];
        }

        int numberOfKeys;
        desc >> numberOfKeys;
        for (size_t j = 0; j < numberOfKeys; ++j) {
            int var;
            long key;
            desc >> var >> key;
            State::stateFluentHashKeysOfProbabilisticStateFluents[index]
                .push_back(make_pair(var, key));
        }

        desc >> numberOfKeys;
        for (size_t j = 0; j < numberOfKeys; ++j) {
            int var;
            long key;
            desc >> var >> key;
            KleeneState::indexToStateFluentHashKeyMap
                [index + State::numberOfDeterministicStateFluents]
                    .push_back(make_pair(var, key));
        }
    }
}

void Parser::parseTrainingSet(stringstream& desc) const {
    int numberOfTrainingStates;
    desc >> numberOfTrainingStates;
    for (size_t i = 0; i < numberOfTrainingStates; ++i) {
        vector<double> valuesOfDeterministicStateFluents(
            State::numberOfDeterministicStateFluents);
        for (size_t j = 0; j < State::numberOfDeterministicStateFluents; ++j) {
            desc >> valuesOfDeterministicStateFluents[j];
        }

        vector<double> valuesOfProbabilisticStateFluents(
            State::numberOfProbabilisticStateFluents);
        for (size_t j = 0; j < State::numberOfProbabilisticStateFluents; ++j) {
            desc >> valuesOfProbabilisticStateFluents[j];
        }

        State trainingState(valuesOfDeterministicStateFluents,
                            valuesOfProbabilisticStateFluents,
                            SearchEngine::horizon);
        State::calcStateFluentHashKeys(trainingState);
        State::calcStateHashKey(trainingState);
        SearchEngine::trainingSet.push_back(trainingState);
    }
}

// Resets static variables of SearchEngine, State, KleeneState. Necessary for
// unit tests where the static variables have to be reset between test
// instances.
void Parser::resetStatics() const {
    SearchEngine::actionFluents.clear();
    SearchEngine::stateFluents.clear();
    SearchEngine::probabilisticCPFs.clear();
    SearchEngine::allCPFs.clear();
    SearchEngine::determinizedCPFs.clear();
    SearchEngine::deterministicCPFs.clear();
    SearchEngine::actionPreconditions.clear();
    SearchEngine::actionStates.clear();
    SearchEngine::trainingSet.clear();
    State::stateFluentHashKeysOfDeterministicStateFluents.clear();
    State::stateFluentHashKeysOfProbabilisticStateFluents.clear();
    KleeneState::indexToStateFluentHashKeyMap.clear();
}
