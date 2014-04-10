#include "parser.h"

#include "planning_task.h"
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
    desc >> PlanningTask::name;
    desc >> PlanningTask::horizon;
    desc >> PlanningTask::discountFactor;

    // Parse relevant numbers
    int numberOfActionFluents;
    desc >> numberOfActionFluents;

    int numberOfDetStateFluents;
    desc >> numberOfDetStateFluents;
    PlanningTask::firstProbabilisticVarIndex = numberOfDetStateFluents;

    int numberOfProbStateFluents;
    desc >> numberOfProbStateFluents;

    int numberOfPreconds;
    desc >> numberOfPreconds;

    desc >> PlanningTask::numberOfActions;
    desc >> State::numberOfStateFluentHashKeys;
    KleeneState::numberOfStateFluentHashKeys = State::numberOfStateFluentHashKeys;

    State::stateSize = numberOfDetStateFluents + numberOfProbStateFluents;
    KleeneState::stateSize = State::stateSize;

    // Parse initial state
    vector<double> initialVals(State::stateSize, 0.0);
    for(unsigned int i = 0; i < State::stateSize; ++i) {
        desc >> initialVals[i];
    }
    PlanningTask::initialState = State(initialVals, PlanningTask::horizon);

    // Parse task properties
    desc >> PlanningTask::isDeterministic;
    desc >> State::stateHashingPossible;
    desc >> KleeneState::stateHashingPossible;

    string finalReward;
    desc >> finalReward;
    if(finalReward == "NOOP") {
        PlanningTask::finalRewardCalculationMethod = PlanningTask::NOOP;
    } else if(finalReward == "FIRST_APPLICABLE") {
        PlanningTask::finalRewardCalculationMethod = PlanningTask::FIRST_APPLICABLE;
    } else {
        assert(finalReward == "BEST_OF_CANDIDATE_SET");
        PlanningTask::finalRewardCalculationMethod = PlanningTask::BEST_OF_CANDIDATE_SET;

        int sizeOfCandidateSet;
        desc >> sizeOfCandidateSet;
        PlanningTask::candidatesForOptimalFinalAction.resize(sizeOfCandidateSet);
        for(unsigned int i = 0; i < sizeOfCandidateSet; ++i) {
            desc >> PlanningTask::candidatesForOptimalFinalAction[i];
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
        ConditionalProbabilityFunction* cpf = parseCPF(desc, formulas, detFormulas, false);
        parseCachingType(desc, cpf, false);
        parseActionHashKeyMap(desc, cpf);
    }

    for(unsigned int i = 0; i < numberOfProbStateFluents; ++i) {
        ConditionalProbabilityFunction* cpf = parseCPF(desc, formulas, detFormulas, true);
        parseCachingType(desc, cpf, true);
        parseActionHashKeyMap(desc, cpf);
    }

    // All fluents have been created -> create the CPF formulas
    for(unsigned int i = 0; i < PlanningTask::CPFs.size(); ++i) {
        PlanningTask::CPFs[i]->formula = LogicalExpression::createFromString(PlanningTask::stateFluents, PlanningTask::actionFluents, formulas[i]);
        if(i < numberOfDetStateFluents) {
            assert(detFormulas[i].empty());
            PlanningTask::CPFs[i]->determinizedFormula = PlanningTask::CPFs[i]->formula;
        } else {
            PlanningTask::CPFs[i]->determinizedFormula = LogicalExpression::createFromString(PlanningTask::stateFluents, PlanningTask::actionFluents, detFormulas[i]);
        }
    }

    // Parse reward function
    PlanningTask::rewardCPF = parseRewardFunction(desc);
    parseCachingType(desc, PlanningTask::rewardCPF, false);
    parseActionHashKeyMap(desc, PlanningTask::rewardCPF);

    // Parse action preconditions
    vector<Evaluatable*> actionPreconditions;

    for(unsigned int i = 0; i < numberOfPreconds; ++i) {
        Evaluatable* precond = parseActionPrecondition(desc);
        parseCachingType(desc, precond, false);
        parseActionHashKeyMap(desc, precond);
    }

    // Parse action states
    for(unsigned int i = 0; i < PlanningTask::numberOfActions; ++i) {
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
    State::calcStateFluentHashKeys(PlanningTask::initialState);
    State::calcStateHashKey(PlanningTask::initialState);

    // Parse training set
    parseTrainingSet(desc);

    // Set mapping of variables to variable names and of values as strings to
    // internal values for communication between planner and environment
    for(unsigned int i = 0; i < PlanningTask::CPFs.size(); ++i) {
        assert(stateVariableIndices.find(PlanningTask::CPFs[i]->name) == stateVariableIndices.end());
        stateVariableIndices[PlanningTask::CPFs[i]->name] = i;
        stateVariableValues.push_back(PlanningTask::CPFs[i]->head->values);
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

    PlanningTask::actionFluents.push_back(new ActionFluent(index, name, values));
}

ConditionalProbabilityFunction* Parser::parseCPF(stringstream& desc,
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
    PlanningTask::stateFluents.push_back(sf);

    ConditionalProbabilityFunction* cpf = new ConditionalProbabilityFunction(hashIndex, sf);
    PlanningTask::CPFs.push_back(cpf);

    return cpf;
}

RewardFunction* Parser::parseRewardFunction(stringstream& desc) const {
    string formulaAsString;
    desc.ignore(1, '\n');
    getline(desc, formulaAsString, '\n');
    LogicalExpression* rewardFormula = LogicalExpression::createFromString(PlanningTask::stateFluents, PlanningTask::actionFluents, formulaAsString);

    double minVal;
    desc >> minVal;

    double maxVal;
    desc >> maxVal;

    int hashIndex;
    desc >> hashIndex;

    return new RewardFunction(hashIndex, minVal, maxVal, rewardFormula);
}

Evaluatable* Parser::parseActionPrecondition(stringstream& desc) const {
    int index;
    desc >> index;

    stringstream name;
    name << "Precond " << index;

    string formulaAsString;
    desc.ignore(1, '\n');
    getline(desc, formulaAsString, '\n');

    LogicalExpression* formula = LogicalExpression::createFromString(PlanningTask::stateFluents, PlanningTask::actionFluents, formulaAsString);

    int hashIndex;
    desc >> hashIndex;

    Evaluatable* precond = new Evaluatable(name.str(), hashIndex, formula, formula);

    assert(PlanningTask::actionPreconditions.size() == index);
    PlanningTask::actionPreconditions.push_back(precond);
    return precond;
}

void Parser::parseCachingType(stringstream& desc, Evaluatable* eval, bool const& isProbabilistic) const {
    string cachingType;
    desc >> cachingType;

    if(cachingType == "NONE") {
        eval->cachingType = Evaluatable::NONE;
    } else if(cachingType == "VECTOR") {
        eval->cachingType = Evaluatable::VECTOR;

        int cachingVecSize;
        desc >> cachingVecSize;

        if(isProbabilistic) {
            eval->evaluationCacheVector.resize(cachingVecSize, -numeric_limits<double>::max());
            eval->pdEvaluationCacheVector.resize(cachingVecSize);

            for(unsigned int i = 0; i < cachingVecSize; ++i) {
                int key;
                desc >> key;
                assert(key == i);
                desc >> eval->evaluationCacheVector[i];

                int sizeOfPD;
                desc >> sizeOfPD;

                eval->pdEvaluationCacheVector[i].values.resize(sizeOfPD);
                eval->pdEvaluationCacheVector[i].probabilities.resize(sizeOfPD);
                for(unsigned int j = 0; j < sizeOfPD; ++j) {
                    desc >> eval->pdEvaluationCacheVector[i].values[j];
                    desc >> eval->pdEvaluationCacheVector[i].probabilities[j];
                }
                assert(eval->pdEvaluationCacheVector[i].isWellDefined());
            }
        } else {
            eval->evaluationCacheVector.resize(cachingVecSize, -numeric_limits<double>::max());
            for(unsigned int i = 0; i < cachingVecSize; ++i) {
                int key;
                desc >> key;
                assert(key == i);
                desc >> eval->evaluationCacheVector[i];
            }
        }
    } else if(cachingType == "MAP") {
        eval->cachingType = Evaluatable::MAP;
    }

    desc >> cachingType;
    if(cachingType == "NONE") {
        eval->kleeneCachingType = Evaluatable::NONE;
    } else if(cachingType == "VECTOR") {
        eval->kleeneCachingType = Evaluatable::VECTOR;

        int cachingVecSize;
        desc >> cachingVecSize;
        eval->kleeneEvaluationCacheVector.resize(cachingVecSize);
    } else if(cachingType == "MAP") {
        eval->kleeneCachingType = Evaluatable::MAP;
    }
}

void Parser::parseActionHashKeyMap(stringstream& desc, Evaluatable* eval) const {
    eval->actionHashKeyMap.resize(PlanningTask::numberOfActions);
    for(unsigned int i = 0; i < PlanningTask::numberOfActions; ++i) {
        int actionIndex;
        desc >> actionIndex;
        assert(actionIndex == i);
        desc >> eval->actionHashKeyMap[i];
    }
}

void Parser::parseActionState(stringstream& desc) const {
    int index;
    desc >> index;

    vector<int> values(PlanningTask::actionFluents.size());
    vector<ActionFluent*> scheduledActionFluents;

    for(unsigned int j = 0; j < PlanningTask::actionFluents.size(); ++j) {
        desc >> values[j];
        if(values[j] == 1) {
            scheduledActionFluents.push_back(PlanningTask::actionFluents[j]);
        }
    }

    int numberOfRelevantPreconditions;
    desc >> numberOfRelevantPreconditions;
    vector<Evaluatable*> relevantPreconditions(numberOfRelevantPreconditions);

    for(unsigned int j = 0; j < numberOfRelevantPreconditions; ++j) {
        int precondIndex;
        desc >> precondIndex;
        relevantPreconditions[j] = PlanningTask::actionPreconditions[precondIndex];
    }

    PlanningTask::actionStates.push_back(ActionState(index, values, scheduledActionFluents, relevantPreconditions));
}

void Parser::parseHashKeys(stringstream& desc) const {
    for(unsigned int i = 0; i < PlanningTask::CPFs.size(); ++i) {
        int index;
        desc >> index;
        assert(index == i);

        if(State::stateHashingPossible) {
            State::stateHashKeys[index].resize(PlanningTask::CPFs[index]->head->values.size());
            for(unsigned int j = 0; j < PlanningTask::CPFs[index]->head->values.size(); ++j) {
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
        State trainingState(values, PlanningTask::horizon);
        State::calcStateFluentHashKeys(trainingState);
        State::calcStateHashKey(trainingState);
        PlanningTask::trainingSet.push_back(trainingState);
    }
}
