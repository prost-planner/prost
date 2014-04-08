#include "parser.h"

#include "planning_task.h"

#include "utils/string_utils.h"
#include "utils/system_utils.h"

using namespace std;

PlanningTask* Parser::parseTask(map<string,int>& stateVariableIndices, vector<vector<string> >& stateVariableValues) {
    // Read the parser output file
    string problemDesc;
    if(!SystemUtils::readFile(problemFileName, problemDesc, "#")) {
        SystemUtils::abort("Error: Unable to read problem file: " + problemFileName);
    }
    stringstream desc(problemDesc);

    // Parse general task properties
    string problemName;
    desc >> problemName;

    int horizon;
    desc >> horizon;

    double discountFactor;
    desc >> discountFactor;

    int numberOfActionFluents;
    desc >> numberOfActionFluents;

    int numberOfDetStateFluents;
    desc >> numberOfDetStateFluents;

    int numberOfProbStateFluents;
    desc >> numberOfProbStateFluents;

    int numberOfPreconds;
    desc >> numberOfPreconds;

    int numberOfActionStates;
    desc >> numberOfActionStates;

    int numberOfStateFluentHashKeys;
    desc >> numberOfStateFluentHashKeys;

    int stateSize = numberOfDetStateFluents + numberOfProbStateFluents;

    vector<double> initialVals(stateSize, 0.0);
    for(unsigned int i = 0; i < stateSize; ++i) {
        desc >> initialVals[i];
    }
    State initialState(initialVals, horizon, numberOfStateFluentHashKeys);

    bool isDeterministic;
    desc >> isDeterministic;

    bool stateHashingPossible;
    desc >> stateHashingPossible;

    bool kleeneStateHashingPossible;
    desc >> kleeneStateHashingPossible;

    PlanningTask::FinalRewardCalculationMethod finalRewardCalculationMethod;
    vector<int> candidatesForOptimalFinalAction;
    string finalReward;
    desc >> finalReward;
    if(finalReward == "NOOP") {
        finalRewardCalculationMethod = PlanningTask::NOOP;
    } else if(finalReward == "FIRST_APPLICABLE") {
        finalRewardCalculationMethod = PlanningTask::FIRST_APPLICABLE;
    } else {
        assert(finalReward == "BEST_OF_CANDIDATE_SET");
        finalRewardCalculationMethod = PlanningTask::BEST_OF_CANDIDATE_SET;

        int sizeOfCandidateSet;
        desc >> sizeOfCandidateSet;
        candidatesForOptimalFinalAction.resize(sizeOfCandidateSet);
        for(unsigned int i = 0; i < sizeOfCandidateSet; ++i) {
            desc >> candidatesForOptimalFinalAction[i];
        }
    }

    bool rewardFormulaAllowsRewardLockDetection;
    desc >> rewardFormulaAllowsRewardLockDetection;

    // Parse action fluents
    vector<ActionFluent*> actionFluents;
    for(unsigned int i = 0; i < numberOfActionFluents; ++i) {
        parseActionFluent(desc, actionFluents);
    }

    // Parse state fluents and CPFs (Since we cannot create the CPFs before all
    // fluents have been parsed, so we collect them as strings and create them
    // later)
    vector<StateFluent*> stateFluents;
    vector<ConditionalProbabilityFunction*> CPFs;
    vector<string> formulas;
    vector<string> detFormulas;

    for(unsigned int i = 0; i < numberOfDetStateFluents; ++i) {
        ConditionalProbabilityFunction* cpf = parseCPF(desc, stateFluents, CPFs, formulas, detFormulas, false);
        parseCachingType(desc, cpf, false);
        parseActionHashKeyMap(desc, cpf, numberOfActionStates);
    }

    for(unsigned int i = 0; i < numberOfProbStateFluents; ++i) {
        ConditionalProbabilityFunction* cpf = parseCPF(desc, stateFluents, CPFs, formulas, detFormulas, true);
        parseCachingType(desc, cpf, true);
        parseActionHashKeyMap(desc, cpf, numberOfActionStates);
    }

    // All fluents have been created -> parse the CPFs and the determinized
    // formulas

    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        CPFs[i]->formula = LogicalExpression::createFromString(stateFluents, actionFluents, formulas[i]);
        if(i < numberOfDetStateFluents) {
            assert(detFormulas[i].empty());
            CPFs[i]->determinizedFormula = CPFs[i]->formula;
        } else {
            CPFs[i]->determinizedFormula = LogicalExpression::createFromString(stateFluents, actionFluents, detFormulas[i]);
        }
    }

    // Parse reward function
    RewardFunction* rewardCPF = parseRewardFunction(desc, stateFluents, actionFluents);
    parseCachingType(desc, rewardCPF, false);
    parseActionHashKeyMap(desc, rewardCPF, numberOfActionStates);

    // Parse action preconditions
    vector<Evaluatable*> actionPreconditions;

    for(unsigned int i = 0; i < numberOfPreconds; ++i) {
        Evaluatable* precond = parseActionPrecondition(desc, actionPreconditions, stateFluents, actionFluents);
        parseCachingType(desc, precond, false);
        parseActionHashKeyMap(desc, precond, numberOfActionStates);
    }

    // Parse action states
    vector<ActionState> actionStates;

    for(unsigned int i = 0; i < numberOfActionStates; ++i) {
        parseActionState(desc, actionStates, actionFluents, actionPreconditions);
    }

    // Parse hash keys
    vector<vector<long> > hashKeyBases;
    if(stateHashingPossible) {
        hashKeyBases.resize(stateSize);
    }

    vector<long> kleeneHashKeyBases;
    if(kleeneStateHashingPossible) {
        kleeneHashKeyBases.resize(stateSize);
    }

    vector<vector<pair<int, long> > > indexToStateFluentHashKeyMap(stateSize);
    vector<vector<pair<int, long> > > indexToKleeneStateFluentHashKeyMap(stateSize);

    parseHashKeys(desc, hashKeyBases, kleeneHashKeyBases,
                  indexToStateFluentHashKeyMap, indexToKleeneStateFluentHashKeyMap,
                  CPFs, stateHashingPossible, kleeneStateHashingPossible);

    // Parse training set
    vector<State> trainingSet;
    parseTrainingSet(desc, stateSize, horizon, numberOfStateFluentHashKeys, trainingSet);

    // Set mapping of variables to variable names and of values as strings to
    // internal values for communication between planner and environment
    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        assert(stateVariableIndices.find(CPFs[i]->name) == stateVariableIndices.end());
        stateVariableIndices[CPFs[i]->name] = i;
        stateVariableValues.push_back(CPFs[i]->head->values);
    }

    // Create the task
    return new PlanningTask(problemName,
                            trainingSet,
                            actionFluents,
                            actionStates, 
                            stateFluents,
                            CPFs,
                            rewardCPF,
                            actionPreconditions, 
                            initialState,
                            horizon,
                            discountFactor,
                            finalRewardCalculationMethod,
                            candidatesForOptimalFinalAction,
                            rewardFormulaAllowsRewardLockDetection,
                            hashKeyBases,
                            kleeneHashKeyBases,
                            indexToStateFluentHashKeyMap,
                            indexToKleeneStateFluentHashKeyMap);
}

void Parser::parseActionFluent(stringstream& desc, vector<ActionFluent*>& actionFluents) const {
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

    actionFluents.push_back(new ActionFluent(index, name, values));
}

ConditionalProbabilityFunction* Parser::parseCPF(stringstream& desc,
                                                 vector<StateFluent*>& stateFluents,
                                                 vector<ConditionalProbabilityFunction*>& CPFs,
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
    stateFluents.push_back(sf);

    ConditionalProbabilityFunction* cpf = new ConditionalProbabilityFunction(hashIndex, sf);
    CPFs.push_back(cpf);

    return cpf;
}

RewardFunction* Parser::parseRewardFunction(stringstream& desc, vector<StateFluent*> const& stateFluents, vector<ActionFluent*> const& actionFluents) const {
    string formulaAsString;
    desc.ignore(1, '\n');
    getline(desc, formulaAsString, '\n');
    LogicalExpression* rewardFormula = LogicalExpression::createFromString(stateFluents, actionFluents, formulaAsString);

    double minVal;
    desc >> minVal;

    double maxVal;
    desc >> maxVal;

    int hashIndex;
    desc >> hashIndex;

    return new RewardFunction(hashIndex, minVal, maxVal, rewardFormula);
}

Evaluatable* Parser::parseActionPrecondition(stringstream& desc,
                                             vector<Evaluatable*>& actionPreconditions,
                                             vector<StateFluent*> const& stateFluents,
                                             vector<ActionFluent*> const& actionFluents) const {
    int index;
    desc >> index;

    stringstream name;
    name << "Precond " << index;

    string formulaAsString;
    desc.ignore(1, '\n');
    getline(desc, formulaAsString, '\n');

    LogicalExpression* formula = LogicalExpression::createFromString(stateFluents, actionFluents, formulaAsString);

    int hashIndex;
    desc >> hashIndex;

    Evaluatable* precond = new Evaluatable(name.str(), hashIndex, formula, formula);

    assert(actionPreconditions.size() == index);
    actionPreconditions.push_back(precond);
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

void Parser::parseActionHashKeyMap(stringstream& desc, Evaluatable* eval, int const& numberOfActionStates) const {
    eval->actionHashKeyMap.resize(numberOfActionStates);
    for(unsigned int i = 0; i < numberOfActionStates; ++i) {
        int actionIndex;
        desc >> actionIndex;
        assert(actionIndex == i);
        desc >> eval->actionHashKeyMap[i];
    }
}

void Parser::parseActionState(stringstream& desc,
                              vector<ActionState>& actionStates,
                              vector<ActionFluent*> const& actionFluents,
                              vector<Evaluatable*> const& actionPreconditions) const {
    int index;
    desc >> index;

    vector<int> values(actionFluents.size());
    vector<ActionFluent*> scheduledActionFluents;

    for(unsigned int j = 0; j < actionFluents.size(); ++j) {
        desc >> values[j];
        if(values[j] == 1) {
            scheduledActionFluents.push_back(actionFluents[j]);
        }
    }

    int numberOfRelevantPreconditions;
    desc >> numberOfRelevantPreconditions;
    vector<Evaluatable*> relevantPreconditions(numberOfRelevantPreconditions);

    for(unsigned int j = 0; j < numberOfRelevantPreconditions; ++j) {
        int precondIndex;
        desc >> precondIndex;
        relevantPreconditions[j] = actionPreconditions[precondIndex];
    }

    actionStates.push_back(ActionState(index, values, scheduledActionFluents, relevantPreconditions));
}

void Parser::parseHashKeys(stringstream& desc,
                           vector<vector<long> >& hashKeyBases,
                           vector<long>& kleeneHashKeyBases,
                           vector<vector<pair<int, long> > >& indexToStateFluentHashKeyMap,
                           vector<vector<pair<int, long> > >& indexToKleeneStateFluentHashKeyMap,
                           vector<ConditionalProbabilityFunction*> const& CPFs,
                           bool const& stateHashingPossible,
                           bool const& kleeneStateHashingPossible) const {
    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        int index;
        desc >> index;
        assert(index == i);

        if(stateHashingPossible) {
            hashKeyBases[index].resize(CPFs[index]->head->values.size());
            for(unsigned int j = 0; j < CPFs[index]->head->values.size(); ++j) {
                desc >> hashKeyBases[index][j];
            }
        }

        if(kleeneStateHashingPossible) {
            desc >> kleeneHashKeyBases[index];
        }

        int numberOfKeys;
        desc >> numberOfKeys;
        for(unsigned int j = 0; j < numberOfKeys; ++j) {
            int var;
            long key;
            desc >> var >> key;
            indexToStateFluentHashKeyMap[index].push_back(make_pair(var, key));
        }

        desc >> numberOfKeys;
        for(unsigned int j = 0; j < numberOfKeys; ++j) {
            int var;
            long key;
            desc >> var >> key;
            indexToKleeneStateFluentHashKeyMap[index].push_back(make_pair(var, key));
        }
    }
}

void Parser::parseTrainingSet(stringstream& desc,
                              int const& stateSize,
                              int const& horizon,
                              int const& numberOfStateFluentHashKeys,
                              vector<State>& trainingSet) const {
    int numberOfTrainingStates;
    desc >> numberOfTrainingStates;
    for(unsigned int i = 0; i < numberOfTrainingStates; ++i) {
        vector<double> values(stateSize);
        for(unsigned int j = 0; j < stateSize; ++j) {
            desc >> values[j];
        }
        trainingSet.push_back(State(values, horizon, numberOfStateFluentHashKeys));
    }
}
