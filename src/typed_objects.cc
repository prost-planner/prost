#include "typed_objects.h"

#include "utils/string_utils.h"

#include <iostream>
#include <cstdlib>

using namespace std;

/*****************************************************************
                              Type
*****************************************************************/

void Type::parse(string& desc, UnprocessedPlanningTask* task) {
    size_t cutPos = desc.find(":");
    assert(cutPos != string::npos);

    string name = desc.substr(0,cutPos);
    StringUtils::trim(name);

    string rest = desc.substr(cutPos+1, desc.length());
    StringUtils::trim(rest);

    if(rest.find("{") == 0) {
        assert(rest[rest.length()-1] == '}');
        rest = rest.substr(1,rest.length()-2);

        ObjectType* newType = new ObjectType(name, ObjectType::enumRootInstance());
        task->addObjectType(newType);

        vector<string> valsAsString;
        StringUtils::split(rest, valsAsString, ",");
        for(unsigned int i = 0; i < valsAsString.size(); ++i) {
            assert(valsAsString[i][0] == '@');
            task->addObject(new Object(valsAsString[i], newType));
        }
    } else {
        task->addObjectType(new ObjectType(name, task->getObjectType(rest)));
    }
}

void Type::print() {
    cout << name << endl;
}

double Type::valueStringToDouble(string& val) {
    return atof(val.c_str());
}

Type* Type::typeFromName(string& typeName, UnprocessedPlanningTask* task) {//DomainToken* domain) {
    if((typeName == BoolType::instance()->name)) {
        return BoolType::instance();
    } else if(typeName == IntType::instance()->name) {
        return IntType::instance();
    } else if(typeName == RealType::instance()->name) {
        return RealType::instance();
    } //else { if(task->objectTypes.find(typeName) != task->objectTypes.end()) {
    //    return task->objectTypes[typeName];
    //}
    return task->getObjectType(typeName);
}

double BoolType::valueStringToDouble(string& val) {
    if(val == "true" || val == "1.0") {
        return 1.0;
    }
    assert(val == "false" || val == "0.0");
    return 0.0;
}

ObjectType::ObjectType(string Name, ObjectType* SuperType) :Type(Name, Type::OBJECT), superType(SuperType) {
    assert(superType);
}

void ObjectType::print() {
    if(this != objectRootInstance() && this != enumRootInstance()) {
        assert(superType);
        cout << name << " : " << superType->name << endl;
    } else {
        cout << name << endl;
    } 
}

double ObjectType::valueStringToDouble(string& /*val*/) {
    assert(false);
    return -1.0;
}

/*****************************************************************
                             Object
*****************************************************************/

void Object::parse(string& desc, UnprocessedPlanningTask* task) {
    size_t cutPos = desc.find(":");
    assert(cutPos != string::npos);

    string type = desc.substr(0,cutPos);
    StringUtils::trim(type);
    //assert(task->objectTypes.find(type) != task->objectTypes.end());
    
    string objs = desc.substr(cutPos+1,desc.length());
    StringUtils::trim(objs);
    assert(objs[0] == '{');
    assert(objs[objs.length()-1] == '}');
    objs = objs.substr(1,objs.length()-2);

    vector<string> objectNames;
    StringUtils::split(objs, objectNames, ",");
    for(unsigned int i = 0; i < objectNames.size(); ++i) {
        task->addObject(new Object(objectNames[i],task->getObjectType(type)));
    }
}

void Object::getObjectTypes(vector<ObjectType*>& objectTypes) {
    ObjectType* objType = type;
    while(objType != NULL) {
        objectTypes.push_back(objType);
        objType = objType->superType;
    }
}

void Object::print() {
    cout << name << " : " << type->name << endl;
}

