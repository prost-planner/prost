%option noyywrap
%option yylineno
%option noinput
%option nounput

%{
#include <iostream>
#include <cstdlib>
#include <vector>

#include "logical_expressions.h"
#include "rddl.h"

void lex_error(char c) {
    std::cerr << "\033[1;31mLexical ERROR! \033[0m Unknown char " << c
              << std::endl;
    exit(EXIT_FAILURE);
}

void yyerror (std::string message) {
    extern int yylineno;
    std::cerr << "\033[1;31mERROR! \033[0m Unknown token: " << yytext
              << " at line " << yylineno << ": "  << message << std::endl;
    exit(EXIT_FAILURE);
}

#include "parser.tab.hh"

%}

DIGIT [0-9]
ALPHA [A-Za-z]
WHITE_SPACE_CHAR [\r\n\t\b ]

%%
\<=                                                     { return lessOrEqual_token; }
>=                                                      { return greaterOrEqual_token; }
==                                                      { return equal_token; }
\<=>                                                    { return equivalent_token;}
=>                                                      { return imply_token;}
~=                                                      { return nonEqual_token; }
!=                                                      { return nonEqual_token; }
[+\-*/();:~\^\|=<>{}\]\[?,\$&_]                         { return *yytext; }
true                                                    { yylval.str = new std::string(yytext); return true_token;  }
false                                                   { yylval.str = new std::string(yytext); return false_token; }
domain                                                  { return domain_token; }
instance                                                { return instance_token; }
objects                                                 { return objects_token; }
horizon                                                 { return horizon_token; }
discount                                                { return discount_token; }
init-state                                              { return init_state_token; }
requirements                                            { return requirements_token; }
state-action-constraints                                { return state_action_constraints_token; }
action-preconditions                                    { return action_preconditions_token; }
state-invariants                                        { return state_invariants_token; }
max-nondef-actions                                      { return max_nondef_actions_token; }
terminate-when                                          { return terminate_when_token; }
types                                                   { return types_token; }
pvariables                                              { return variables_token; }
cpfs                                                    { return cpfs_token; }
cdfs                                                    { return cdfs_token; }
reward                                                  { return reward_token; }
bool                                                    { yylval.str = new std::string(yytext); return bool_token; }
int                                                     { yylval.str = new std::string(yytext); return integer_token; }
real                                                    { yylval.str = new std::string(yytext); return real_token; }
object                                                  { yylval.str = new std::string(yytext); return object_token; }
default                                                 { yylval.str = new std::string(yytext); return default_token; }
level                                                   { yylval.str = new std::string(yytext); return level_token; }
state\-fluent                                           { yylval.str = new std::string(yytext); return state_fluent_token; }
interm\-fluent                                          { yylval.str = new std::string(yytext); return intermediate_token; }
non\-fluent                                             { yylval.str = new std::string(yytext); return non_fluent_token; }
non\-fluents                                            { return non_fluents_token; }
derived\-fluent                                         { yylval.str = new std::string(yytext); return derived_fluent_token; }
observ\-fluent                                          { yylval.str = new std::string(yytext); return observ_fluent_token; }
action-fluent                                           { yylval.str = new std::string(yytext); return action_fluent_token; }
pos\-inf                                                { return positive_infinity_token ;}
neg\-inf                                                { return negative_infinity_token ;}
forall                                                  { return forall_token; }
exists                                                  { return exists_token; }
sum                                                     { return sum_token; }
product                                                 { return product_token; }
prod                                                    { return product_token; }
if                                                      { return if_token; }
then                                                    { return then_token; }
else                                                    { return else_token; }
switch                                                  { return switch_token; }
case                                                    { return case_token; }
otherwise                                               { return otherwise_token; }
KronDelta                                               { return kronDelta_token; }
DiracDelta                                              { return diracDelta_token; }
Uniform                                                 { return uniform_token; }
Bernoulli                                               { return bernoulli_token; }
Discrete                                                { return discrete_token; }
Normal                                                  { return normal_token; }
Poisson                                                 { return poisson_token; }
Exponential                                             { return exponential_token; }
exp                                                     { return exponential_token; }
Weibull                                                 { return weibull_token; }
Gama                                                    { return gama_token; }
Multinomial                                             { return multinomial_token; }
Dirichelt                                               { return dirichlet_token; }
{WHITE_SPACE_CHAR}+                                     { /* Ignoring white spaces.*/}
{DIGIT}*\.{DIGIT}+                                      { yylval.d = atof(yytext); return doubleNum_token;}
{DIGIT}+                                                { yylval.i = atoi(yytext); return intNum_token;}
{ALPHA}(({ALPHA}|{DIGIT}|-|_)*({ALPHA}|{DIGIT}))?(')?   { yylval.str = new std::string(yytext); return id_token; }
"?"({ALPHA}|{DIGIT}|-|_)*({ALPHA}|{DIGIT})              { yylval.str = new std::string(yytext); return variable_token; }
"@"({ALPHA}|{DIGIT}|-|_)*({ALPHA}|{DIGIT})              { yylval.str = new std::string(yytext); return enum_token; }
\/\/[^\r\n]*                                            { /* Ignore comments. */}
.                                                       { lex_error(*yytext); }
%%
