/**
    helper_methods.cc: helper methods implementations

    @author Đorđe Relić <dorde.relic@unibas.ch>
    @version 1.0 06/2016
*/

#include "helper_methods.h"

#include "utils/system_utils.h"

// This method is used to get the value of parametrized variable
// Parametrized variable is stored with its definition in pVarDefinition and the
// values of parameters used in the particular call of parametrized variable are stored in pVarExpression
ParametrizedVariable* getParametrizedVariableFromPvarDefinition(std::string pVarName) {
    PvarDefinition *pVarDefinition = parametrizedVariableDefinitionsMap[pVarName];
    PvarExpression *pVarExpression = parametrizedVariableMap[pVarName];

    std::string name = pVarDefinition->getName();
    std::string varTypeName = pVarDefinition->getVarType();
    std::string defaultVarType = pVarDefinition->getDefaultVarType();
    std::string satisfactionType = pVarDefinition->getSatisfactionType();
    std::string defaultVarValString = pVarDefinition->getDefaultVarValue();
    double defaultVarValue;

    std::vector<Parameter*> params;
    // Adding parameters from PvarExpression (those are the parameters that user set when he called parametrized variablea as an espression)
    for(unsigned i = 0; i < pVarDefinition->getParameters()->size(); i++)
        if (typeMap.find((*pVarDefinition->getParameters())[i]) == typeMap.end())
            SystemUtils::abort("Undefined type " + (*pVarExpression->getParameters())[i] + " used as parameter in " + name + ".");
        else
            params.push_back(new Parameter((*pVarExpression->getParameters())[i], typeMap[(*pVarDefinition->getParameters())[i]]));

    // TODO: This initialization here is wrong but is put here to prevent warning during the compliation
    // setting it to NULL doesn't help
    ParametrizedVariable::VariableType varType = ParametrizedVariable::STATE_FLUENT;
    if (varTypeName == "state-fluent")
        varType = ParametrizedVariable::STATE_FLUENT;
    else if (varTypeName == "action-fluent")
        varType = ParametrizedVariable::ACTION_FLUENT;
    else if (varTypeName == "non-fluent")
        varType = ParametrizedVariable::NON_FLUENT;
    // TODO: cover other types of parametrized variables
    else
        SystemUtils::abort("Parametrized variable: " + varTypeName + " not implemented. Implemented for now: state-fluent, action-fluent, non-fluent.");

    if (typeMap.find(defaultVarType) == typeMap.end())
        SystemUtils::abort("Unknown type " + defaultVarType + " defined.");
    Type* valueType = typeMap[defaultVarType];

    switch (varType) {
        case ParametrizedVariable::NON_FLUENT:
        case ParametrizedVariable::STATE_FLUENT:
        case ParametrizedVariable::ACTION_FLUENT:
            if (satisfactionType != "default")
                SystemUtils::abort("Unknown satisfaction type for parametrized variable " + varTypeName + ". Did you mean 'default'?");

            if (valueType->name == "int" || valueType->name == "real" ) // TODO: ?? -> || valueType->name == "bool")
                defaultVarValue = std::atof(defaultVarValString.c_str());
            else {
                if (objectMap.find(defaultVarValString) == objectMap.end())
                    SystemUtils::abort("Default value " + defaultVarValString + " of variable " + name + " not defined.");
                Object* val = objectMap[defaultVarValString];
                defaultVarValue = val->value;
            }
            break;

        // case ParametrizedVariable::INTERM_FLUENT:
        // case ParametrizedVariable::DERIVED_FLUENT:
        //     if (satisfactionType != "level")
        //         SystemUtils::abort("Unknown satisfaction type for parametrized variable " + varTypeName + ". Did you mean 'level'?");
        //
        //     // TODO: implement this
        //     SystemUtils::abort("Not implemented. Var type: " + varTypeName);
        //     break;

        default:
            SystemUtils::abort("Unknown var type: " + varTypeName);
            break;
    }
    // std::cout << "Adding parametrized variable: " << name << " with parameters: ";
    // for(std::vector<Parameter*>::iterator it = params.begin(); it != params.end(); it++)
    //     std::cout << (*it)->name << " of type " << (*it)->type->name << ", ";
    // std::cout << std::endl;
    ParametrizedVariable* var = new ParametrizedVariable(name, params, varType, valueType, defaultVarValue);

    // std::cout << "returned parametrized variable: " << name << " with parameters" << std::endl;
    // for (int i = 0; i < params.size(); i++)
    //     std::cout << "\t" << params[i]->name << std::endl;

    return var;
}
