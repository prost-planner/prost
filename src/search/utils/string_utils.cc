#include "string_utils.h"

#include <algorithm>
#include <cassert>
#include <sstream>

void StringUtils::replaceAll(std::string& s, char const& searchFor,
                             char const& replaceBy) {
    std::replace(s.begin(), s.end(), searchFor, replaceBy);
}

void StringUtils::trim(std::string& s) {
    if (s.empty()) {
        return;
    }

    while (s[0] == ' ' || s[0] == '\t' || s[0] == '\n' || s[0] == '\r') {
        s.erase(0, 1);
        if (s.empty()) {
            return;
        }
    }

    size_t counter = s.length() - 1;
    while (s[counter] == ' ' || s[counter] == '\t' || s[counter] == '\n' ||
           s[counter] == '\r') {
        s.erase(counter, 1);
        counter--;
    }
}

void StringUtils::replace(std::string& s, std::string& searchFor,
                          std::string& replaceBy) {
    if (s.compare(searchFor) == 0) {
        s = replaceBy;
    }
}

void StringUtils::standardizeParens(std::string& s) {
    size_t index;
    while ((index = s.find("( ")) != std::string::npos) {
        s.erase(index + 1, 1);
    }
    while ((index = s.find(" )")) != std::string::npos) {
        s.erase(index, 1);
    }
    while ((index = s.find("{ ")) != std::string::npos) {
        s.erase(index + 1, 1);
    }
    while ((index = s.find(" }")) != std::string::npos) {
        s.erase(index, 1);
    }
}

void StringUtils::standardizeCommata(std::string& s) {
    size_t index = 0;
    while ((index = s.find(",", index + 2)) != std::string::npos)
        s.replace(index, 1, " , ");
}

void StringUtils::standardizeColons(std::string& s) {
    size_t index = 0;
    while ((index = s.find(":", index + 2)) != std::string::npos) {
        s.replace(index, 1, " : ");
    }
}

void StringUtils::standardizeEqualSign(std::string& s) {
    size_t index = 0;
    while ((index = s.find("=", index + 2)) != std::string::npos) {
        assert(index > 0 && index < s.length() - 1);
        if (s[index - 1] != '<' && s[index - 1] != '>' && s[index + 1] != '>' &&
            s[index - 1] != '=' && s[index + 1] != '=' && s[index - 1] != '!' &&
            s[index - 1] != '~') {
            s.replace(index, 1, " = ");
        }
    }
}

void StringUtils::standardizeSemicolons(std::string& s) {
    size_t index;
    while ((index = s.find(" ;")) != std::string::npos) {
        s.erase(index, 1);
    }
}

void StringUtils::removeConsecutiveWhiteSpaces(std::string& s, bool doTrim) {
    if (doTrim) {
        trim(s);
    }

    size_t index;
    while ((index = s.find("  ")) != std::string::npos) {
        s.erase(index, 1);
    }
}

void StringUtils::toLowerCase(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))std::tolower);
}

void StringUtils::toUpperCase(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))std::toupper);
}

void StringUtils::firstLetterToUpper(std::string& s) {
    if (s.empty()) {
        return;
    }

    std::string tmp = s.substr(0, 1);
    toUpperCase(tmp);
    s = tmp + s.substr(1, s.length());
}

void StringUtils::embraceSubstringWithWhitespaces(std::string& s,
                                                  std::string substr) {
    std::stringstream tmp;
    size_t index = -1;
    while ((index = s.find(substr, index + 1)) != std::string::npos) {
        if (index != 0 && s[index - 1] != ' ') {
            tmp << s.substr(0, index - 1) << " " << s.substr(index);
            s = tmp.str();
            tmp.str("");
            index++;
        }
        if (index != s.length() - 1 && s[index + 1] != ' ') {
            tmp << s.substr(0, index) << " " << s.substr(index + 1);
            s = tmp.str();
            tmp.str("");
            index++;
        }
    }
}

void StringUtils::simplify(std::string& s, bool casePreserving) {
    if (!casePreserving) {
        toLowerCase(s);
    }
    replaceAll(s, '\t', ' ');
    replaceAll(s, '\r', ' ');
    replaceAll(s, '\n', ' ');
    removeConsecutiveWhiteSpaces(s);
    standardizeParens(s);
}

void StringUtils::removeTRN(std::string& s) {
    replaceAll(s, '\t', ' ');
    replaceAll(s, '\r', ' ');
    replaceAll(s, '\n', ' ');
}

void StringUtils::removeFirstAndLastCharacter(std::string& s) {
    if (s.length() <= 2) {
        s = "";
    } else {
        s = s.substr(1, s.length() - 2);
    }
}

void StringUtils::deleteCommentFromLine(std::string& line,
                                        std::string commentSign) {
    size_t index;
    if ((index = line.find(commentSign)) != std::string::npos) {
        line.resize(index);
    }
}

void StringUtils::tokenize(std::string const& s, char open_paren,
                           char close_paren, std::vector<std::string>& res) {
    int openParens = 0;
    std::stringstream tmp;
    for (size_t pos = 0; pos < s.length(); ++pos) {
        tmp << s[pos];
        if (s[pos] == open_paren) {
            openParens++;
        } else if (s[pos] == close_paren) {
            openParens--;
            if (openParens == 0) {
                std::string token = tmp.str();
                trim(token);
                res.push_back(token);
                tmp.str("");
            }
        }
    }
}

void StringUtils::split(std::string const& s, std::vector<std::string>& res,
                        std::string const& delim) {
    std::stringstream tmp;
    for (unsigned int i = 0; i < s.length(); i++) {
        if ((s.substr(i, delim.size()).compare(delim) == 0)) {
            std::string t = tmp.str();
            trim(t);
            if (!t.empty()) {
                res.push_back(t);
            }

            tmp.str("");
            i += (int)delim.size() - 1;
        } else {
            tmp << s[i];
        }
    }
    std::string t = tmp.str();
    trim(t);
    if (!t.empty()) {
        res.push_back(t);
    }
}

void StringUtils::tabString(std::string& s, int tabs) {
    std::stringstream tmp;
    for (int i = 0; i < tabs; ++i) {
        tmp << "\t";
    }
    s = tmp.str();
}

void StringUtils::nextParamValuePair(std::string& desc, std::string& param,
                                     std::string& value) {
    std::stringstream tmp;
    StringUtils::trim(desc);
    assert(desc[0] == '-');
    int index = 0;
    while (desc[index] != ' ') {
        tmp << desc[index++];
    }
    param = tmp.str();
    tmp.str("");

    assert(desc[index] == ' ');
    desc = desc.substr(index, desc.size());
    StringUtils::trim(desc);

    if (desc[0] == '[') {
        tmp << desc[0];
        index = 1;
        int openParens = 1;
        while (openParens > 0) {
            if (desc[index] == '[') {
                ++openParens;
            } else if (desc[index] == ']') {
                --openParens;
            }
            tmp << desc[index++];
        }
    } else {
        index = 0;
        while (index < desc.size() && desc[index] != ' ') {
            tmp << desc[index++];
        }
    }

    value = tmp.str();
    tmp.str("");

    assert(index == desc.size() || desc[index] == ' ');
    if (index < desc.size()) {
        desc = desc.substr(index, desc.size());
    } else {
        desc = "";
    }
    StringUtils::trim(desc);
}

bool StringUtils::startsWith(std::string const& s, std::string const& prefix) {
    std::string tmp = s.substr(0, prefix.length());
    return prefix == tmp;
}
