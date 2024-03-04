%define api.prefix {IR_yy}
%define api.token.prefix {IR_TOKEN_}
%define parse.error verbose

%{

#ifndef CODE_MACRO_H
#define CODE_MACRO_H

#include <string.h>

// macro stringizing
#define str_temp(x) #x
#define str(x) str_temp(x)

// strlen() for string constant
#define STRLEN(CONST_STR) (sizeof(CONST_STR) - 1)

// calculate the length of an array
#define ARRLEN(arr) (int)(sizeof(arr) / sizeof(arr[0]))

// macro concatenation
#define concat_temp(x, y) x ## y
#define concat(x, y) concat_temp(x, y)
#define concat2(x, y) concat(x, y)
#define concat3(x, y, z) concat(concat(x, y), z)
#define concat4(x, y, z, w) concat3(concat(x, y), z, w)
#define concat5(x, y, z, v, w) concat4(concat(x, y), z, v, w)

// macro testing
// See https://stackoverflow.com/questions/26099745/test-if-preprocessor-symbol-is-defined-inside-macro
#define CHOOSE2nd(a, b, ...) b
#define MUX_WITH_COMMA(contain_comma, a, b) CHOOSE2nd(contain_comma a, b)
#define MUX_MACRO_PROPERTY(p, macro, a, b) MUX_WITH_COMMA(concat(p, macro), a, b)
// define placeholders for some property
#define __P_DEF_0  X,
#define __P_DEF_1  X,
#define __P_ONE_1  X,
#define __P_ZERO_0 X,
// define some selection functions based on the properties of BOOLEAN macro
#define MUXDEF(macro, X, Y)  MUX_MACRO_PROPERTY(__P_DEF_, macro, X, Y)
#define MUXNDEF(macro, X, Y) MUX_MACRO_PROPERTY(__P_DEF_, macro, Y, X)
#define MUXONE(macro, X, Y)  MUX_MACRO_PROPERTY(__P_ONE_, macro, X, Y)
#define MUXZERO(macro, X, Y) MUX_MACRO_PROPERTY(__P_ZERO_,macro, X, Y)

// test if a boolean macro is defined
#define ISDEF(macro) MUXDEF(macro, 1, 0)
// test if a boolean macro is undefined
#define ISNDEF(macro) MUXNDEF(macro, 1, 0)
// test if a boolean macro is defined to 1
#define ISONE(macro) MUXONE(macro, 1, 0)
// test if a boolean macro is defined to 0
#define ISZERO(macro) MUXZERO(macro, 1, 0)
// test if a macro of ANY type is defined
// NOTE1: it ONLY works inside a function, since it calls `strcmp()`
// NOTE2: macros defined to themselves (#define A A) will get wrong results
#define isdef(macro) (strcmp("" #macro, "" str(macro)) != 0)

// simplification for conditional compilation
#define __IGNORE(...)
#define __KEEP(...) __VA_ARGS__
// keep the code if a boolean macro is defined
#define IFDEF(macro, ...) MUXDEF(macro, __KEEP, __IGNORE)(__VA_ARGS__)
// keep the code if a boolean macro is undefined
#define IFNDEF(macro, ...) MUXNDEF(macro, __KEEP, __IGNORE)(__VA_ARGS__)
// keep the code if a boolean macro is defined to 1
#define IFONE(macro, ...) MUXONE(macro, __KEEP, __IGNORE)(__VA_ARGS__)
// keep the code if a boolean macro is defined to 0
#define IFZERO(macro, ...) MUXZERO(macro, __KEEP, __IGNORE)(__VA_ARGS__)

#define MAP(c, f) c(f)

#define ARG_1(a1, ...) a1
#define ARG_2(a1, ...) ARG_1(__VA_ARGS__)
#define ARG_3(a1, ...) ARG_2(__VA_ARGS__)
#define ARG_4(a1, ...) ARG_3(__VA_ARGS__)
#define ARG_5(a1, ...) ARG_4(__VA_ARGS__)
#define ARG_6(a1, ...) ARG_5(__VA_ARGS__)
#define ARG_7(a1, ...) ARG_6(__VA_ARGS__)
#define ARG_8(a1, ...) ARG_7(__VA_ARGS__)

#define TODO(...) do{fprintf(stderr, "TODO: File(%s) Line(%d)", __FILE__, __LINE__); exit(-1);}while(0)

#endif //CODE_MACRO_H


//
// Created by hby on 22-10-28.
//

#ifndef CODE_OBJECT_H
#define CODE_OBJECT_H

#include <macro.h>

#define CLASS_virtualTable struct{ \
        void (*teardown)(void *ptr);};

typedef struct virtualTable {
    CLASS_virtualTable
} virtualTable;

// 返回已执行init构造函数的指针, 类似于C++的 new obj 语句
#define NEW(TYPE, ...) ({                             \
            TYPE *TMP = (TYPE*)malloc(sizeof(TYPE));  \
            concat(TYPE, _init) (TMP, ##__VA_ARGS__); \
            TMP;                                      \
        })
// 从虚函数表vTable中调用函数, 指针需要解引用
#define VCALL(obj, func, ...) \
        (((obj).vTable)->func(&(obj), ##__VA_ARGS__))
// 执行teardown析构函数并free, 类似于C++的 delete obj 语句
#define DELETE(obj_ptr) \
        do { VCALL(*obj_ptr, teardown); free(obj_ptr); } while(0)
// 虚函数表中没有teardown, 需要显式调用特定类型的析构函数
#define RDELETE(type, obj_ptr) \
        do { concat(type, _teardown)(obj_ptr); free(obj_ptr); } while(0)

#endif //CODE_OBJECT_H


//
// Created by hby on 22-11-24.
//

#ifndef CODE_IR_PARSE_H
#define CODE_IR_PARSE_H

#include <stdbool.h>
#include <stdio.h>
#include <config.h>
#include <IR.h>


extern void IR_yyrestart ( FILE *input_file );
extern int IR_yylex();
extern int IR_yyparse();
extern int IR_yyerror(const char *msg);

extern void IR_parse(const char *input_IR_path);

extern IR_var get_IR_var(const char *id);
extern IR_label get_IR_label(const char *id);

#endif //CODE_IR_PARSE_H


static void args_stack_push(IR_val arg);
static void args_stack_pop(unsigned *argc_ptr, IR_val **argv_ptr);

static IR_function *now_function = NULL;

%}

%union{
    int INT;
    char *id;
    IR_OP_TYPE IR_op_type;
    IR_RELOP_TYPE IR_relop_type;
    IR_program *IR_program_ptr_node;
    IR_function *IR_function_ptr_node;
    IR_stmt *IR_stmt_ptr_node;
    IR_val IR_val_node;
    IR_label IR_label_node;
    IR_var IR_var_node;
}


/* declare tokens */

%token EOL
%token COLON
%token FUNCTION
%token LABEL
%token SHARP
%token<INT> INT
%token ASSIGN
%token<IR_op_type> STAR
%token<IR_op_type> OP
%token IF
%token<IR_relop_type> RELOP
%token ADDR_OF
%token GOTO
%token RETURN
%token DEC
%token ARG
%token CALL
%token PARAM
%token READ
%token WRITE
%token<id> ID

%type<IR_program_ptr_node> IR_globol
%type<IR_program_ptr_node> IR_program
%type<IR_function_ptr_node> IR_function
%type<IR_stmt_ptr_node> IR_stmt
%type<IR_val_node> IR_val
%type<IR_val_node> val_deref
%type<IR_val_node> IR_val_rs
%type<IR_label_node> IR_label
%type<IR_var_node> IR_var

%%

IR_globol   : MUL_EOL IR_program                        { ir_program_global = $2; }
            ;       
IR_program  : IR_program IR_function                    {
                                                            $$ = $1;
                                                            IR_function_closure($2);
                                                            VCALL($$->functions, push_back, $2);
                                                        }
            |                                           { $$ = NEW(IR_program); }
            ;
IR_function : FUNCTION ID COLON EOL                     { $$ = NEW(IR_function, $2); now_function = $$; free($2); }
            | IR_function PARAM IR_var EOL              { $$ = $1; VCALL($$->params, push_back, $3); }
            | IR_function DEC IR_var INT EOL            { $$ = $1; IR_function_insert_dec($$, $3, (unsigned)$4); }
            | IR_function ARG IR_val_rs EOL             { $$ = $1; args_stack_push($3); }
            | IR_function LABEL IR_label COLON EOL      { $$ = $1; IR_function_push_label($$, $3); }
            | IR_function IR_stmt EOL                   { $$ = $1; IR_function_push_stmt($$, $2); }
            ;
IR_stmt     : IR_var ASSIGN IR_val                      { $$ = (IR_stmt*)NEW(IR_assign_stmt, $1, $3); }
            | IR_var ASSIGN val_deref                   { $$ = (IR_stmt*)NEW(IR_load_stmt, $1, $3); }
            | val_deref ASSIGN IR_val_rs                { $$ = (IR_stmt*)NEW(IR_store_stmt, $1, $3); }
            | IR_var ASSIGN IR_val_rs OP IR_val_rs      { $$ = (IR_stmt*)NEW(IR_op_stmt, $4, $1, $3, $5); }
            | IR_var ASSIGN IR_val_rs STAR IR_val_rs    { $$ = (IR_stmt*)NEW(IR_op_stmt, $4, $1, $3, $5); }
            | GOTO IR_label                             { $$ = (IR_stmt*)NEW(IR_goto_stmt, $2); }
            | IF IR_val_rs RELOP IR_val_rs GOTO IR_label
                                                        { $$ = (IR_stmt*)NEW(IR_if_stmt, $3, $2, $4, $6, IR_LABEL_NONE); }
            | RETURN IR_val                             { $$ = (IR_stmt*)NEW(IR_return_stmt, $2); }
            | IR_var ASSIGN CALL ID                     {
                                                             unsigned argc;
                                                             IR_val *argv;
                                                             args_stack_pop(&argc, &argv);
                                                             $$ = (IR_stmt*)NEW(IR_call_stmt, $1, $4, argc, argv);
                                                             free($4);
                                                        }
            | READ IR_var                               { $$ = (IR_stmt*)NEW(IR_read_stmt, $2); }
            | WRITE IR_val_rs                           { $$ = (IR_stmt*)NEW(IR_write_stmt, $2); }
            ;
IR_val      : IR_var                                    { $$ = (IR_val){.is_const = false, .var = $1}; }
            | SHARP INT                                 { $$ = (IR_val){.is_const = true, .const_val = $2}; }
            | ADDR_OF IR_var                            { $$ = (IR_val){.is_const = false,
                                                                        .var = VCALL(now_function->map_dec, get, $2).dec_addr}; }
            ;
val_deref   : STAR IR_val                               { $$ = $2; }
IR_val_rs   : IR_val                                    { $$ = $1; }
            | val_deref                                 {
                                                            IR_var content = ir_var_generator();
                                                            IR_function_push_stmt(now_function,
                                                                                  (IR_stmt*)NEW(IR_load_stmt, content, $1));
                                                            $$ = (IR_val){.is_const = false, .var = content};
                                                        }
IR_var      : ID                                        { $$ = get_IR_var($1); free($1); }
            ;
IR_label    : ID                                        { $$ = get_IR_label($1); free($1); }
            ;
MUL_EOL     : MUL_EOL EOL
            |
            ;

%%

int IR_yyerror(const char *msg) {
    fprintf(YYERROR_OUTPUT, "IR syntax error: %s\n", msg);
    return 0;
}

//// ================================== args stack ==================================

unsigned args_stack_top = 0;
static IR_val args_stack[16];

static void args_stack_push(IR_val arg) {
    args_stack[args_stack_top ++] = arg;
}

static void args_stack_pop(unsigned *argc_ptr, IR_val **argv_ptr) {
    unsigned argc = args_stack_top;
    IR_val *argv = (IR_val*)malloc(sizeof(IR_val[argc]));
    for(int i = 0; i < argc; i ++)
        argv[i] = args_stack[i];
    *argc_ptr = argc;
    *argv_ptr = argv;
    args_stack_top = 0;
}

