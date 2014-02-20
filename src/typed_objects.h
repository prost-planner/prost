#ifndef TYPED_OBJECTS_H
#define TYPED_OBJECTS_H

#include "unprocessed_planning_task.h"

class Object;

class Type {
public:
    static Type* typeFromName(std::string& typeName, UnprocessedPlanningTask* task);

    std::string name;
    enum TypeType {
        BOOL,
        INT,
        REAL,
        OBJECT
    } type;

    // If the domain is empty, it is infinte
    std::vector<Object*> domain;

    virtual ~Type() {}
    virtual void print(std::ostream& out);
    virtual void printDomain(std::ostream& out) = 0;
    virtual double valueStringToDouble(std::string& val);

    Type(std::string _name, TypeType _type) :
        name(_name), type(_type) {}
};

class BoolType : public Type {
public:
    static BoolType* instance() {
        static BoolType* inst = new BoolType();
        return inst;
    }

    void printDomain(std::ostream& out);
    double valueStringToDouble(std::string& val);

    ~BoolType() {}

private:
    BoolType() :
        Type("bool", Type::BOOL) {}
};

class IntType : public Type {
public:
    static IntType* instance() {
        static IntType* inst = new IntType();
        return inst;
    }

    ~IntType() {}

    void printDomain(std::ostream& out);

private:
    IntType() :
        Type("int", Type::INT) {}
};

class RealType : public Type {
public:
    static RealType* instance() {
        static RealType* inst = new RealType();
        return inst;
    }

    ~RealType() {}

    void printDomain(std::ostream& out);

private:
    RealType() :
        Type("real", Type::REAL) {}
};

class ObjectType : public Type {
public:
    static ObjectType* objectRootInstance() {
        static ObjectType* inst = new ObjectType("object");
        return inst;
    }

    static ObjectType* enumRootInstance() {
        static ObjectType* inst = new ObjectType("enum");
        return inst;
    }

    ObjectType(std::string _name, ObjectType* _superType) :
        Type(_name, Type::OBJECT), superType(_superType) {}
    ~ObjectType() {}

    ObjectType* superType;

    void print(std::ostream& out);
    void printDomain(std::ostream& out);
    double valueStringToDouble(std::string& val);

private:
    ObjectType(std::string _name) :
        Type(_name, Type::OBJECT), superType(NULL) {}
};

#endif
