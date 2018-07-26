#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <sstream>
#include <string>
#include <vector>

class StringUtils {
public:
    static void replaceAll(std::string& s, char const& searchFor,
                           char const& replaceBy);
    static void replace(std::string& s, std::string& searchFor,
                        std::string& replaceBy);
    static void trim(std::string& s);
    static void standardizeParens(std::string& s);
    static void standardizeCommata(std::string& s);
    static void standardizeColons(std::string& s);
    static void standardizeEqualSign(std::string& s);
    static void standardizeSemicolons(std::string& s);
    static void removeConsecutiveWhiteSpaces(std::string& s,
                                             bool doTrim = true);
    static void toLowerCase(std::string& s);
    static void toUpperCase(std::string& s);
    static void firstLetterToUpper(std::string& s);
    static void simplify(std::string& s, bool casePreserving = true);
    static void embraceSubstringWithWhitespaces(std::string& s,
                                                std::string substr);
    static void deleteCommentFromLine(std::string& line,
                                      std::string commentSign);
    static void tokenize(std::string const& s, char open_paren,
                         char close_paren, std::vector<std::string>& res);
    static void split(std::string const& s, std::vector<std::string>& res,
                      std::string const& delim = " ");
    static void tabString(std::string& s, int tabs);
    static void removeTRN(std::string& s);
    static void removeFirstAndLastCharacter(std::string& s);
    static void nextParamValuePair(std::string& desc, std::string& param,
                                   std::string& value);
    static bool startsWith(std::string const& s, std::string const& prefix);

    template <typename T>
    static void concatenateNames(std::vector<T*>& tokens, std::string& res,
                                 char const& delimiter = '*') {
        std::stringstream s;
        for (unsigned int i = 0; i < tokens.size(); ++i) {
            s << tokens[i]->name;
            if (i != tokens.size() - 1) {
                s << delimiter;
            }
        }
        res = s.str();
    }

    static void concatenateNames2(std::vector<std::string> tokens,
                                  std::string& res,
                                  char const& delimiter = '*') {
        std::stringstream s;
        for (unsigned int i = 1; i < tokens.size(); ++i) {
            s << tokens[i];
            if (i != tokens.size() - 1) {
                s << delimiter;
            }
        }
        res = s.str();
    }
};

#endif
