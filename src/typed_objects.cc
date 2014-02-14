#include "typed_objects.h"

#include "utils/string_utils.h"

#include <iostream>
#include <cstdlib>

using namespace std;

/*****************************************************************
                              Type
*****************************************************************/

void Type::print(ostream& out) {
    out << name << endl;
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

void BoolType::printDomain(ostream& out) {
    out << "0: false" << endl << "1: true" << endl;
}

void IntType::printDomain(ostream& out) {
    out << "N" << endl;
}

void RealType::printDomain(ostream& out) {
    out << "R" << endl;
}

ObjectType::ObjectType(string Name, ObjectType* SuperType) :Type(Name, Type::OBJECT), superType(SuperType) {
    assert(superType);
}

void ObjectType::print(ostream& out) {
    if(this != objectRootInstance() && this != enumRootInstance()) {
        assert(superType);
        out << name << " : " << superType->name << endl;
    } else {
        out << name << endl;
    }
}

void ObjectType::printDomain(ostream& out) {
    for(unsigned int i = 0; i < domain.size(); ++i) {
        out << i << ": " << domain[i]->name << endl;
    }
}

double ObjectType::valueStringToDouble(string& val) {
    for(unsigned int i = 0; i < domain.size(); ++i) {
        if(domain[i]->name == val) {
            return i;
        }
    }
    assert(false);
    return -1.0;
}

/*****************************************************************
                             Object
*****************************************************************/

void Object::getObjectTypes(vector<ObjectType*>& objectTypes) {
    ObjectType* objType = type;
    while(objType != NULL) {
        objectTypes.push_back(objType);
        objType = objType->superType;
    }
}

void Object::print(ostream& out) {
    out << name << " : " << type->name << endl;
}

