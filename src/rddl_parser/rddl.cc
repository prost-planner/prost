#include "rddl.h"

#include "evaluatables.h"
#include "logical_expressions.h"

#include "utils/math_utils.h"
#include "utils/string_utils.h"
#include "utils/system_utils.h"
#include "utils/timer.h"

#include "z3++.h"

#include <iostream>

using namespace std;

/*****************************************************************
                           RDDL Block
*****************************************************************/
RDDLTask::RDDLTask()
    : numberOfConcurrentActions(numeric_limits<int>::max()),
      horizon(1),
      discountFactor(1.0),
      rewardCPF(nullptr),
      rewardLockDetected(false),
      unreasonableActionDetected(false),
      unreasonableActionInDeterminizationDetected(false),
      numberOfEncounteredStates(0),
      numberOfUniqueEncounteredStates(0),
      nonTerminalStatesWithUniqueAction(0),
      uniqueNonTerminalStatesWithUniqueAction(0) {
    // Add bool type
    addType("bool");
    addObject("bool", "false");
    addObject("bool", "true");

    // Add numerical types
    addType("int");
    addType("real");

    // Add object super type
    addType("object");
}

void RDDLTask::addCPF(ParametrizedVariable variable,
                      LogicalExpression* logicalExpression) {
    // Variable
    string name = variable.variableName;
    if (name[name.length() - 1] == '\'') {
        name = name.substr(0, name.length() - 1);
    }

    if (variableDefinitions.find(name) == variableDefinitions.end()) {
        SystemUtils::abort("No according variable to CPF " + name + ".");
    }

    ParametrizedVariable* head = variableDefinitions[name];

    if (variable.params.size() != head->params.size()) {
        SystemUtils::abort(
            "Wrong number of parameters for parametrized variable " + name +
            ".");
    }

    for (int i = 0; i < variable.params.size(); ++i) {
        head->params[i]->name = variable.params[i]->name;
    }

    if (CPFDefinitions.find(head) != CPFDefinitions.end()) {
        SystemUtils::abort("Error: Multiple definition of CPF " + name + ".");
    }
    // Expression
    CPFDefinitions[head] = logicalExpression;
}

void RDDLTask::setInstance(string name, string domainName,
                           string nonFluentsName, int maxNonDefActions,
                           int horizon, double discount) {
    this->name = name;

    // Check domain name
    if (this->domainName != domainName) {
        SystemUtils::abort("Unknown domain " + domainName +
                           " defined in Instance section");
    }

    // Check Non fluents name
    if (this->nonFluentsName != nonFluentsName) {
        SystemUtils::abort("Unknown non fluents " + nonFluentsName +
                           "defined in Non fluent section");
    }

    // Set Max nondef actions
    this->numberOfConcurrentActions = maxNonDefActions;

    // Set horizon
    this->horizon = horizon;

    // Set discount
    this->discountFactor = discount;
}

void RDDLTask::addType(string const& name, string const& superType) {
    if (types.find(name) != types.end()) {
        SystemUtils::abort("Error: Type " + name + " is ambiguous.");
    }

    if (superType.empty()) {
        types[name] = new Type(name);
    } else if (types.find(superType) == types.end()) {
        SystemUtils::abort("Error: Supertype not found: " + superType);
    } else {
        types[name] = new Type(name, types[superType]);
    }
}

Type* RDDLTask::getType(string typeName) {
    if (types.find(typeName) != types.end()) {
        return types[typeName];
    }
    return nullptr;
}

void RDDLTask::addObject(string const& typeName,
                         string const& objectName) {
    if (types.find(typeName) == types.end()) {
        SystemUtils::abort("Error: Type " + typeName + " not defined.");
    }

    if (objects.find(objectName) != objects.end()) {
        SystemUtils::abort("Error: Object name " + objectName +
                           " is ambiguous.");
    }

    objects[objectName] = new Object(objectName, types[typeName]);
}

Object* RDDLTask::getObject(string objName) {
    if (objects.find(objName) != objects.end()) {
        return objects[objName];
    }
    return nullptr;
}

void RDDLTask::addVariableSchematic(ParametrizedVariable* varDef) {
    if (variableDefinitions.find(varDef->fullName) !=
        variableDefinitions.end()) {
        SystemUtils::abort("Error: Ambiguous variable name: " +
                           varDef->fullName);
    }
    variableDefinitions[varDef->fullName] = varDef;
}

void RDDLTask::addParametrizedVariable(ParametrizedVariable* parent,
                                       vector<Parameter*> const& params) {
    addParametrizedVariable(parent, params, parent->initialValue);
}

void RDDLTask::addParametrizedVariable(ParametrizedVariable* parent,
                                       vector<Parameter*> const& params,
                                       double initialValue) {
    if (variableDefinitions.find(parent->variableName) ==
        variableDefinitions.end()) {
        SystemUtils::abort("Error: Parametrized variable " +
                           parent->variableName + " not defined.");
    }

    switch (parent->variableType) {
    case ParametrizedVariable::STATE_FLUENT: {
        StateFluent* sf = new StateFluent(*parent, params, initialValue);

        // This is already defined if it occurs in the initial state entry
        if (stateFluentMap.find(sf->fullName) != stateFluentMap.end()) {
            return;
        }

        stateFluents.push_back(sf);
        stateFluentMap[sf->fullName] = sf;

        if (stateFluentsBySchema.find(parent) == stateFluentsBySchema.end()) {
            stateFluentsBySchema[parent] = vector<StateFluent*>();
        }
        stateFluentsBySchema[parent].push_back(sf);
        break;
    }
    case ParametrizedVariable::ACTION_FLUENT: {
        ActionFluent* af = new ActionFluent(*parent, params);

        assert(actionFluentMap.find(af->fullName) == actionFluentMap.end());

        af->index = actionFluents.size();
        actionFluents.push_back(af);
        actionFluentMap[af->fullName] = af;
        break;
    }
    case ParametrizedVariable::NON_FLUENT: {
        NonFluent* nf = new NonFluent(*parent, params, initialValue);

        // This is already defined if it occurs in the non fluents entry
        if (nonFluentMap.find(nf->fullName) != nonFluentMap.end()) {
            return;
        }

        nonFluents.push_back(nf);
        nonFluentMap[nf->fullName] = nf;
        break;
    }
        // case ParametrizedVariable::INTERM_FLUENT:
        // assert(false);
        // break;
    }
}

ParametrizedVariable* RDDLTask::getParametrizedVariable(string varName) {
    if (variableDefinitions.find(varName) != variableDefinitions.end()) {
        return variableDefinitions[varName];
    } else {
        SystemUtils::abort("Unknown variable: " + varName + ".");
    }
    return nullptr;
}

StateFluent* RDDLTask::getStateFluent(string const& name) {
    if (stateFluentMap.find(name) == stateFluentMap.end()) {
        SystemUtils::abort("Error: state-fluent " + name +
                           " used but not defined.");
        return nullptr;
    }
    return stateFluentMap[name];
}

ActionFluent* RDDLTask::getActionFluent(string const& name) {
    if (actionFluentMap.find(name) == actionFluentMap.end()) {
        SystemUtils::abort("Error: action-fluent " + name +
                           " used but not defined.");
        return nullptr;
    }
    return actionFluentMap[name];
}

NonFluent* RDDLTask::getNonFluent(string const& name) {
    if (nonFluentMap.find(name) == nonFluentMap.end()) {
        SystemUtils::abort("Error: non-fluent " + name +
                           " used but not defined.");
        return nullptr;
    }
    return nonFluentMap[name];
}

// TODO: Return const reference?
vector<StateFluent*> RDDLTask::getStateFluentsOfSchema(
    ParametrizedVariable* schema) {
    assert(stateFluentsBySchema.find(schema) != stateFluentsBySchema.end());
    return stateFluentsBySchema[schema];
}

void RDDLTask::setRewardCPF(LogicalExpression* const& rewardFormula) {
    if (rewardCPF) {
        SystemUtils::abort("Error: RewardCPF exists already.");
    }
    rewardCPF = new RewardFunction(rewardFormula);
}

void RDDLTask::buildCSP(z3::context& c, z3::solver& s,
                        vector<z3::expr>& sf_exprs,
                        vector<z3::expr>& af_exprs,
                        bool addSACs) const {
    stringstream ss;
    for (ConditionalProbabilityFunction* cpf : CPFs) {
        ss.str("");
        StateFluent* sf = cpf->head;
        ss << "sf_" << sf->index << "_" << sf->fullName;
        int domainSize = sf->valueType->objects.size();
        sf_exprs.push_back(c.int_const(ss.str().c_str()));
        s.add(sf_exprs.back() >= 0);
        s.add(sf_exprs.back() < domainSize);
    }

    for (ActionFluent* af : actionFluents) {
        ss.str("");
        ss << "af_" << af->index << "_" << af->fullName;
        int domainSize = af->valueType->objects.size();
        af_exprs.push_back(c.int_const(ss.str().c_str()));
        s.add(af_exprs.back() >= 0);
        s.add(af_exprs.back() < domainSize);
        ss.str("");
    }

    if (addSACs) {
        for (LogicalExpression* sac : SACs) {
            // Only boolean valued formulas can be added, but toZ3Formula returns a
            // number. The number is converted to a boolean by comparing to 0
            s.add(sac->toZ3Formula(c, sf_exprs, af_exprs) != 0);
        }
    }

    // The value provided by max-nondef-actions is also a constraint that must
    // be taken into account. It is first modeled as a LogicalExpression and
    // then converted to a z3::expr in the same way preconditions are.
    int numActionFluents = actionFluents.size();
    int numConcurrentActions = numberOfConcurrentActions;
    if (numberOfConcurrentActions < numActionFluents) {
        vector<LogicalExpression*> afs;
        for (ActionFluent* af : actionFluents) {
            afs.push_back(af);
        }
        vector<LogicalExpression*> maxConcurrent =
            {new Addition(afs), new NumericConstant(numConcurrentActions)};
        LowerEqualsExpression* constraint =
            new LowerEqualsExpression(maxConcurrent);
        s.add(constraint->toZ3Formula(c, sf_exprs, af_exprs) != 0);
    }

    // We could even assert that there is an applicable action in the initial
    // state (i.e., we could set all state fluents to their values in the
    // initial state), but the assertion below also holds (it basically says
    // there must be a state with at least one applicable action).
    assert(s.check() == z3::sat);
}

void RDDLTask::print(ostream& out) {
    // Set precision of doubles
    out.unsetf(ios::floatfield);
    out.precision(numeric_limits<double>::digits10);

    int firstProbabilisticVarIndex = (int)CPFs.size();
    bool deterministic = true;
    for (unsigned int i = 0; i < CPFs.size(); ++i) {
        if (CPFs[i]->isProbabilistic()) {
            firstProbabilisticVarIndex = i;
            deterministic = false;
            break;
        }
    }

    out << "#####TASK#####" << endl;
    out << "## name" << endl;
    out << name << endl;
    out << "## horizon" << endl;
    out << horizon << endl;
    out << "## discount factor" << endl;
    out << discountFactor << endl;
    out << "## number of action fluents" << endl;
    out << actionFluents.size() << endl;
    out << "## number of det state fluents" << endl;
    out << firstProbabilisticVarIndex << endl;
    out << "## number of prob state fluents" << endl;
    out << (CPFs.size() - firstProbabilisticVarIndex) << endl;
    out << "## number of preconds" << endl;
    out << actionPreconds.size() << endl;
    out << "## number of actions" << endl;
    out << actionStates.size() << endl;
    out << "## number of hashing functions" << endl;
    out << (actionPreconds.size() + CPFs.size() + 1) << endl;
    out << "## initial state" << endl;
    for (unsigned int i = 0; i < CPFs.size(); ++i) {
        out << CPFs[i]->getInitialValue() << " ";
    }
    out << endl;
    out << "## 1 if task is deterministic" << endl;
    out << deterministic << endl;
    out << "## 1 if state hashing possible" << endl;
    out << !stateHashKeys.empty() << endl;
    out << "## 1 if kleene state hashing possible" << endl;
    out << !kleeneStateHashKeyBases.empty() << endl;
    out << "## method to calculate the final reward" << endl;
    out << finalRewardCalculationMethod << endl;
    if (finalRewardCalculationMethod == "BEST_OF_CANDIDATE_SET") {
        out << "## set of candidates to calculate final reward (first line is "
               "the number)"
            << endl;
        out << candidatesForOptimalFinalAction.size() << endl;
        for (unsigned int i = 0; i < candidatesForOptimalFinalAction.size();
             ++i) {
            out << candidatesForOptimalFinalAction[i] << " ";
        }
        out << endl;
    }
    out << "## 1 if reward formula allows reward lock detection and a reward "
           "lock was found during task analysis"
        << endl;
    out << rewardLockDetected << endl;
    out << "## 1 if an unreasonable action was detected" << endl;
    out << unreasonableActionDetected << endl;
    out << "## 1 if an unreasonable action was detected in the determinization"
        << endl;
    out << unreasonableActionInDeterminizationDetected << endl;

    out << "## number of states that were encountered during task analysis"
        << endl;
    out << numberOfEncounteredStates << endl;
    out << "## number of unique states that were encountered during task "
           "analysis"
        << endl;
    out << numberOfUniqueEncounteredStates << endl;
    out << "## number of states with only one applicable reasonable action "
           "that were encountered during task analysis"
        << endl;
    out << nonTerminalStatesWithUniqueAction << endl;
    out << "## number of unique states with only one applicable reasonable "
           "action that were encountered during task analysis"
        << endl;
    out << uniqueNonTerminalStatesWithUniqueAction << endl;

    out << endl << endl << "#####ACTION FLUENTS#####" << endl;
    for (ActionFluent* af : actionFluents) {
        out << "## index" << endl;
        out << af->index << endl;
        out << "## name" << endl;
        out << af->fullName << endl;
        out << "## number of values" << endl;
        vector<Object*>& values = af->valueType->objects;
        int numValues = values.size();
        out << numValues << endl;
        out << "## values" << endl;
        if (numValues == 2) {
            out << "0 false" << endl << "1 true" << endl;
        } else {
            for (Object* value : values) {
                out << value->value << " " << value->name << endl;
            }
        }
        out << endl;
    }

    out << endl
        << endl
        << "#####DET STATE FLUENTS AND CPFS#####" << endl;
    for (unsigned int index = 0; index < firstProbabilisticVarIndex; ++index) {
        assert(CPFs[index]->head->index == index);
        assert(!CPFs[index]->isProbabilistic());
        out << "## index" << endl;
        out << index << endl;
        out << "## name" << endl;
        out << CPFs[index]->head->fullName << endl;
        out << "## number of values" << endl;
        out << CPFs[index]->domain.size() << endl;
        out << "## values" << endl;
        for (set<double>::iterator it = CPFs[index]->domain.begin();
             it != CPFs[index]->domain.end(); ++it) {
            out << *it << " "
                << CPFs[index]->head->valueType->objects[*it]->name
                << endl;
        }

        out << "## formula" << endl;
        CPFs[index]->formula->print(out);
        out << endl;

        out << "## hash index" << endl;
        out << CPFs[index]->hashIndex << endl;
        out << "## caching type " << endl;
        out << CPFs[index]->cachingType << endl;
        if (CPFs[index]->cachingType == "VECTOR") {
            out << "## precomputed results" << endl;
            out << CPFs[index]->precomputedResults.size() << endl;
            for (unsigned int res = 0;
                 res < CPFs[index]->precomputedResults.size(); ++res) {
                out << res << " " << CPFs[index]->precomputedResults[res]
                    << endl;
            }
        }
        out << "## kleene caching type" << endl;
        out << CPFs[index]->kleeneCachingType << endl;
        if (CPFs[index]->kleeneCachingType == "VECTOR") {
            out << "## kleene caching vec size" << endl;
            out << CPFs[index]->kleeneCachingVectorSize << endl;
        }

        out << "## action hash keys" << endl;
        for (unsigned int actionIndex = 0;
             actionIndex < CPFs[index]->actionHashKeyMap.size();
             ++actionIndex) {
            out << actionIndex << " "
                << CPFs[index]->actionHashKeyMap[actionIndex] << endl;
        }
        out << endl;
    }

    out << endl
        << endl
        << "#####PROB STATE FLUENTS AND CPFS#####" << endl;
    for (unsigned int index = firstProbabilisticVarIndex; index < CPFs.size();
         ++index) {
        assert(CPFs[index]->head->index == index);
        assert(CPFs[index]->isProbabilistic());
        out << "## index" << endl;
        out << (index - firstProbabilisticVarIndex) << endl;
        out << "## name" << endl;
        out << CPFs[index]->head->fullName << endl;
        out << "## number of values" << endl;
        out << CPFs[index]->domain.size() << endl;
        out << "## values" << endl;
        for (set<double>::iterator it = CPFs[index]->domain.begin();
             it != CPFs[index]->domain.end(); ++it) {
            out << *it << " "
                << CPFs[index]->head->valueType->objects[*it]->name
                << endl;
        }

        out << "## formula" << endl;
        CPFs[index]->formula->print(out);
        out << endl;

        out << "## determinized formula" << endl;
        CPFs[index]->determinization->print(out);
        out << endl;

        out << "## hash index" << endl;
        out << CPFs[index]->hashIndex << endl;
        out << "## caching type " << endl;
        out << CPFs[index]->cachingType << endl;
        if (CPFs[index]->cachingType == "VECTOR") {
            out << "## precomputed results (key - determinization - size of "
                   "distribution - value-probability pairs)"
                << endl;
            out << CPFs[index]->precomputedResults.size() << endl;
            for (unsigned int res = 0;
                 res < CPFs[index]->precomputedResults.size(); ++res) {
                out << res << " " << CPFs[index]->precomputedResults[res] << " "
                    << CPFs[index]->precomputedPDResults[res].values.size();
                for (unsigned int valProbPair = 0;
                     valProbPair <
                     CPFs[index]->precomputedPDResults[res].values.size();
                     ++valProbPair) {
                    out << " "
                        << CPFs[index]
                               ->precomputedPDResults[res]
                               .values[valProbPair]
                        << " "
                        << CPFs[index]
                               ->precomputedPDResults[res]
                               .probabilities[valProbPair];
                }
                out << endl;
            }
        }
        out << "## kleene caching type" << endl;
        out << CPFs[index]->kleeneCachingType << endl;
        if (CPFs[index]->kleeneCachingType == "VECTOR") {
            out << "## kleene caching vec size" << endl;
            out << CPFs[index]->kleeneCachingVectorSize << endl;
        }

        out << "## action hash keys" << endl;
        for (unsigned int actionIndex = 0;
             actionIndex < CPFs[index]->actionHashKeyMap.size();
             ++actionIndex) {
            out << actionIndex << " "
                << CPFs[index]->actionHashKeyMap[actionIndex] << endl;
        }

        out << endl;
    }

    out << endl << endl << "#####REWARD#####" << endl;
    out << "## formula" << endl;
    rewardCPF->formula->print(out);
    out << endl;
    out << "## min" << endl;
    out << *rewardCPF->domain.begin() << endl;
    out << "## max" << endl;
    out << *rewardCPF->domain.rbegin() << endl;
    out << "## independent from actions" << endl;
    out << (rewardCPF->positiveActionDependencies.empty() &&
            rewardCPF->negativeActionDependencies.empty())
        << endl;
    out << "## hash index" << endl;
    out << rewardCPF->hashIndex << endl;
    out << "## caching type" << endl;
    out << rewardCPF->cachingType << endl;
    if (rewardCPF->cachingType == "VECTOR") {
        out << "## precomputed results" << endl;
        out << rewardCPF->precomputedResults.size() << endl;
        for (unsigned int res = 0; res < rewardCPF->precomputedResults.size();
             ++res) {
            out << res << " " << rewardCPF->precomputedResults[res]
                << endl;
        }
    }
    out << "## kleene caching type" << endl;
    out << rewardCPF->kleeneCachingType << endl;
    if (rewardCPF->kleeneCachingType == "VECTOR") {
        out << "## kleene caching vec size" << endl;
        out << rewardCPF->kleeneCachingVectorSize << endl;
    }

    out << "## action hash keys" << endl;
    for (unsigned int actionIndex = 0;
         actionIndex < rewardCPF->actionHashKeyMap.size(); ++actionIndex) {
        out << actionIndex << " " << rewardCPF->actionHashKeyMap[actionIndex]
            << endl;
    }

    out << endl << endl << "#####PRECONDITIONS#####" << endl;

    for (unsigned int index = 0; index < actionPreconds.size(); ++index) {
        assert(actionPreconds[index]->index == index);
        out << "## index" << endl;
        out << index << endl;
        out << "## formula" << endl;
        actionPreconds[index]->formula->print(out);
        out << endl;
        out << "## hash index" << endl;
        out << actionPreconds[index]->hashIndex << endl;
        out << "## caching type" << endl;
        out << actionPreconds[index]->cachingType << endl;
        if (actionPreconds[index]->cachingType == "VECTOR") {
            out << "## precomputed results" << endl;
            out << actionPreconds[index]->precomputedResults.size()
                << endl;
            for (unsigned int res = 0;
                 res < actionPreconds[index]->precomputedResults.size();
                 ++res) {
                out << res << " "
                    << actionPreconds[index]->precomputedResults[res]
                    << endl;
            }
        }
        out << "## kleene caching type" << endl;
        out << actionPreconds[index]->kleeneCachingType << endl;
        if (actionPreconds[index]->kleeneCachingType == "VECTOR") {
            out << "## kleene caching vec size" << endl;
            out << actionPreconds[index]->kleeneCachingVectorSize << endl;
        }

        out << "## action hash keys" << endl;
        for (unsigned int actionIndex = 0;
             actionIndex < actionPreconds[index]->actionHashKeyMap.size();
             ++actionIndex) {
            out << actionIndex << " "
                << actionPreconds[index]->actionHashKeyMap[actionIndex]
                << endl;
        }

        out << endl;
    }

    out << endl << endl << "#####ACTION STATES#####" << endl;
    for (unsigned int index = 0; index < actionStates.size(); ++index) {
        assert(index == actionStates[index].index);
        out << "## index" << endl;
        out << index << endl;
        out << "## state" << endl;
        for (unsigned int varIndex = 0;
             varIndex < actionStates[index].state.size(); ++varIndex) {
            out << actionStates[index][varIndex] << " ";
        }
        out << endl;
        out << "## relevant preconditions" << endl;
        out << actionStates[index].relevantSACs.size() << endl;
        for (unsigned int sacIndex = 0;
             sacIndex < actionStates[index].relevantSACs.size(); ++sacIndex) {
            out << actionStates[index].relevantSACs[sacIndex]->index << " ";
        }
        out << endl;
        out << endl;
    }

    out << endl
        << "#####HASH KEYS OF DETERMINISTIC STATE FLUENTS#####" << endl;
    for (unsigned int index = 0; index < firstProbabilisticVarIndex; ++index) {
        assert(CPFs[index]->head->index == index);
        out << "## index" << endl;
        out << index << endl;
        if (!stateHashKeys.empty()) {
            out << "## state hash key (for each value in the domain)"
                << endl;
            for (unsigned int valIndex = 0;
                 valIndex < stateHashKeys[index].size(); ++valIndex) {
                out << stateHashKeys[index][valIndex];
                if (valIndex != stateHashKeys[index].size() - 1) {
                    out << " ";
                }
            }
        }
        out << endl;

        if (!kleeneStateHashKeyBases.empty()) {
            out << "## kleene state hash key base" << endl;
            out << kleeneStateHashKeyBases[index] << endl;
        }

        out << "## state fluent hash keys (first line is the number of keys)"
            << endl;
        out << indexToStateFluentHashKeyMap[index].size() << endl;
        for (unsigned int i = 0; i < indexToStateFluentHashKeyMap[index].size();
             ++i) {
            out << indexToStateFluentHashKeyMap[index][i].first << " ";
            out << indexToStateFluentHashKeyMap[index][i].second << endl;
        }

        out << "## kleene state fluent hash keys (first line is the number of "
               "keys)"
            << endl;
        out << indexToKleeneStateFluentHashKeyMap[index].size() << endl;
        for (unsigned int i = 0;
             i < indexToKleeneStateFluentHashKeyMap[index].size(); ++i) {
            out << indexToKleeneStateFluentHashKeyMap[index][i].first << " ";
            out << indexToKleeneStateFluentHashKeyMap[index][i].second
                << endl;
        }
        out << endl;
    }

    out << endl
        << "#####HASH KEYS OF PROBABILISTIC STATE FLUENTS#####" << endl;
    for (unsigned int index = firstProbabilisticVarIndex; index < CPFs.size();
         ++index) {
        assert(CPFs[index]->head->index == index);
        out << "## index" << endl;
        out << (index - firstProbabilisticVarIndex) << endl;
        if (!stateHashKeys.empty()) {
            out << "## state hash key (for each value in the domain)"
                << endl;
            for (unsigned int valIndex = 0;
                 valIndex < stateHashKeys[index].size(); ++valIndex) {
                out << stateHashKeys[index][valIndex];
                if (valIndex != stateHashKeys[index].size() - 1) {
                    out << " ";
                }
            }
        }
        out << endl;

        if (!kleeneStateHashKeyBases.empty()) {
            out << "## kleene state hash key base" << endl;
            out << kleeneStateHashKeyBases[index] << endl;
        }

        out << "## state fluent hash keys (first line is the number of keys)"
            << endl;
        out << indexToStateFluentHashKeyMap[index].size() << endl;
        for (unsigned int i = 0; i < indexToStateFluentHashKeyMap[index].size();
             ++i) {
            out << indexToStateFluentHashKeyMap[index][i].first << " ";
            out << indexToStateFluentHashKeyMap[index][i].second << endl;
        }

        out << "## kleene state fluent hash keys (first line is the number of "
               "keys)"
            << endl;
        out << indexToKleeneStateFluentHashKeyMap[index].size() << endl;
        for (unsigned int i = 0;
             i < indexToKleeneStateFluentHashKeyMap[index].size(); ++i) {
            out << indexToKleeneStateFluentHashKeyMap[index][i].first << " ";
            out << indexToKleeneStateFluentHashKeyMap[index][i].second
                << endl;
        }
        out << endl;
    }

    out << endl << endl << "#####TRAINING SET#####" << endl;
    out << trainingSet.size() << endl;
    for (State st : trainingSet) {
        for (unsigned int i = 0; i < st.state.size(); ++i) {
            out << st.state[i] << " ";
        }
        out << endl;
    }
}
