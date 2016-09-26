#include "rddl.h"

#include <fstream>
#include <iostream>

#include "logical_expressions.h"

#include "evaluatables.h"
#include "instantiator.h"
#include "preprocessor.h"
#include "task_analyzer.h"

#include "utils/math_utils.h"
#include "utils/string_utils.h"
#include "utils/system_utils.h"
#include "utils/timer.h"

std::string Domain::validRequirement(RDDLTask rddlTask, std::string req) {
    if (rddlTask.validRequirements.find(req)
              == rddlTask.validRequirements.end()) {
        std::cerr << "Error! Invalid requirement: " << req << std::endl;
        exit(EXIT_FAILURE);
    }
    return req;
}

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


    validRequirements = {
        "continuous",         "multivalued",       "reward-deterministic",
        "intermediate-nodes", "constrained-state", "partially-observed",
        "concurrent",         "integer-valued",    "CPF-deterministic"};
}

void RDDLTask::addTypes(Domain* domain) {
    for (SchematicType* type : domain->getDomainTypes()) {
        if (type->getSuperTypeList().empty()) {
            addType(type->getName(), type->getSuperType());
        } else {
            // Type definition using enum values (type_name : @enum1,...,@enumN)
            addType(type->getName());
            for (std::string const& objectName : type->getSuperTypeList()) {
                addObject(type->getName(), objectName);
            }
        }
    }
}

void RDDLTask::addVariables(Domain* domain) {
    // Adding VarSection
    for (VariableSchematic* varDef : domain->getVariableSchematics()) {
        std::string name = varDef->getName();
        std::vector<Parameter*> params;
        for (std::string const& param : varDef->getParameters()) {
            if (types.find(param) == types.end()) {
                SystemUtils::abort("Undefined type " + param +
                                   " used as parameter in " + name + ".");
            } else {
                params.push_back(
                    new Parameter(types[param]->name, types[param]));
            }
        }

        // TODO: This initialization here is wrong but is put here to prevent
        // warning during the compliation. Setting it to nullptr doesn't help
        ParametrizedVariable::VariableType varType =
            ParametrizedVariable::STATE_FLUENT;
        std::string varTypeName = varDef->getVarType();
        if (varTypeName == "state-fluent") {
            varType = ParametrizedVariable::STATE_FLUENT;
        } else if (varTypeName == "action-fluent") {
            varType = ParametrizedVariable::ACTION_FLUENT;
        } else if (varTypeName == "non-fluent") {
            varType = ParametrizedVariable::NON_FLUENT;
        }
        // TODO: cover other types of parametrized variables
        else {
            SystemUtils::abort("Parametrized variable: " + varTypeName +
                               " not implemented. Implemented for now: "
                               "state-fluent, action-fluent, non-fluent.");
        }

        std::string defaultVarType = varDef->getDefaultVarType();
        if (types.find(defaultVarType) == types.end()) {
            SystemUtils::abort("Unknown type " + defaultVarType + " defined.");
        }

        Type* valueType = types[defaultVarType];
        double defaultVarValue;

        switch (varType) {
        case ParametrizedVariable::NON_FLUENT:
        case ParametrizedVariable::STATE_FLUENT:
        case ParametrizedVariable::ACTION_FLUENT: {
            std::string satisfactionType = varDef->getSatisfactionType();
            if (satisfactionType != "default") {
                SystemUtils::abort(
                    "Unknown satisfaction type for parametrized variable " +
                    varTypeName + ". Did you mean 'default'?");
            }
            std::string defaultVarValString = varDef->getDefaultVarValue();

            // TODO: ?? -> || valueType->name == "bool")
            if (valueType->name == "int" || valueType->name == "real") {
                defaultVarValue = std::atof(defaultVarValString.c_str());
            } else {
                if (objects.find(defaultVarValString) == objects.end()) {
                    SystemUtils::abort("Default value " + defaultVarValString +
                                       " of variable " + name +
                                       " not defined.");
                }

                Object* val = objects[defaultVarValString];
                defaultVarValue = val->value;
            }
            break;
        }

        // case ParametrizedVariable::INTERM_FLUENT:
        // case ParametrizedVariable::DERIVED_FLUENT:
        //     if (satisfactionType != "level") {
        //         SystemUtils::abort("Unknown satisfaction type for
        //         parametrized variable " + varTypeName + ". Did you mean
        //         'level'?");
        //     }
        //     // TODO: implement this
        //     SystemUtils::abort("Not implemented. Variable type: " + varTypeName);
        //     break;

        default: {
            SystemUtils::abort("Unknown variable type: " + varTypeName);
            break;
        }
        }

        ParametrizedVariable* var = new ParametrizedVariable(
            name, params, varType, valueType, defaultVarValue);

        addVariableSchematic(var);
    }
}

void RDDLTask::addCPFs(Domain* domain) {
    // Consists of Parametrized variable and logical expression
    for (CPFSchematic* cpf : domain->getCPFs()) {
        // Variable
        std::string name = cpf->getVariable()->getName();

        if (name[name.length() - 1] == '\'') {
            name = name.substr(0, name.length() - 1);
        }

        if (variableDefinitions.find(name) == variableDefinitions.end()) {
            SystemUtils::abort("No according variable to CPF " + name + ".");
        }

        ParametrizedVariable* head = variableDefinitions[name];

        if (cpf->getVariable()->getParameters().size() != head->params.size()) {
            SystemUtils::abort(
                "Wrong number of parameters for parametrized variable " + name +
                ".");
        }

        for (int i = 0; i < cpf->getVariable()->getParameters().size(); ++i) {
            head->params[i]->name = cpf->getVariable()->getParameters()[i];
        }

        if (CPFDefinitions.find(head) != CPFDefinitions.end()) {
            SystemUtils::abort("Error: Multiple definition of CPF " + name +
                               ".");
        }
        // Expression
        CPFDefinitions[head] = cpf->getLogicalExpression();
    }
}

void RDDLTask::setReward(Domain* domain) {
    assert(!domain->getReward());
    setRewardCPF(domain->getReward());
}

void RDDLTask::addStateConstraints(Domain* domain) {
    for (LogicalExpression* expr : domain->getStateConstraints()) {
        SACs.push_back(expr);
    }
}

void RDDLTask::addObjects(Domain* domain) {
    for (ObjectSchematic* objDef : domain->getObjects()) {
        std::string typeName = objDef->getTypeName();
        if (types.find(typeName) == types.end()) {
            SystemUtils::abort("Unknown object " + typeName);
        }

        for (std::string const& objectName : objDef->getObjectNames()) {
          std::cout << "Added object (from objects section): " << typeName
          << " : " << objectName << std::endl;
            addObject(typeName, objectName);
        }
    }
}

void RDDLTask::addDomain(Domain* domain) {
    // TODO: requirements section is stored and prepared, implementation is left

    domainName = domain->getName();

    //addTypes(domain);
    addVariables(domain);
    addCPFs(domain);
    setReward(domain);
    addStateConstraints(domain);
    //addObjects(domain);
    // TODO: StateInvariantSection
}

void RDDLTask::addNonFluent(NonFluentBlock* nonFluent) {
    // Set nonFluentsName
    nonFluentsName = nonFluent->getName();

    // Check if domain name is corresponding
    if (nonFluent->getDomainName() != this->getDomainName()) {
        SystemUtils::abort("Unknown domain " + nonFluent->getDomainName() +
                           "  defined in Non fluent section");
    }

    // NOTE: No need to add objects here, since they are added directly from parser.ypp
    // Adding objects
    // for (ObjectSchematic* objDef : nonFluent->getObjects()) {
    //     std::string typeName = objDef->getTypeName();
    //
    //     for (std::string const& objectName : objDef->getObjectNames()) {
    //         addObject(typeName, objectName);
    //         std::cout << "Added object (from non fluents): " << objectName <<
    //         " of type " << typeName << std::endl;
    //     }
    // }

    // Adding non fluents
    std::vector<Parameter*> params;
    for (VariablesInstanceSchematic* varDef : nonFluent->getNonFluents()) {
        std::string name = varDef->getName();
        params.clear();
        // Set parameters
        for (std::string const& paramName : varDef->getParameters()) {
            if (objects.find(paramName) == objects.end()) {
                SystemUtils::abort("Unknown object: " + paramName);
            }

            // TODO: this is wrong. It only covers the case that
            // parametrized variable is a Parameter Expression (see issue
            // #15)
            params.push_back(objects[paramName]);
        }

        if (variableDefinitions.find(name) == variableDefinitions.end()) {
            SystemUtils::abort("Variable " + name + " used but not defined.");
        }

        ParametrizedVariable* parent = variableDefinitions[name];
        double initialVal = varDef->getInitValue();

        addParametrizedVariable(parent, params, initialVal);
    }
}

void RDDLTask::addInstance(Instance* instance) {
    this->name = instance->getName();

    // Check domain name
    if (this->getDomainName() != instance->getDomainName()) {
        SystemUtils::abort("Unknown domain " + instance->getDomainName() +
                           " defined in Instance section");
    }

    // Check Non fluents name
    if (this->getNonFluentsName() != instance->getNonFluentsName()) {
        SystemUtils::abort("Unknown non fluents " +
                           instance->getNonFluentsName() +
                           "defined in Non fluent section");
    }

    // Add init states
    // TODO: This is basically the same code as for non-fluents, think about
    // helper method
    for (VariablesInstanceSchematic* varDef : instance->getVariables()) {
        std::string name = varDef->getName();
        std::vector<Parameter*> params;
        // Set parameters
        for (std::string const& paramName : varDef->getParameters()) {
            if (objects.find(paramName) == objects.end()) {
                SystemUtils::abort("Unknown object: " + paramName);
            }
            // TODO: this is wrong. It only covers the case that
            // parametrized variable is a Parameter Expression (see issue
            // #15)
            params.push_back(objects[paramName]);
        }

        if (variableDefinitions.find(name) == variableDefinitions.end()) {
            SystemUtils::abort("Variable " + name + " used but not defined.");
        }

        ParametrizedVariable* parent = variableDefinitions[name];
        double initialVal = varDef->getInitValue();

        addParametrizedVariable(parent, params, initialVal);
    }

    // Set Max nondef actions
    numberOfConcurrentActions = instance->getMaxNonDefActions();

    // Set horizon
    horizon = instance->getHorizon();

    // Set discount
    discountFactor = instance->getDiscount();
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
    for (std::set<State>::iterator it = trainingSet.begin();
         it != trainingSet.end(); ++it) {
        for (unsigned int i = 0; i < it->state.size(); ++i) {
            out << it->state[i] << " ";
        }
        out << std::endl;
    }
}

void RDDLTask::execute(std::string td /*target dir*/) {
    std::cout << "Executing..." << std::endl << "Writing output.." << std::endl;

    Timer t, totalTime;
    std::cout << "Parsing...finished" << std::endl;

    t.reset();
    std::cout << "instantiating..." << std::endl;
    Instantiator instantiator(this);
    instantiator.instantiate();
    std::cout << "...finished (" << t << ")." << std::endl;

    t.reset();
    std::cout << "preprocessing..." << std::endl;
    Preprocessor preprocessor(this);
    preprocessor.preprocess();
    std::cout << "...finished (" << t << ")." << std::endl;

    t.reset();
    std::cout << "analyzing task..." << std::endl;
    TaskAnalyzer analyzer(this);
    analyzer.analyzeTask();
    std::cout << "...finished (" << t << ")." << std::endl;

    t.reset();
    std::cout << "writing output..." << std::endl;
    std::ofstream resultFile;
    std::cout << "Task name " << name << std::endl;
    std::string targetDir = td + "/" + name;
    resultFile.open(targetDir.c_str());
    print(resultFile);
    resultFile.close();
    // print(std::cout);
    std::cout << "...finished (" << t << ")." << name << std::endl;

    std::cout << "total time: " << totalTime << std::endl;
}

/*****************************************************************
                           Helper Methods
*****************************************************************/

// This method is used to get the value of parametrized variable
// Parametrized variable is stored with its definition in VariableSchematic
// and the values of parameters used in the particular call of parametrized
// variable are stored in varExpression
ParametrizedVariable* RDDLTask::getParametrizedVariableFromVariableSchematic(
    std::string varName) {
    if (parametrizedVariableSchematicsMap.find(varName) ==
        parametrizedVariableSchematicsMap.end()) {
        return nullptr;
    }

    VariableSchematic* varSchematic =
        parametrizedVariableSchematicsMap[varName];
    VariableExpression* varExpression = parametrizedVariableMap[varName];

    std::string name = varSchematic->getName();
    std::string varTypeName = varSchematic->getVarType();
    std::string defaultVarType = varSchematic->getDefaultVarType();
    std::string satisfactionType = varSchematic->getSatisfactionType();
    std::string defaultVarValString = varSchematic->getDefaultVarValue();
    double defaultVarValue;

    std::vector<Parameter*> params;
    // Adding parameters from VariableExpression (those are the parameters
    // that user set when he called parametrized variablea as an espression)
    for (unsigned i = 0; i < varSchematic->getParameters().size(); i++)
        if (types.find((varSchematic->getParameters())[i]) ==
            types.end()) {
            SystemUtils::abort("Undefined type " +
                               (varExpression->getParameters())[i] +
                               " used as parameter in " + name + ".");
        } else {
            params.push_back(
                new Parameter((varExpression->getParameters())[i],
                              types[(varSchematic->getParameters())[i]]));
        }

    // TODO: This initialization here is wrong but is put here to prevent
    // warning during the compliation
    // setting it to nullptr doesn't help
    ParametrizedVariable::VariableType varType =
        ParametrizedVariable::STATE_FLUENT;
    if (varTypeName == "state-fluent") {
        varType = ParametrizedVariable::STATE_FLUENT;
    } else if (varTypeName == "action-fluent") {
        varType = ParametrizedVariable::ACTION_FLUENT;
    } else if (varTypeName == "non-fluent") {
        varType = ParametrizedVariable::NON_FLUENT;
    }
    // TODO: cover other types of parametrized variables
    else {
        SystemUtils::abort("Parametrized variable: " + varTypeName +
                           " not implemented. Implemented for now: "
                           "state-fluent, action-fluent, non-fluent.");
    }

    if (types.find(defaultVarType) == types.end()) {
        SystemUtils::abort("Unknown type " + defaultVarType + " defined.");
    }
    Type* valueType = types[defaultVarType];

    switch (varType) {
    case ParametrizedVariable::NON_FLUENT:
    case ParametrizedVariable::STATE_FLUENT:
    case ParametrizedVariable::ACTION_FLUENT:
        if (satisfactionType != "default") {
            SystemUtils::abort(
                "Unknown satisfaction type for parametrized variable " +
                varTypeName + ". Did you mean 'default'?");
        }

        if (valueType->name == "int" ||
            valueType->name ==
                "real") { // TODO: ?? -> || valueType->name == "bool")
            defaultVarValue = std::atof(defaultVarValString.c_str());
        } else {
            if (objects.find(defaultVarValString) == objects.end()) {
                SystemUtils::abort("Default value " + defaultVarValString +
                                   " of variable " + name + " not defined.");
            }
            Object* val = objects[defaultVarValString];
            defaultVarValue = val->value;
        }
        break;

    // case ParametrizedVariable::INTERM_FLUENT:
    // case ParametrizedVariable::DERIVED_FLUENT:
    //     if (satisfactionType != "level")
    //         SystemUtils::abort("Unknown satisfaction type for parametrized
    //         variable " + varTypeName + ". Did you mean 'level'?");
    //
    //     // TODO: implement this
    //     SystemUtils::abort("Not implemented. Variable type: " + varTypeName);
    //     break;

    default:
        SystemUtils::abort("Unknown variable type: " + varTypeName);
        break;
    }
    ParametrizedVariable* var = new ParametrizedVariable(
        name, params, varType, valueType, defaultVarValue);

    return var;
}

void RDDLTask::storeParametrizedVariableFromVariableSchematic(std::string varName,
                                                 VariableSchematic* var) {
    parametrizedVariableSchematicsMap[varName] = var;
}

void RDDLTask::storeParametrizedVariableMap(std::string varName,
                                  VariableExpression* varExpression) {
    parametrizedVariableMap[varName] = varExpression;
}

Object* RDDLTask::getObject(std::string objName) {
    if (objects.find(objName) != objects.end()) {
        return objects[objName];
    }
    return nullptr;
}

Type* RDDLTask::getType(std::string typeName) {
    if (types.find(typeName) != types.end()) {
        return types[typeName];
    }
    return nullptr;
}
