#ifndef HELPER_METHODS_H
#define HELPER_METHODS_H

#include "logical_expressions.h"
#include "domain.h"

extern std::map<std::string, PvarDefinition*> parametrizedVariableDefinitionsMap; // Map for storing definition of ParametrizedVariables
extern std::map<std::string, PvarExpression*> parametrizedVariableMap; // Map for storing ParametrizedVariables as expressions

// Map for storing defined objects
extern std::map<std::string, Object*> objectMap;

// Map for storing defined types
extern std::map<std::string, Type*> typeMap;

ParametrizedVariable* getParametrizedVariableFromPvarDefinition(std::string name);

#endif
