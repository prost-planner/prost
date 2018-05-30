#include "rddl.h"

#include <fstream>
#include <iostream>

#include "logical_expressions.h"
#include <cstdarg>

#include "evaluatables.h"
#include "instantiator.h"
#include "preprocessor.h"
#include "task_analyzer.h"

#include "utils/math_utils.h"
#include "utils/string_utils.h"
#include "utils/system_utils.h"
#include "utils/timer.h"

/*****************************************************************
                           RDDL Block
*****************************************************************/
RDDLTask::RDDLTask()
    : numberOfConcurrentActions(std::numeric_limits<int>::max()),
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
    std::string name = variable.variableName;
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
        SystemUtils::abort("Error: Multiple definition of CPF " + name +
                           ".");
    }
    // Expression
    CPFDefinitions[head] = logicalExpression;
}

void RDDLTask::setInstance(std::string name, std::string domainName,
         std::string nonFluentsName,
         int maxNonDefActions, int horizon, double discount) {

    this->name = name;

    // Check domain name
    if (this->domainName != domainName) {
        SystemUtils::abort("Unknown domain " + domainName +
                           " defined in Instance section");
    }

    // Check Non fluents name
    if (this->nonFluentsName != nonFluentsName) {
        SystemUtils::abort("Unknown non fluents " +
                           nonFluentsName +
                           "defined in Non fluent section");
    }

    // Set Max nondef actions
    this->numberOfConcurrentActions = maxNonDefActions;

    // Set horizon
    this->horizon = horizon;

    // Set discount
    this->discountFactor = discount;
}

void RDDLTask::addType(std::string const& name, std::string const& superType) {
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

Type* RDDLTask::getType(std::string typeName) {
    if (types.find(typeName) != types.end()) {
        return types[typeName];
    }
    return nullptr;
}

void RDDLTask::addObject(std::string const& typeName,
                         std::string const& objectName) {
    if (types.find(typeName) == types.end()) {
        SystemUtils::abort("Error: Type " + typeName + " not defined.");
    }

    if (objects.find(objectName) != objects.end()) {
        SystemUtils::abort("Error: Object name " + objectName +
                           " is ambiguous.");
    }

    objects[objectName] = new Object(objectName, types[typeName]);
}

Object* RDDLTask::getObject(std::string objName) {
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
                                       std::vector<Parameter*> const& params) {
    addParametrizedVariable(parent, params, parent->initialValue);
}

void RDDLTask::addParametrizedVariable(ParametrizedVariable* parent,
                                       std::vector<Parameter*> const& params,
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
            stateFluentsBySchema[parent] = std::vector<StateFluent*>();
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

ParametrizedVariable* RDDLTask::getParametrizedVariable(std::string varName) {
  if (variableDefinitions.find(varName) != variableDefinitions.end()) {
    return variableDefinitions[varName];
  } else {
    SystemUtils::abort("Unknown variable: " + varName + ".");
  }
  return nullptr;
}

StateFluent* RDDLTask::getStateFluent(std::string const& name) {
    if (stateFluentMap.find(name) == stateFluentMap.end()) {
        SystemUtils::abort("Error: state-fluent " + name +
                           " used but not defined.");
        return nullptr;
    }
    return stateFluentMap[name];
}

ActionFluent* RDDLTask::getActionFluent(std::string const& name) {
    if (actionFluentMap.find(name) == actionFluentMap.end()) {
        SystemUtils::abort("Error: action-fluent " + name +
                           " used but not defined.");
        return nullptr;
    }
    return actionFluentMap[name];
}

NonFluent* RDDLTask::getNonFluent(std::string const& name) {
    if (nonFluentMap.find(name) == nonFluentMap.end()) {
        SystemUtils::abort("Error: non-fluent " + name +
                           " used but not defined.");
        return nullptr;
    }
    return nonFluentMap[name];
}

// TODO: Return const reference?
std::vector<StateFluent*> RDDLTask::getStateFluentsOfSchema(
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

void RDDLTask::print(std::ostream& out) {
    // Set precision of doubles
    out.unsetf(std::ios::floatfield);
    out.precision(std::numeric_limits<double>::digits10);

    int firstProbabilisticVarIndex = (int)CPFs.size();
    bool deterministic = true;
    for (unsigned int i = 0; i < CPFs.size(); ++i) {
        if (CPFs[i]->isProbabilistic()) {
            firstProbabilisticVarIndex = i;
            deterministic = false;
            break;
        }
    }

    out << "#####TASK#####" << std::endl;
    out << "## name" << std::endl;
    out << name << std::endl;
    out << "## horizon" << std::endl;
    out << horizon << std::endl;
    out << "## discount factor" << std::endl;
    out << discountFactor << std::endl;
    out << "## number of action fluents" << std::endl;
    out << actionFluents.size() << std::endl;
    out << "## number of det state fluents" << std::endl;
    out << firstProbabilisticVarIndex << std::endl;
    out << "## number of prob state fluents" << std::endl;
    out << (CPFs.size() - firstProbabilisticVarIndex) << std::endl;
    out << "## number of preconds" << std::endl;
    out << actionPreconds.size() << std::endl;
    out << "## number of actions" << std::endl;
    out << actionStates.size() << std::endl;
    out << "## number of hashing functions" << std::endl;
    out << (actionPreconds.size() + CPFs.size() + 1) << std::endl;
    out << "## initial state" << std::endl;
    for (unsigned int i = 0; i < CPFs.size(); ++i) {
        out << CPFs[i]->getInitialValue() << " ";
    }
    out << std::endl;
    out << "## 1 if task is deterministic" << std::endl;
    out << deterministic << std::endl;
    out << "## 1 if state hashing possible" << std::endl;
    out << !stateHashKeys.empty() << std::endl;
    out << "## 1 if kleene state hashing possible" << std::endl;
    out << !kleeneStateHashKeyBases.empty() << std::endl;
    out << "## method to calculate the final reward" << std::endl;
    out << finalRewardCalculationMethod << std::endl;
    if (finalRewardCalculationMethod == "BEST_OF_CANDIDATE_SET") {
        out << "## set of candidates to calculate final reward (first line is "
               "the number)"
            << std::endl;
        out << candidatesForOptimalFinalAction.size() << std::endl;
        for (unsigned int i = 0; i < candidatesForOptimalFinalAction.size();
             ++i) {
            out << candidatesForOptimalFinalAction[i] << " ";
        }
        out << std::endl;
    }
    out << "## 1 if reward formula allows reward lock detection and a reward "
           "lock was found during task analysis"
        << std::endl;
    out << rewardLockDetected << std::endl;
    out << "## 1 if an unreasonable action was detected" << std::endl;
    out << unreasonableActionDetected << std::endl;
    out << "## 1 if an unreasonable action was detected in the determinization"
        << std::endl;
    out << unreasonableActionInDeterminizationDetected << std::endl;

    out << "## number of states that were encountered during task analysis"
        << std::endl;
    out << numberOfEncounteredStates << std::endl;
    out << "## number of unique states that were encountered during task "
           "analysis"
        << std::endl;
    out << numberOfUniqueEncounteredStates << std::endl;
    out << "## number of states with only one applicable reasonable action "
           "that were encountered during task analysis"
        << std::endl;
    out << nonTerminalStatesWithUniqueAction << std::endl;
    out << "## number of unique states with only one applicable reasonable "
           "action that were encountered during task analysis"
        << std::endl;
    out << uniqueNonTerminalStatesWithUniqueAction << std::endl;

    out << std::endl << std::endl << "#####ACTION FLUENTS#####" << std::endl;
    for (unsigned int i = 0; i < actionFluents.size(); ++i) {
        out << "## index" << std::endl;
        out << actionFluents[i]->index << std::endl;
        out << "## name" << std::endl;
        out << actionFluents[i]->fullName << std::endl;
        out << "## number of values" << std::endl;
        out << "2" << std::endl;
        out << "## values" << std::endl;
        out << "0 false" << std::endl << "1 true" << std::endl;
        out << std::endl;
    }

    out << std::endl
        << std::endl
        << "#####DET STATE FLUENTS AND CPFS#####" << std::endl;
    for (unsigned int index = 0; index < firstProbabilisticVarIndex; ++index) {
        assert(CPFs[index]->head->index == index);
        assert(!CPFs[index]->isProbabilistic());
        out << "## index" << std::endl;
        out << index << std::endl;
        out << "## name" << std::endl;
        out << CPFs[index]->head->fullName << std::endl;
        out << "## number of values" << std::endl;
        out << CPFs[index]->domain.size() << std::endl;
        out << "## values" << std::endl;
        for (std::set<double>::iterator it = CPFs[index]->domain.begin();
             it != CPFs[index]->domain.end(); ++it) {
            out << *it << " "
                << CPFs[index]->head->valueType->objects[*it]->name
                << std::endl;
        }

        out << "## formula" << std::endl;
        CPFs[index]->formula->print(out);
        out << std::endl;

        out << "## hash index" << std::endl;
        out << CPFs[index]->hashIndex << std::endl;
        out << "## caching type " << std::endl;
        out << CPFs[index]->cachingType << std::endl;
        if (CPFs[index]->cachingType == "VECTOR") {
            out << "## precomputed results" << std::endl;
            out << CPFs[index]->precomputedResults.size() << std::endl;
            for (unsigned int res = 0;
                 res < CPFs[index]->precomputedResults.size(); ++res) {
                out << res << " " << CPFs[index]->precomputedResults[res]
                    << std::endl;
            }
        }
        out << "## kleene caching type" << std::endl;
        out << CPFs[index]->kleeneCachingType << std::endl;
        if (CPFs[index]->kleeneCachingType == "VECTOR") {
            out << "## kleene caching vec size" << std::endl;
            out << CPFs[index]->kleeneCachingVectorSize << std::endl;
        }

        out << "## action hash keys" << std::endl;
        for (unsigned int actionIndex = 0;
             actionIndex < CPFs[index]->actionHashKeyMap.size();
             ++actionIndex) {
            out << actionIndex << " "
                << CPFs[index]->actionHashKeyMap[actionIndex] << std::endl;
        }
        out << std::endl;
    }

    out << std::endl
        << std::endl
        << "#####PROB STATE FLUENTS AND CPFS#####" << std::endl;
    for (unsigned int index = firstProbabilisticVarIndex; index < CPFs.size();
         ++index) {
        assert(CPFs[index]->head->index == index);
        assert(CPFs[index]->isProbabilistic());
        out << "## index" << std::endl;
        out << (index - firstProbabilisticVarIndex) << std::endl;
        out << "## name" << std::endl;
        out << CPFs[index]->head->fullName << std::endl;
        out << "## number of values" << std::endl;
        out << CPFs[index]->domain.size() << std::endl;
        out << "## values" << std::endl;
        for (std::set<double>::iterator it = CPFs[index]->domain.begin();
             it != CPFs[index]->domain.end(); ++it) {
            out << *it << " "
                << CPFs[index]->head->valueType->objects[*it]->name
                << std::endl;
        }

        out << "## formula" << std::endl;
        CPFs[index]->formula->print(out);
        out << std::endl;

        out << "## determinized formula" << std::endl;
        CPFs[index]->determinization->print(out);
        out << std::endl;

        out << "## hash index" << std::endl;
        out << CPFs[index]->hashIndex << std::endl;
        out << "## caching type " << std::endl;
        out << CPFs[index]->cachingType << std::endl;
        if (CPFs[index]->cachingType == "VECTOR") {
            out << "## precomputed results (key - determinization - size of "
                   "distribution - value-probability pairs)"
                << std::endl;
            out << CPFs[index]->precomputedResults.size() << std::endl;
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
                out << std::endl;
            }
        }
        out << "## kleene caching type" << std::endl;
        out << CPFs[index]->kleeneCachingType << std::endl;
        if (CPFs[index]->kleeneCachingType == "VECTOR") {
            out << "## kleene caching vec size" << std::endl;
            out << CPFs[index]->kleeneCachingVectorSize << std::endl;
        }

        out << "## action hash keys" << std::endl;
        for (unsigned int actionIndex = 0;
             actionIndex < CPFs[index]->actionHashKeyMap.size();
             ++actionIndex) {
            out << actionIndex << " "
                << CPFs[index]->actionHashKeyMap[actionIndex] << std::endl;
        }

        out << std::endl;
    }

    out << std::endl << std::endl << "#####REWARD#####" << std::endl;
    out << "## formula" << std::endl;
    rewardCPF->formula->print(out);
    out << std::endl;
    out << "## min" << std::endl;
    out << *rewardCPF->domain.begin() << std::endl;
    out << "## max" << std::endl;
    out << *rewardCPF->domain.rbegin() << std::endl;
    out << "## independent from actions" << std::endl;
    out << (rewardCPF->positiveActionDependencies.empty() &&
            rewardCPF->negativeActionDependencies.empty())
        << std::endl;
    out << "## hash index" << std::endl;
    out << rewardCPF->hashIndex << std::endl;
    out << "## caching type" << std::endl;
    out << rewardCPF->cachingType << std::endl;
    if (rewardCPF->cachingType == "VECTOR") {
        out << "## precomputed results" << std::endl;
        out << rewardCPF->precomputedResults.size() << std::endl;
        for (unsigned int res = 0; res < rewardCPF->precomputedResults.size();
             ++res) {
            out << res << " " << rewardCPF->precomputedResults[res]
                << std::endl;
        }
    }
    out << "## kleene caching type" << std::endl;
    out << rewardCPF->kleeneCachingType << std::endl;
    if (rewardCPF->kleeneCachingType == "VECTOR") {
        out << "## kleene caching vec size" << std::endl;
        out << rewardCPF->kleeneCachingVectorSize << std::endl;
    }

    out << "## action hash keys" << std::endl;
    for (unsigned int actionIndex = 0;
         actionIndex < rewardCPF->actionHashKeyMap.size(); ++actionIndex) {
        out << actionIndex << " " << rewardCPF->actionHashKeyMap[actionIndex]
            << std::endl;
    }

    out << std::endl << std::endl << "#####PRECONDITIONS#####" << std::endl;

    for (unsigned int index = 0; index < actionPreconds.size(); ++index) {
        assert(actionPreconds[index]->index == index);
        out << "## index" << std::endl;
        out << index << std::endl;
        out << "## formula" << std::endl;
        actionPreconds[index]->formula->print(out);
        out << std::endl;
        out << "## hash index" << std::endl;
        out << actionPreconds[index]->hashIndex << std::endl;
        out << "## caching type" << std::endl;
        out << actionPreconds[index]->cachingType << std::endl;
        if (actionPreconds[index]->cachingType == "VECTOR") {
            out << "## precomputed results" << std::endl;
            out << actionPreconds[index]->precomputedResults.size()
                << std::endl;
            for (unsigned int res = 0;
                 res < actionPreconds[index]->precomputedResults.size();
                 ++res) {
                out << res << " "
                    << actionPreconds[index]->precomputedResults[res]
                    << std::endl;
            }
        }
        out << "## kleene caching type" << std::endl;
        out << actionPreconds[index]->kleeneCachingType << std::endl;
        if (actionPreconds[index]->kleeneCachingType == "VECTOR") {
            out << "## kleene caching vec size" << std::endl;
            out << actionPreconds[index]->kleeneCachingVectorSize << std::endl;
        }

        out << "## action hash keys" << std::endl;
        for (unsigned int actionIndex = 0;
             actionIndex < actionPreconds[index]->actionHashKeyMap.size();
             ++actionIndex) {
            out << actionIndex << " "
                << actionPreconds[index]->actionHashKeyMap[actionIndex]
                << std::endl;
        }

        out << std::endl;
    }

    out << std::endl << std::endl << "#####ACTION STATES#####" << std::endl;
    for (unsigned int index = 0; index < actionStates.size(); ++index) {
        assert(index == actionStates[index].index);
        out << "## index" << std::endl;
        out << index << std::endl;
        out << "## state" << std::endl;
        for (unsigned int varIndex = 0;
             varIndex < actionStates[index].state.size(); ++varIndex) {
            out << actionStates[index][varIndex] << " ";
        }
        out << std::endl;
        out << "## relevant preconditions" << std::endl;
        out << actionStates[index].relevantSACs.size() << std::endl;
        for (unsigned int sacIndex = 0;
             sacIndex < actionStates[index].relevantSACs.size(); ++sacIndex) {
            out << actionStates[index].relevantSACs[sacIndex]->index << " ";
        }
        out << std::endl;
        out << std::endl;
    }

    out << std::endl
        << "#####HASH KEYS OF DETERMINISTIC STATE FLUENTS#####" << std::endl;
    for (unsigned int index = 0; index < firstProbabilisticVarIndex; ++index) {
        assert(CPFs[index]->head->index == index);
        out << "## index" << std::endl;
        out << index << std::endl;
        if (!stateHashKeys.empty()) {
            out << "## state hash key (for each value in the domain)"
                << std::endl;
            for (unsigned int valIndex = 0;
                 valIndex < stateHashKeys[index].size(); ++valIndex) {
                out << stateHashKeys[index][valIndex];
                if (valIndex != stateHashKeys[index].size() - 1) {
                    out << " ";
                }
            }
        }
        out << std::endl;

        if (!kleeneStateHashKeyBases.empty()) {
            out << "## kleene state hash key base" << std::endl;
            out << kleeneStateHashKeyBases[index] << std::endl;
        }

        out << "## state fluent hash keys (first line is the number of keys)"
            << std::endl;
        out << indexToStateFluentHashKeyMap[index].size() << std::endl;
        for (unsigned int i = 0; i < indexToStateFluentHashKeyMap[index].size();
             ++i) {
            out << indexToStateFluentHashKeyMap[index][i].first << " ";
            out << indexToStateFluentHashKeyMap[index][i].second << std::endl;
        }

        out << "## kleene state fluent hash keys (first line is the number of "
               "keys)"
            << std::endl;
        out << indexToKleeneStateFluentHashKeyMap[index].size() << std::endl;
        for (unsigned int i = 0;
             i < indexToKleeneStateFluentHashKeyMap[index].size(); ++i) {
            out << indexToKleeneStateFluentHashKeyMap[index][i].first << " ";
            out << indexToKleeneStateFluentHashKeyMap[index][i].second
                << std::endl;
        }
        out << std::endl;
    }

    out << std::endl
        << "#####HASH KEYS OF PROBABILISTIC STATE FLUENTS#####" << std::endl;
    for (unsigned int index = firstProbabilisticVarIndex; index < CPFs.size();
         ++index) {
        assert(CPFs[index]->head->index == index);
        out << "## index" << std::endl;
        out << (index - firstProbabilisticVarIndex) << std::endl;
        if (!stateHashKeys.empty()) {
            out << "## state hash key (for each value in the domain)"
                << std::endl;
            for (unsigned int valIndex = 0;
                 valIndex < stateHashKeys[index].size(); ++valIndex) {
                out << stateHashKeys[index][valIndex];
                if (valIndex != stateHashKeys[index].size() - 1) {
                    out << " ";
                }
            }
        }
        out << std::endl;

        if (!kleeneStateHashKeyBases.empty()) {
            out << "## kleene state hash key base" << std::endl;
            out << kleeneStateHashKeyBases[index] << std::endl;
        }

        out << "## state fluent hash keys (first line is the number of keys)"
            << std::endl;
        out << indexToStateFluentHashKeyMap[index].size() << std::endl;
        for (unsigned int i = 0; i < indexToStateFluentHashKeyMap[index].size();
             ++i) {
            out << indexToStateFluentHashKeyMap[index][i].first << " ";
            out << indexToStateFluentHashKeyMap[index][i].second << std::endl;
        }

        out << "## kleene state fluent hash keys (first line is the number of "
               "keys)"
            << std::endl;
        out << indexToKleeneStateFluentHashKeyMap[index].size() << std::endl;
        for (unsigned int i = 0;
             i < indexToKleeneStateFluentHashKeyMap[index].size(); ++i) {
            out << indexToKleeneStateFluentHashKeyMap[index][i].first << " ";
            out << indexToKleeneStateFluentHashKeyMap[index][i].second
                << std::endl;
        }
        out << std::endl;
    }

    out << std::endl << std::endl << "#####TRAINING SET#####" << std::endl;
    out << trainingSet.size() << std::endl;
    for (State st : trainingSet) {
        for (unsigned int i = 0; i < st.state.size(); ++i) {
            out << st.state[i] << " ";
        }
        out << std::endl;
    }
}

void RDDLTask::execute(std::string td, double seed, int numStates, int numSimulations, bool useIPC2018Rules) {
    Timer t;
    srand(seed);

    t.reset();
    std::cout << "instantiating..." << std::endl;
    Instantiator instantiator(this);
    instantiator.instantiate();
    std::cout << "...finished (" << t << ")." << std::endl;

    t.reset();
    std::cout << "preprocessing..." << std::endl;
    Preprocessor preprocessor(this, useIPC2018Rules);
    preprocessor.preprocess();
    std::cout << "...finished (" << t << ")." << std::endl;

    t.reset();
    std::cout << "analyzing task..." << std::endl;
    TaskAnalyzer analyzer(this);
    analyzer.analyzeTask(numStates, numSimulations);
    std::cout << "...finished (" << t << ")." << std::endl;

    t.reset();
    std::ofstream resultFile;
    std::string targetDir = td + "/" + name;
    std::cout << "writing output for instance " << name << " to " << targetDir << " ..." << std::endl;
    resultFile.open(targetDir.c_str());
    print(resultFile);
    resultFile.close();
    // print(std::cout);
    std::cout << "...finished (" << t << ")." << std::endl;
}
