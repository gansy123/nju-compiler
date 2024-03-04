%{
#include<stdio.h>
#include<stdbool.h>
#include<stdarg.h>
#include"lex.yy.c"
struct node{
    int line;
    char name[32];
    int child_num;
    bool isterminal;
    int isint;
    union{
        unsigned int val_int;
        float val_float;
        char id[32];
    }val;
    struct node* left;
    struct node* right;
};
struct node* root;
bool syntax_error = false;
void yyerror(const char *msg);
extern int yylineno;
extern char* yytext;
int yyerror_line = -1;
void print_tree(struct node* root, int depth);
%}

%locations
%union {
    struct node* node;
}

/* High-level Definitions */
%type <node> Program
%type <node> ExtDefList
%type <node> ExtDef
%type <node> ExtDecList
/* Specifiers */
%type <node> Specifier
%type <node> StructSpecifier
%type <node> OptTag
%type <node> Tag
/* Declarators */
%type <node> VarDec
%type <node> FunDec
%type <node> VarList
%type <node> ParamDec
/* Statements */
%type <node> CompSt
%type <node> StmtList
%type <node> Stmt
/* Local Definitions */
%type <node> DefList
%type <node> Def
%type <node> DecList
%type <node> Dec
/* Expressions */
%type <node> Exp
%type <node> Args
%start Program
/* declared tokens */
%token <node> INT FLOAT ID
%token <node> SEMI COMMA DOT
%token <node> ASSIGNOP RELOP PLUS MINUS STAR DIV
%token <node> AND OR NOT
%token <node> LP RP LB RB LC RC
%token <node> TYPE STRUCT RETURN
%token <node> IF ELSE WHILE

%right ASSIGNOP
%left  OR
%left  AND
%left  RELOP
%left  PLUS 
%left  MINUS
%left  STAR
%left  DIV
%right NOT
%right UMINUS
%left  DOT 
%left  LP 
%left  RP 
%left  LB 
%left  RB
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%%
/* High-level Definitions */
Program: ExtDefList                                 {$$ = add_node("Program", 4, NULL, false, @$.first_line); root = $$; construct($$, 1, $1);}
;               
ExtDefList: ExtDef ExtDefList                       {$$ = add_node("ExtDefList", 4, NULL, false, @$.first_line); construct($$, 2, $1, $2);}
    | /* empty */                                   {$$ = NULL;}
;               
ExtDef: Specifier ExtDecList SEMI                   {$$ = add_node("ExtDef", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | Specifier SEMI                                {$$ = add_node("ExtDef", 4, NULL, false, @$.first_line); construct($$, 2, $1, $2);}
    | Specifier FunDec CompSt                       {$$ = add_node("ExtDef", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}                    
    | Specifier FunDec SEMI                         {$$ = add_node("ExtDef", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}                    


    | error CompSt                                  {}
    | error SEMI                                    {}
;               
ExtDecList: VarDec                                  {$$ = add_node("ExtDecList", 4, NULL, false, @$.first_line); construct($$, 1, $1);}
    | VarDec COMMA ExtDecList                       {$$ = add_node("ExtDecList", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
;
/* Specifiers */
Specifier: TYPE                                     {$$ = add_node("Specifier", 4, NULL, false, @$.first_line); construct($$, 1, $1);}
    | StructSpecifier                               {$$ = add_node("Specifier", 4, NULL, false, @$.first_line); construct($$, 1, $1);}
;
StructSpecifier: STRUCT OptTag LC DefList RC        {$$ = add_node("StructSpecifier", 4, NULL, false, @$.first_line); construct($$, 5, $1, $2, $3, $4, $5);}
    | STRUCT Tag                                    {$$ = add_node("StructSpecifier", 4, NULL, false, @$.first_line); construct($$, 2, $1, $2);}
;   
OptTag: ID                                          {$$ = add_node("OptTag", 4, NULL, false, @$.first_line); construct($$, 1, $1);
}
    | /* empty */                                   {$$ = NULL;}
;
Tag: ID                                             {$$ = add_node("Tag", 4, NULL, false, @$.first_line); construct($$, 1, $1);}
;
/* Declarators */
VarDec: ID                                          {$$ = add_node("VarDec", 4, NULL, false, @$.first_line); construct($$, 1, $1);}
    | VarDec LB INT RB                              {$$ = add_node("VarDec", 4, NULL, false, @$.first_line); construct($$, 4, $1, $2, $3, $4);}
;               
FunDec: ID LP VarList RP                            {$$ = add_node("FunDec", 4, NULL, false, @$.first_line); construct($$, 4, $1, $2, $3, $4);}
    | ID LP RP                                      {$$ = add_node("FunDec", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
;
VarList: ParamDec COMMA VarList                     {$$ = add_node("VarList", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | ParamDec                                      {$$ = add_node("VarList", 4, NULL, false, @$.first_line); construct($$, 1, $1);}
;
ParamDec: Specifier VarDec                          {$$ = add_node("ParamDec", 4, NULL, false, @$.first_line); construct($$, 2, $1, $2);}
;
/* Statements */
CompSt: LC DefList StmtList RC                      {$$ = add_node("CompSt", 4, NULL, false, @$.first_line); construct($$, 4, $1, $2, $3, $4);}
;
StmtList: Stmt StmtList                             {$$ = add_node("StmtList", 4, NULL, false, @$.first_line); construct($$, 2, $1, $2);}
    | /* empty */                                   {$$ = NULL;}
;
Stmt: Exp SEMI                                      {$$ = add_node("Stmt", 4, NULL, false, @$.first_line); construct($$, 2, $1, $2);}
    | CompSt                                        {$$ = add_node("Stmt", 4, NULL, false, @$.first_line); construct($$, 1, $1);}
    | RETURN Exp SEMI                               {$$ = add_node("Stmt", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE       {$$ = add_node("Stmt", 4, NULL, false, @$.first_line); construct($$, 5, $1, $2, $3, $4, $5);}
    | IF LP Exp RP Stmt ELSE Stmt                   {$$ = add_node("Stmt", 4, NULL, false, @$.first_line); construct($$, 7, $1, $2, $3, $4, $5, $6, $7);}
    | WHILE LP Exp RP Stmt                          {$$ = add_node("Stmt", 4, NULL, false, @$.first_line); construct($$, 5, $1, $2, $3, $4, $5);}                              

    | error SEMI                                    {}
    | error Stmt                                    {}
;
/* Local Definitions */
DefList: Def DefList                                {$$ = add_node("DefList", 4, NULL, false, @$.first_line); construct($$, 2, $1, $2);
}
    | /* empty */                                   {$$ = NULL;}
;
Def: Specifier DecList SEMI                         {$$ = add_node("Def", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}                  
    | Specifier DecList error SEMI                  {}
    | Specifier error SEMI                          {}
;
DecList: Dec                                        {$$ = add_node("DecList", 4, NULL, false, @$.first_line); construct($$, 1, $1);}
    | Dec COMMA DecList                             {$$ = add_node("DecList", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
;
Dec: VarDec                                         {$$ = add_node("Dec", 4, NULL, false, @$.first_line); construct($$, 1, $1);}
    | VarDec ASSIGNOP Exp                           {$$ = add_node("Dec", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
;
/* Expressions */
Exp: Exp ASSIGNOP Exp                               {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | Exp AND Exp                                   {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | Exp OR Exp                                    {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | Exp RELOP Exp                                 {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | Exp PLUS Exp                                  {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | Exp MINUS Exp                                 {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | Exp STAR Exp                                  {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | Exp DIV Exp                                   {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | LP Exp RP                                     {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | MINUS Exp %prec UMINUS                        {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 2, $1, $2);}
    | NOT Exp                                       {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 2, $1, $2);}
    | ID LP Args RP                                 {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 4, $1, $2, $3, $4);}
    | ID LP RP                                      {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | Exp LB Exp RB                                 {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 4, $1, $2, $3, $4);}
    | Exp DOT ID                                    {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | ID                                            {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 1, $1);}
    | INT                                           {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 1, $1);}
    | FLOAT                                         {$$ = add_node("Exp", 4, NULL, false, @$.first_line); construct($$, 1, $1);}

;
Args: Exp COMMA Args                                {$$ = add_node("Args", 4, NULL, false, @$.first_line); construct($$, 3, $1, $2, $3);}
    | Exp                                           {$$ = add_node("Args", 4, NULL, false, @$.first_line); construct($$, 1, $1);}
;
%%

void yyerror(const char *msg) {
    if(yyerror_line == yylineno) return;
    yyerror_line = yylineno;
    syntax_error = true;
    //printf("B%d\n",yylineno);
    printf("Error type B at Line %d: %s %s\n",yylineno, msg, yytext);
}

struct node* add_node(char* name, int isint, char* val, bool isterminal, int line)
{
    struct node* new = (struct node*)malloc(sizeof(struct node));
    new->isint = isint;
    strcpy(new->name, name);
    if(val != NULL)
        strcpy(new->val.id, val);
    else
        strcpy(new->val.id, "");
    new->isterminal = isterminal;
    new->left = NULL;
    new->right = NULL;
    if (!isterminal)
        new->line = line;
    return new;
}


void print_tree(struct node* root, int depth)
{
    if(root == NULL)
    {
        return ;
    }
    for(int i = 0; i < depth; i++)
    {
        printf("  ");
    }
    printf("%s", root->name);
    if(root->isterminal)
    {
        if(root->isint == 0)
        {
            printf(": %d", atoi(root->val.id));
        }
        else if(root->isint == 1)
        {
            printf(": %lf", atof(root->val.id));
        }
        else if(root->isint == 2)
        {
            printf(": %s", root->val.id);
        }
        else if(root->isint == 3)
        {
            printf(": %s", root->val.id);
        }
    }
    else
    {
        printf(" (%d)", root->line);
    }
    printf("\n");
    print_tree(root->left, depth + 1);
    print_tree(root->right, depth);
}

void construct(struct node* parent, int num,...){
    va_list brother;
    va_start(brother, num);
    struct node* temp = NULL;
    while(temp == NULL)
        temp = va_arg(brother, struct node*);
    parent->left = temp;
    parent->child_num = num;
    for(int i = 1; i < num; i++){
    temp->right = va_arg(brother, struct node*);
    if(temp->right != NULL){
        temp = temp->right;
        }
    }
}
