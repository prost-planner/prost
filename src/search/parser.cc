#include "parser.h"

#include "planning_task.h"

#include "utils/string_utils.h"
#include "utils/system_utils.h"

using namespace std;

PlanningTask* Parser::parseTask(map<string,int>& stateVariableIndices, vector<vector<string> >& stateVariableValues) {
    string problemDesc;
    if(!SystemUtils::readFile(problemFileName, problemDesc)) {
        SystemUtils::abort("Error: Unable to read problem file: " + problemFileName);
    }

    stringstream desc(problemDesc);
    string line;

    getline(desc, line, '\n');
    assert(line == "-----TASK-----");

    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "instance name : "));
    string problemName = line.substr(16);

    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "horizon : "));
    line = line.substr(10);
    int horizon = atoi(line.c_str());

    // Get the discount factor
    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "discount factor : "));
    line = line.substr(18);
    double discountFactor = atoi(line.c_str());

    // Get the number of action fluents
    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "number of action fluents : "));
    // we currently do not use this information

    // Get the number of state  fluents (which is, atm, equivalent to the state size)
    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "number of state fluents : "));
    line = line.substr(26);
    int stateSize = atoi(line.c_str());
    std::vector<std::vector<std::pair<int, long> > > indexToStateFluentHashKeyMap(stateSize);
    std::vector<std::vector<std::pair<int, long> > > indexToKleeneStateFluentHashKeyMap(stateSize);

     // Get the number of actionPreconditions
    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "number of preconds : "));
    // we currently do not use this information

    // Get the number of action fluents
    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "number of actions : "));
    // we currently do not use this information

    // Get the index of the first probabilistic variable
    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "number of deterministic state fluents : "));
    line = line.substr(40);
    int firstProbabilisticVarIndex = atoi(line.c_str());

    // Get the number of evaluatables that have a hash index
    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "number of hashing functions : "));
    line = line.substr(30);
    int numberOfStateFluentHashKeys = atoi(line.c_str());

    // Get the initial state
    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "initial state : "));
    line = line.substr(16);
    vector<string> initialValsAsString;
    StringUtils::split(line, initialValsAsString, " ");
    vector<double> initialVals;
    for(unsigned int i = 0; i < initialValsAsString.size(); ++i) {
        initialVals.push_back(atof(initialValsAsString[i].c_str()));
    }
    State initialState(initialVals, horizon, numberOfStateFluentHashKeys);

    // Determine if the task is deterministic
    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "deterministic : "));
    line = line.substr(16);
    // we currently do not use this information
    //bool deterministic = atoi(line.c_str());

    // Determine if state hashing is possible
    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "state hashing possible : "));
    line = line.substr(25);
    bool stateHashingPossible = atoi(line.c_str());
    std::vector<std::vector<long> > hashKeyBases;
    if(stateHashingPossible) {
        hashKeyBases.resize(stateSize);
    }

    // Determine if kleene state hashing is possible
    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "kleene state hashing possible : "));
    line = line.substr(32);
    bool kleeneStateHashingPossible = atoi(line.c_str());
    std::vector<long> kleeneHashKeyBases;
    if(kleeneStateHashingPossible) {
        kleeneHashKeyBases.resize(stateSize);
    }

    // Determine if noop is always optimal as final action
    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "noop is optimal final action : "));
    line = line.substr(31);
    bool noopIsOptimalFinalAction = atoi(line.c_str());

    // Determine if the reward formula allows reward lock detection
    getline(desc, line, '\n');
    assert(StringUtils::startsWith(line, "reward formula allows reward lock detection : "));
    line = line.substr(46);
    bool rewardFormulaAllowsRewardLockDetection = atoi(line.c_str());

    // Parse fluents section
    getline(desc, line, '\n');
    assert(line == "-----FLUENTS-----");

    std::vector<StateFluent*> stateFluents;
    std::vector<ActionFluent*> actionFluents;

    std::vector<ConditionalProbabilityFunction*> CPFs;
    RewardFunction* rewardCPF = NULL;
    std::vector<Evaluatable*> actionPreconditions;

    while(getline(desc, line, '\n')) {
        if(StringUtils::startsWith(line, "action-fluent")) {
            // Get index of action fluent and assert action fluents are parsed
            // in order of their index
            line = line.substr(13);
            int index = atoi(line.c_str());
            assert(index == actionFluents.size());

            // Get name of action fluent
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "name : "));
            string name = line.substr(7);

            // Get values of state fluent
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "values : "));
            line = line.substr(9);

            vector<string> values;
            StringUtils::split(line, values, ",");

            for(unsigned int val = 0; val < values.size(); ++val) {
                assert(atoi(values[val].substr(0, values[val].find(":")-1).c_str()) == val);
                values[val] = values[val].substr(values[val].find(":")+2);
            }

            actionFluents.push_back(new ActionFluent(index, name, values));

            // Get delim line
            getline(desc, line, '\n');
            assert(line == "-----");
        } else         if(StringUtils::startsWith(line, "state-fluent")) {
            // Get index of state fluent and assert state fluents are parsed in
            // order of their index
            line = line.substr(12);
            int index = atoi(line.c_str());
            assert(index == stateFluents.size());

            // Get name of state fluent
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "name : "));
            string name = line.substr(7);

            // Get values of state fluent
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "values : "));
            line = line.substr(9);

            vector<string> values;
            StringUtils::split(line, values, ",");

            for(unsigned int val = 0; val < values.size(); ++val) {
                assert(atoi(values[val].substr(0, values[val].find(":")-1).c_str()) == val);
                values[val] = values[val].substr(values[val].find(":")+2);
            }

            // Get hash index
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "hash index : "));
            line = line.substr(13);
            int hashIndex = atoi(line.c_str());

            // Create state fluent and assigned CPF
            StateFluent* sf = new StateFluent(index, name, values);
            stateFluents.push_back(sf);
            ConditionalProbabilityFunction* cpf = new ConditionalProbabilityFunction(hashIndex, sf);
            CPFs.push_back(cpf);

            // Get caching type
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "caching type : "));
            line = line.substr(15);
            if(line == "NONE") {
                cpf->cachingType = Evaluatable::NONE;
            } else if(line == "VECTOR") {
                cpf->cachingType = Evaluatable::VECTOR;
                getline(desc, line, '\n');
                assert(StringUtils::startsWith(line, "caching vec size : "));
                line = line.substr(19);
                cpf->evaluationCacheVector.resize(atoi(line.c_str()), -numeric_limits<double>::max());
                if(index >= firstProbabilisticVarIndex) {
                    cpf->pdEvaluationCacheVector.resize(atoi(line.c_str()));
                } 
            } else if(line == "MAP") {
                cpf->cachingType = Evaluatable::MAP;
            }

            // Get kleene caching type
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "kleene caching type : "));
            line = line.substr(22);
            if(line == "NONE") {
                cpf->kleeneCachingType = Evaluatable::NONE;
            } else if(line == "VECTOR") {
                cpf->kleeneCachingType = Evaluatable::VECTOR;
                getline(desc, line, '\n');
                assert(StringUtils::startsWith(line, "kleene caching vec size : "));
                line = line.substr(26);
                cpf->kleeneEvaluationCacheVector.resize(atoi(line.c_str()));
            } else if(line == "MAP") {
                cpf->kleeneCachingType = Evaluatable::MAP;
            }

            // Get delim line
            getline(desc, line, '\n');
            assert(line == "-----");
        } else if(StringUtils::startsWith(line, "precond")) {
            // Get index of action fluent and assert action fluents are parsed
            // in order of their index
            line = line.substr(8);
            assert(atoi(line.c_str()) == actionPreconditions.size());

            string name = "Precond " + line;

            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "hash index : "));
            line = line.substr(13);
            int hashIndex = atoi(line.c_str());

            Evaluatable* precond  = new Evaluatable(name, hashIndex);
            actionPreconditions.push_back(precond);

            // Get caching type
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "caching type : "));
            line = line.substr(15);
            if(line == "NONE") {
                precond->cachingType = Evaluatable::NONE;
            } else if(line == "VECTOR") {
                precond->cachingType = Evaluatable::VECTOR;
                getline(desc, line, '\n');
                assert(StringUtils::startsWith(line, "caching vec size : "));
                line = line.substr(19);
                precond->evaluationCacheVector.resize(atoi(line.c_str()), -numeric_limits<double>::max());
            } else if(line == "MAP") {
                precond->cachingType = Evaluatable::MAP;
            }

            // Get kleene caching type
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "kleene caching type : "));
            line = line.substr(22);
            if(line == "NONE") {
                precond->kleeneCachingType = Evaluatable::NONE;
            } else if(line == "VECTOR") {
                precond->kleeneCachingType = Evaluatable::VECTOR;
                getline(desc, line, '\n');
                assert(StringUtils::startsWith(line, "kleene caching vec size : "));
                line = line.substr(26);
                precond->kleeneEvaluationCacheVector.resize(atoi(line.c_str()));
            } else if(line == "MAP") {
                precond->kleeneCachingType = Evaluatable::MAP;
            }

            // Get delim line
            getline(desc, line, '\n');
            assert(line == "-----");
        } else if(StringUtils::startsWith(line, "reward")) {
            // Get mininmal reward
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "min : "));
            double minVal = atof(line.substr(6).c_str());

            // Get maxinmal reward
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "max : "));
            double maxVal = atof(line.substr(6).c_str());

            // Get action independency
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "action independent : "));
            bool actionIndependent = atoi(line.substr(21).c_str());

            // Get hash index
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "hash index : "));
            line = line.substr(13);
            int hashIndex = atoi(line.c_str());

            rewardCPF = new RewardFunction(hashIndex, minVal, maxVal, actionIndependent);

            // Get caching type
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "caching type : "));
            line = line.substr(15);
            if(line == "NONE") {
                rewardCPF->cachingType = Evaluatable::NONE;
            } else if(line == "VECTOR") {
                rewardCPF->cachingType = Evaluatable::VECTOR;
                getline(desc, line, '\n');
                assert(StringUtils::startsWith(line, "caching vec size : "));
                line = line.substr(19);
                rewardCPF->evaluationCacheVector.resize(atoi(line.c_str()), -numeric_limits<double>::max());
            } else if(line == "MAP") {
                rewardCPF->cachingType = Evaluatable::MAP;
            }

            // Get kleene caching type
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "kleene caching type : "));
            line = line.substr(22);
            if(line == "NONE") {
                rewardCPF->kleeneCachingType = Evaluatable::NONE;
            } else if(line == "VECTOR") {
                rewardCPF->kleeneCachingType = Evaluatable::VECTOR;
                getline(desc, line, '\n');
                assert(StringUtils::startsWith(line, "kleene caching vec size : "));
                line = line.substr(26);
                rewardCPF->kleeneEvaluationCacheVector.resize(atoi(line.c_str()));
            } else if(line == "MAP") {
                rewardCPF->kleeneCachingType = Evaluatable::MAP;
            }
        } else {
            assert(line == "-----EVALUATABLES-----");
            break;
        }
    }

    // Parse function section

    while(getline(desc, line, '\n')) {
        if(StringUtils::startsWith(line, "state-fluent")) {
            // Get index of state fluent
            line = line.substr(12);
            int index = atoi(line.c_str());
            assert((index >= 0) && (index < CPFs.size()));

            // Get formula
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "orig : "));
            line = line.substr(7);
            CPFs[index]->setFormula(stateFluents, actionFluents, line);

            // Get determinization (if available) and/or delim
            getline(desc, line, '\n');
            if(StringUtils::startsWith(line, "det : ")) {
                line = line.substr(6);
                CPFs[index]->setFormula(stateFluents, actionFluents, line, true);

                getline(desc, line, '\n');
            }
            assert(line == "-----");
        } else if(StringUtils::startsWith(line, "precond")) {
            // Get index of precond
            line = line.substr(8);
            int index = atoi(line.c_str());
            assert((index >= 0) && (index < actionPreconditions.size()));

            // Get formula
            getline(desc, line, '\n');
            actionPreconditions[index]->setFormula(stateFluents, actionFluents, line);

            // Get delim line
            getline(desc, line, '\n');
            assert(line == "-----");
        } else if(StringUtils::startsWith(line, "reward")) {
            getline(desc, line, '\n');
            rewardCPF->setFormula(stateFluents, actionFluents, line);

            getline(desc, line, '\n');
            assert(line == "-----");
        } else {
            assert(line == "-----ACTION STATES-----");
            break;
        }
    }

    std::vector<ActionState> actionStates;

    // Parse legal action combinations

    while(getline(desc, line, '\n')) {
        if(StringUtils::startsWith(line, "action-state ")) {
            // Get index of action state and assert action states are parsed in
            // order of their index
            line = line.substr(13);
            int index = atoi(line.c_str());
            assert(index == actionStates.size());

            // Get values of action fluents
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "fluent values :"));
            string actionFluentValues = line.substr(16);

            // Get indices of action precondition that influence this action state
            getline(desc, line, '\n');
            string actionPreconds = "";
            if(line.length() > 10) {
                assert(StringUtils::startsWith(line, "preconds : "));
                actionPreconds = line.substr(11);
            }

            ActionState actionState(index, actionFluentValues, actionFluents, actionPreconds, actionPreconditions);
            actionStates.push_back(actionState);

            // Get delim line
            getline(desc, line, '\n');
            assert(line == "-----");
        } else {
            assert(line == "-----HASH KEYS-----");
            break;
        }
    }

   // Parse hash key section

    while(getline(desc, line, '\n')) {
        if(line == "ACTION HASH KEYS") {
            break;
        }

        assert(StringUtils::startsWith(line, "state-fluent"));
        // Get index of state fluent
        line = line.substr(12);
        int index = atoi(line.c_str());
        assert((index >= 0) && (index < CPFs.size()));

        // Get state hash keys
        if(stateHashingPossible) {
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "state hash key : "));
            line = line.substr(17);
            vector<string> stateHashKeysAsString;
            StringUtils::split(line, stateHashKeysAsString, ",");
            assert(stateHashKeysAsString.size() == CPFs[index]->getDomainSize());
            for(unsigned int i = 0; i < stateHashKeysAsString.size(); ++i) {
                hashKeyBases[index].push_back(atol(stateHashKeysAsString[i].c_str()));
            }
        }

        // Get kleene state hash key base
        if(kleeneStateHashingPossible) {
            getline(desc, line, '\n');
            assert(StringUtils::startsWith(line, "kleene state hash key base : "));
            line = line.substr(29);
            kleeneHashKeyBases[index] = atol(line.c_str());
        }

        // Get state fluent hash key bases
        getline(desc, line, '\n');
        assert(StringUtils::startsWith(line, "state fluent hash keys : "));
        line = line.substr(25);
        int numberOfStateFluentHashKeyDependencies = atoi(line.c_str());
        for(unsigned int i = 0; i < numberOfStateFluentHashKeyDependencies; ++i) {
            getline(desc, line, '\n');
            size_t cutPos = line.find(":");
            assert(cutPos != string::npos);

            int depIndex = atoi(line.substr(0, cutPos-1).c_str());
            line = line.substr(cutPos+1);
            indexToStateFluentHashKeyMap[index].push_back(make_pair(depIndex, atoi(line.c_str())));
            //vector<string> hashKeysAsString;
            //StringUtils::split(line, hashKeysAsString, ",");
            //assert(hashKeysAsString.size() == CPFs[index]->getDomainSize());
            //vector<long> hashKeys;
            //for(unsigned int j = 0; j < hashKeysAsString.size(); ++j) {
            //    hashKeys.push_back(atol(hashKeysAsString[j].c_str()));
            //}
            //indexToStateFluentHashKeyMap[index].push_back(make_pair(depIndex, hashKeys));
        }

        // Get kleene state fluent hash key bases
        getline(desc, line, '\n');
        assert(StringUtils::startsWith(line, "kleene state fluent hash keys : "));
        line = line.substr(32);
        numberOfStateFluentHashKeyDependencies = atoi(line.c_str());
        for(unsigned int i = 0; i < numberOfStateFluentHashKeyDependencies; ++i) {
            getline(desc, line, '\n');
            size_t cutPos = line.find(":");
            assert(cutPos != string::npos);

            int depIndex = atoi(line.substr(0, cutPos-1).c_str());
            line = line.substr(cutPos+1);
            long hashKey = atol(line.c_str());
            indexToKleeneStateFluentHashKeyMap[index].push_back(make_pair(depIndex, hashKey));
        }
        getline(desc, line, '\n');
        assert(line == "-----");
    }

    // Parse action hash keys
    while(getline(desc, line, '\n')) {
        Evaluatable* eval = NULL;

        if(StringUtils::startsWith(line, "state-fluent")) {
            // Get index of state fluent
            line = line.substr(12);
            int index = atoi(line.c_str());
            assert((index >= 0) && (index < CPFs.size()));

            eval = CPFs[index];
        } else if(StringUtils::startsWith(line, "precond")) {
            // Get index of state fluent
            line = line.substr(7);
            int index = atoi(line.c_str());
            assert((index >= 0) && (index < CPFs.size()));

            eval = actionPreconditions[index];
        } else if(line == "reward") {
            eval = rewardCPF;
        } else {
            assert(line == "TRAINING SET");
            break;
        }

        getline(desc, line, '\n');
        vector<string> actionValPair;
        StringUtils::split(line, actionValPair, ",");
        assert(actionValPair.size() == actionStates.size());
        eval->actionHashKeyMap.resize(actionStates.size());

        for(unsigned int i = 0; i < actionValPair.size(); ++i) {
            size_t cutPos = actionValPair[i].find(":");
            int actionIndex = atoi(actionValPair[i].substr(0,cutPos-1).c_str());
            assert(actionIndex == i);
            int val = atoi(actionValPair[i].substr(cutPos+1).c_str());
            eval->actionHashKeyMap[actionIndex] = val;
        }

        getline(desc, line, '\n');
        assert(line == "-----");
    }

    std::vector<State> trainingSet;
    while(getline(desc, line, '\n')) {
        vector<string> valuesAsString;
        StringUtils::split(line, valuesAsString);
        assert(valuesAsString.size() == stateSize);

        vector<double> values;
        for(unsigned int i = 0; i < valuesAsString.size(); ++i) {
            values.push_back(atof(valuesAsString[i].c_str()));
        }
        trainingSet.push_back(State(values, horizon, numberOfStateFluentHashKeys));
    }

    // Set mapping of variables to variable names and of values as strings to
    // internal values for communication between planner and environment

    for(unsigned int i = 0; i < CPFs.size(); ++i) {
        assert(stateVariableIndices.find(CPFs[i]->name) == stateVariableIndices.end());
        stateVariableIndices[CPFs[i]->name] = i;
        stateVariableValues.push_back(CPFs[i]->head->values);
    }

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
                            noopIsOptimalFinalAction,
                            rewardFormulaAllowsRewardLockDetection,
                            hashKeyBases,
                            kleeneHashKeyBases,
                            indexToStateFluentHashKeyMap,
                            indexToKleeneStateFluentHashKeyMap);
}
