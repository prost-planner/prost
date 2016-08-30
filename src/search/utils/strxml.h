#ifndef STRXML_H
#define STRXML_H

#include <iostream>
#include <map>
#include <string>
#include <vector>

struct XMLNode;

typedef std::pair<std::string, std::string> str_pair;
typedef std::vector<str_pair> str_pair_vec;
typedef std::vector<std::string> str_vec;
typedef std::map<std::string, std::string> str_str_map;
typedef std::vector<const XMLNode*> node_vec;

struct XMLNode {
    static const XMLNode* readNode(int fd);

    virtual ~XMLNode() {}

    virtual const XMLNode* getChild(int /*i*/) const {
        return nullptr;
    }

    virtual const XMLNode* getChild(const std::string& /*name*/) const {
        return nullptr;
    }

    virtual int size() const {
        return 0;
    }

    virtual std::string getText() const = 0;
    virtual const std::string& getName() const = 0;
    virtual const std::string& getParam(std::string name) const = 0;
    virtual void print(std::ostream& os) const = 0;

    bool dissect(const std::string& child, std::string& destination) const;

protected:
    friend std::ostream& operator<<(std::ostream& os, const XMLNode& xn);
};

std::ostream& operator<<(std::ostream& os, const XMLNode& xn);
std::ostream& operator<<(std::ostream& os, const XMLNode* xn);

struct XMLText : public XMLNode {
    XMLText(const std::string& _text) : text(_text) {}

    std::string getText() const;
    const std::string& getName() const;
    const std::string& getParam(std::string name) const;

    void print(std::ostream& os) const;

    std::string text;
};

struct XMLParent : public XMLNode {
    XMLParent(const std::string& _name) : name(_name) {}
    virtual ~XMLParent();

    const XMLNode* getChild(int i) const;
    const XMLNode* getChild(const std::string& name) const;
    int size() const;
    std::string getText() const;
    const std::string& getName() const;
    const std::string& getParam(std::string name) const;

    void print(std::ostream& os) const;

    std::string name;
    str_str_map params;
    node_vec children;
};

#endif
