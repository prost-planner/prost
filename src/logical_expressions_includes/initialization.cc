/*****************************************************************
                        Constructors
*****************************************************************/

UninstantiatedVariable::UninstantiatedVariable(VariableDefinition* _parent, std::vector<std::string> _params) :
    parent(_parent), params(_params) {
    assert(params.size() == parent->params.size());
    name = parent->name + "(";
    for(unsigned int i = 0; i < params.size(); ++i) {
        name += params[i];
        if(i != params.size()-1) {
            name += ", ";
        }
    }
    name += ")";
}

AtomicLogicalExpression::AtomicLogicalExpression(VariableDefinition* _parent, std::vector<Object*> _params, double _initialValue) :
    parent(_parent), params(_params), initialValue(_initialValue), index(-1) {

    assert(_parent->params.size() == params.size());
    name = _parent->name + "(";
    for(unsigned int i = 0; i < params.size(); ++i) {
        name += params[i]->name;
        if(i != params.size()-1) {
            name += ", ";
        }
    }
    name += ")";
}

Quantifier::Quantifier(std::vector<LogicalExpression*>& _exprs) {
    assert(_exprs.size() == 2);
    parameterDefsSet = dynamic_cast<ParameterDefinitionSet*>(_exprs[0]);
    assert(parameterDefsSet);
    expr = _exprs[1];
}


/*****************************************************************
                       Parser Methods
*****************************************************************/

void VariableDefinition::parse(string& desc, UnprocessedPlanningTask* task) {
    size_t cutPos = desc.find(":");
    assert(cutPos != string::npos);

    string nameAndParams = desc.substr(0,cutPos);
    StringUtils::trim(nameAndParams);
    string rest = desc.substr(cutPos+1);
    StringUtils::trim(rest);

    string name;
    vector<ObjectType*> params;

    if(nameAndParams[nameAndParams.length()-1] != ')') {
        name = nameAndParams;
    } else {
        cutPos = nameAndParams.find("(");
        name = nameAndParams.substr(0,cutPos);

        string allParams = nameAndParams.substr(cutPos+1,nameAndParams.length()-cutPos-2);
        vector<string> allParamsAsString;
        StringUtils::split(allParams, allParamsAsString, ",");
        for(unsigned int i = 0; i < allParamsAsString.size(); ++i) {
            params.push_back(task->getObjectType(allParamsAsString[i]));
        }
    }

    assert(rest[0] == '{');
    assert(rest[rest.length()-1] == '}');
    rest = rest.substr(1,rest.length()-2);
    vector<string> optionals;
    StringUtils::split(rest,optionals,",");

    assert(optionals.size() == 3);
    VariableDefinition::VariableType varType = VariableDefinition::NON_FLUENT;
    if(optionals[0] == "state-fluent") {
        varType = VariableDefinition::STATE_FLUENT;
    } else if(optionals[0] == "action-fluent") {
        varType = VariableDefinition::ACTION_FLUENT;
    } else if(optionals[0] == "interm-fluent") {
        varType = VariableDefinition::INTERM_FLUENT;
    } else {
        assert(optionals[0] == "non-fluent");
    }

    Type* type = Type::typeFromName(optionals[1], task);
    assert(type);

    double defaultVal = -1.0;
    string defaultValString;
    int level = 0;

    switch(varType) {
    case VariableDefinition::NON_FLUENT:
    case VariableDefinition::STATE_FLUENT:
    case VariableDefinition::ACTION_FLUENT:  
        assert(optionals[2].find("default =") == 0);
        defaultValString = optionals[2].substr(9,optionals[2].length());
        StringUtils::trim(defaultValString);
        defaultVal = type->valueStringToDouble(defaultValString);
        break;
    case VariableDefinition::INTERM_FLUENT:
        assert(optionals[2].find("level =") == 0);
        optionals[2] = optionals[2].substr(7,optionals[2].length());
        StringUtils::trim(optionals[2]);
        level = atoi(optionals[2].c_str());
        break;
    }

    task->addVariableDefinition(new VariableDefinition(name,params,varType,type,defaultVal,level));
}

void AtomicLogicalExpression::parse(string& desc, UnprocessedPlanningTask* task) {
    if(desc.find("~") == 0) {
        assert(desc.find("=") == string::npos);
        desc = desc.substr(1) + " = 0";
    } else if(desc.find("=") == string::npos) {
        desc += " = 1";
    }
    size_t cutPos = desc.find("=");
    assert(cutPos != string::npos);

    string nameAndParams = desc.substr(0,cutPos);
    StringUtils::trim(nameAndParams);

    string valString = desc.substr(cutPos+1);
    StringUtils::trim(valString);

    string name;
    vector<Object*> paramsAsObjects;
    if((cutPos = nameAndParams.find("(")) == string::npos) {
        name = nameAndParams;
    } else {
        name = nameAndParams.substr(0,cutPos);
        string paramsAsString = nameAndParams.substr(cutPos+1);
        assert(paramsAsString[paramsAsString.size()-1] == ')');
        paramsAsString = paramsAsString.substr(0,paramsAsString.size()-1);
        vector<string> paramStrings;
        StringUtils::split(paramsAsString,paramStrings,",");
        for(unsigned int i = 0; i < paramStrings.size(); ++i) {
            paramsAsObjects.push_back(task->getObject(paramStrings[i]));
        }
    }
    VariableDefinition* parent = task->getVariableDefinition(name);
    switch(parent->variableType) {
    case VariableDefinition::STATE_FLUENT:
        task->addStateFluent(new StateFluent(parent,paramsAsObjects,atof(valString.c_str())));
        break;
    case VariableDefinition::INTERM_FLUENT:
        // Interm fluents are currently not supported
        assert(false);
        break;
    case VariableDefinition::ACTION_FLUENT:
        assert(false); //This is only called to parse the initial state and an action fluent just doesn't make sense there!
        break;
    case VariableDefinition::NON_FLUENT:
        task->addNonFluent(new NonFluent(parent,paramsAsObjects,atof(valString.c_str())));
        break;
    }
}
