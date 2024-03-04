%{
#include<stdbool.h>
#include"syntax.tab.h"
bool lexical_error = false;
int yycolumn = 1;
int yylex();
void mysterious_characters_error();
extern YYSTYPE yylval; 
extern int yyerror_line;
struct node* add_node(char* name, int isint, char* val, bool isterminal, int line);
#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;
%}


%option yylineno
DIGIT [0-9]
SEMI  ;
COMMA  ,
ASSIGNOP  =
RELOP  >|<|>=|<=|==|!=
PLUS  \+
MINUS  -
STAR  \*
DIV  \/
AND  &&
OR  \|\|
DOT  \.
NOT  !
TYPE  int|float
LP  \(
RP  \)
LB  \[
RB  \]
LC  \{
RC  \}
STRUCT  struct
RETURN  return
IF  if
ELSE  else
WHILE  while
INT 0|([1-9]{DIGIT}*)
FLOAT {DIGIT}+{DOT}{DIGIT}+
ID [A-Za-z_][A-Za-z0-9_]*
BASE ({DOT}{DIGIT}+|{DIGIT}+{DOT}|{DIGIT}+{DOT}{DIGIT}+)
EXPONENT {BASE}[Ee][+-]?{DIGIT}+
%%

"/*"   {char c;
    int meet = 0;
    int flag = 0;
    while((c=input())!='\0'){
        if(c==EOF) break;
        else if(c=='*') meet=1;
        if(meet) {
            if(c=='/') {flag=1; break;}
            else if(c!='*') meet=0;
        }
    }
    if(!flag) mysterious_characters_error();}
"*/"   {mysterious_characters_error();}
"//"   {char a=input();while(a!='\n')a=input();}

{SEMI}  {yylval.node = add_node("SEMI", 5, NULL, true, -1); return (SEMI);}
{COMMA}  {yylval.node = add_node("COMMA", 5, NULL, true, -1); return (COMMA);}
{ASSIGNOP}  {yylval.node = add_node("ASSIGNOP", 5, NULL, true, -1); return (ASSIGNOP);}
{RELOP}  {yylval.node = add_node("RELOP", 5, yytext, true, -1); return (RELOP);}
{PLUS}  {yylval.node = add_node("PLUS", 5, NULL, true, -1); return (PLUS);}
{MINUS}  {yylval.node = add_node("MINUS", 5, NULL, true, -1); return (MINUS);}
{STAR}  {yylval.node = add_node("STAR", 5, NULL, true, -1); return (STAR);}
{DIV}  {yylval.node = add_node("DIV", 5, NULL, true, -1); return (DIV);}
{AND}  {yylval.node = add_node("AND", 5, NULL, true, -1); return (AND);}
{OR}  {yylval.node = add_node("OR", 5, NULL, true, -1); return (OR);}
{DOT}  {yylval.node = add_node("DOT", 5, NULL, true, -1); return (DOT);}
{NOT}  {yylval.node = add_node("NOT", 5, NULL, true, -1); return (NOT);}
{TYPE}  {yylval.node = add_node("TYPE", 3, yytext, true, -1); return (TYPE);}
{LP}  {yylval.node = add_node("LP", 5, NULL, true, -1); return (LP);}
{RP}  {yylval.node = add_node("RP", 5, NULL, true, -1); return (RP);}
{LB}  {yylval.node = add_node("LB", 5, NULL, true, -1); return (LB);}
{RB}  {yylval.node = add_node("RB", 5, NULL, true, -1); return (RB);}
{LC}  {yylval.node = add_node("LC", 5, NULL, true, -1); return (LC);}
{RC}  {yylval.node = add_node("RC", 5, NULL, true, -1); return (RC);}
{STRUCT}  {yylval.node = add_node("STRUCT", 5, NULL, true, -1); return (STRUCT);}
{RETURN}  {yylval.node = add_node("RETURN", 5, NULL, true, -1); return (RETURN);}
{IF}  {yylval.node = add_node("IF", 5, NULL, true, -1); return (IF);}
{ELSE}  {yylval.node = add_node("ELSE", 5, NULL, true, -1); return (ELSE);}
{WHILE}  {yylval.node = add_node("WHILE", 5, NULL, true, -1); return (WHILE);}
{INT} {yylval.node = add_node("INT", 0, yytext, true, -1); return (INT);}
{FLOAT} {yylval.node = add_node("FLOAT", 1, yytext, true, -1); return (FLOAT);}
{ID} {yylval.node = add_node("ID", 2, yytext, true, -1); return (ID);}
{EXPONENT}  {mysterious_characters_error();return (INT);}
({INT}\.)|(\.{INT})  {mysterious_characters_error();return (FLOAT);}
(0{DIGIT}+)  {mysterious_characters_error();return (INT);}
[\t\f\r\v ]	{}
"\n" {yycolumn = 1;}
.  {mysterious_characters_error();}

%%
void mysterious_characters_error()
{
    lexical_error = true;
    if(yyerror_line == yylineno) return;
    yyerror_line = yylineno;
    //printf("A%d\n",yylineno);
    printf("Error type A at Line %d: Mysterious characters %s \n", yylineno, yytext);
}