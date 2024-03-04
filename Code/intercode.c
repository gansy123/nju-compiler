#include <stdio.h>
#include <string.h>
#include <stdbool.h>
typedef struct node *Node;
typedef struct Type_ *Type;
typedef struct FieldList_ *FieldList;
typedef struct Hashcode_ *Hashcode;
typedef struct Structure_ *Structure;
typedef struct Function_ *Function;
typedef struct _Operand *Operand;
typedef struct _InterCode *InterCode;
typedef struct _CodeList *CodeList;
typedef struct _ArgList *ArgList;
typedef struct _Variable *Variable;
extern Hashcode hashtab[1024];
extern struct node *root;
struct node
{
    int line;
    char name[32];
    int child_num;
    bool isterminal;
    int isint;
    union
    {
        unsigned int val_int;
        float val_float;
        char id[32];
    } val;
    struct node *left;
    struct node *right;
};

struct Type_
{
    enum
    {
        BASIC,
        CONST,
        ARRAY,
        STRUCTURE,
        FUNCTION
    } kind;
    union
    {
        enum
        {
            BASIC_INT,
            BASIC_FLOAT
        } basic;
        struct
        {
            Type elem;
            int size;
        } array;
        Structure structure;
        Function function;
    } u;
    enum
    {
        LEFT,
        RIGHT,
        BOTH
    } assign;
    bool def;
};

struct FieldList_
{
    char *name;
    Type type;
    FieldList tail;
    int id;
};

struct Structure_
{
    char *name;
    FieldList domain;
};

struct Function_
{
    char *name;
    int line;
    Type ret;
    FieldList param;
    FieldList next;
    bool def;
};

struct Hashcode_
{
    FieldList data;
    Hashcode next;
};

struct _Operand
{
    enum
    {
        Em_VARIABLE, // 变量（var）
        Em_CONSTANT, // 常量（#1）
        Em_ADDRESS,  // 地址（&var）
        Em_LABEL,    // 标签(LABEL label1:)
        Em_ARR,      // 数组（arr[2]）
        Em_STRUCT,   // 结构体（struct Point p）
        Em_TEMP,     // 临时变量（t1）
        Em_FUNC
    } kind;
    union
    {
        int varno;   // 变量定义的序号
        int labelno; // 标签序号
        int val;     // 操作数的值
        int tempno;  // 临时变量序号（唯一性）
        int addrno;
        char *funcname;
    } u;
    Type type;
    int size;
};

struct _InterCode
{
    enum
    {
        IC_ASSIGN,
        IC_LABEL,
        IC_PLUS,
        IC_CALL,
        IC_PARAM,
        IC_DEC,
        IC_ADDR,
        IC_LOAD,
        IC_STORE,
        IC_ADD,
        IC_SUB,
        IC_MUL,
        IC_DIV,
        IC_FUNC,
        IC_GOTO,
        IC_ARG,
        IC_READ,
        IC_WRITE,
        IC_IF,
        IC_RETURN,
    } kind;
    union
    {
        Operand op;
        struct
        {
            Operand op;
            int size;
        } dec;
        struct
        {
            Operand right, left;
        } assign;
        struct
        {
            Operand result, op1, op2;
        } binop;
        struct
        {
            Operand op1, op2, op3;
            char relop[5];
        } if_goto;
    } u;
};

struct _CodeList
{
    InterCode code; // 中间代码列表实现方式
    CodeList prev, next;
};
struct _ArgList
{ // 参数列表实现方式
    Operand args;
    ArgList next;
};
struct _Variable
{ // 变量的实现方式
    char *name;
    Operand op;
    Variable next;
};
int label_num, temp_num, var_num; // 新变量/新标签/新临时变量编号
CodeList code_head, code_tail;    // 双链表的首部和尾部
Variable var_head, var_tail;      // 变量表的首部和尾部

struct node *get_child(struct node *n, int num);
FieldList look_up(char *name);
void add_IR(InterCode ir);
void translate_Program(struct node *n);
void translate_ExtDefList(Node n);
void translate_ExtDef(Node n);
Operand translate_VarDec(Node n);
void translate_FunDec(Node n);
void translate_CompSt(Node n);
void translate_Stmt(Node n);
void translate_StmtList(Node n);
void translate_DefList(Node n);
void translate_Def(Node n);
void translate_DecList(Node n);
void translate_Dec(Node n);
void translate_Exp(Node n, Operand op1, Operand op2);
void translate_Args(Node n, bool write);
void print_IR(FILE *out);
void print_op(Operand op, FILE *out);

void debug2(Node n)
{
    // printf("%s\n", n->name);
    ;
}

int get_size(Type type)
{
    if (type == NULL)
        return -1;
    else if (type->kind == BASIC)
    {
        return 4;
    }
    else if (type->kind == ARRAY)
    {
        if (type->u.array.elem != NULL && type->u.array.elem->kind == ARRAY)
        {
            printf("Cannot translate: Code contains variables of multi-dimensional array type or parameters of array type.\n");
            return -1;
        }
        // printf("%d\n", type->u.array.size);
        return type->u.array.size * get_size(type->u.array.elem);
    }
    else if (type->kind == STRUCTURE)
    {
        int res = 0;
        FieldList member = type->u.structure->domain->type->u.structure->domain;
        while (member != NULL)
        {
            res += get_size(member->type);
            member = member->tail;
        }
        return res;
    }
    else
    {
        return -1;
    }
}

Operand generate_Operand(int kind, int val, char *name)
{
    Operand new = (Operand)malloc(sizeof(struct _Operand));
    new->kind = kind;
    FieldList var;
    switch (new->kind)
    {
    case Em_ADDRESS:
        new->u.addrno = val;
        break;
    case Em_ARR:
    case Em_VARIABLE:
    case Em_STRUCT:
        var = look_up(name);
        if (var->id == 0)
            var->id = var_num++;
        new->u.varno = var->id;
        break;
    case Em_FUNC:
        new->u.funcname = name;
        new->size = look_up(name)->type->u.function->line;
        break;
    case Em_CONSTANT:
        new->u.val = val;
        break;
    case Em_LABEL:
        new->u.labelno = val;
        break;
    case Em_TEMP:
        new->u.tempno = val;
        break;
    default:
        break;
    }
    return new;
}

InterCode generate_IR(int kind, Operand op1, Operand op2, Operand op3, int size, char *relop)
{
    InterCode ir = (InterCode)malloc(sizeof(struct _InterCode));
    ir->kind = kind;
    switch (ir->kind)
    {
    case IC_LABEL:
    case IC_FUNC:
    case IC_GOTO:
    case IC_RETURN:
    case IC_PARAM:
    case IC_ARG:
    case IC_WRITE:
    case IC_READ:
        ir->u.op = op1;
        break;
    case IC_ASSIGN:
    case IC_ADDR:
    case IC_CALL:
    case IC_LOAD:
    case IC_STORE:
        ir->u.assign.left = op1;
        ir->u.assign.right = op2;
        break;
    case IC_ADD:
    case IC_SUB:
    case IC_MUL:
    case IC_DIV:
        ir->u.binop.result = op1;
        ir->u.binop.op1 = op2;
        ir->u.binop.op2 = op3;
        break;
    case IC_DEC:
        ir->u.dec.op = op1;
        ir->u.dec.size = size;
        break;
    case IC_IF:
        ir->u.if_goto.op1 = op1;
        ir->u.if_goto.op2 = op2;
        ir->u.if_goto.op3 = op3;
        strcpy(ir->u.if_goto.relop, relop);
        break;
    default:
        break;
    }
    add_IR(ir);
    return ir;
}

void add_IR(InterCode ir)
{
    CodeList new = (CodeList)malloc(sizeof(struct _CodeList));
    new->code = ir;
    new->next = code_head;
    new->prev = code_tail;
    code_tail->next = new;
    code_head->prev = new;
    code_tail = new;
}

Operand load_value(Operand op)
{
    if (op->kind != Em_ADDRESS && op->kind != Em_STRUCT && op->kind != Em_ARR)
        return op;
    Operand ret = generate_Operand(Em_TEMP, temp_num++, NULL);
    generate_IR(IC_LOAD, ret, op, NULL, -1, NULL);
    return ret;
}

void translate_Program(struct node *n)
{
    // Program -> ExtDefList
    if (n == NULL)
        return;
    debug2(n);
    label_num = 1;
    temp_num = 1;
    var_num = 1;
    code_head = (CodeList)malloc(sizeof(struct _CodeList));
    code_head->code = NULL;
    code_tail = code_head;
    code_head->next = code_tail;
    code_head->prev = code_tail;
    code_tail->next = code_head;
    code_tail->prev = code_head;
    var_head = NULL;
    var_tail = NULL;
    translate_ExtDefList(get_child(n, 0));
}

void translate_ExtDefList(Node n)
{
    // ExtDefList -> ExtDef ExtDefList
    if (n == NULL)
        return;
    debug2(n);
    translate_ExtDef(get_child(n, 0));
    translate_ExtDefList(get_child(n, 1));
}

void translate_ExtDef(Node n)
{
    // ExtDef -> Specifier ExtDecList SEMI
    // ExtDef -> Specifier FunDec CompSt
    // ExtDef -> Specifier SEMI
    if (n == NULL)
        return;
    debug2(n);
    if (n->child_num == 2)
    {
    }
    else if (n->child_num == 3)
    {
        if (strcmp(get_child(n, 1)->name, "FunDec") == 0)
        {
            translate_FunDec(get_child(n, 1));
            translate_CompSt(get_child(n, 2));
        }
    }
}

Operand translate_VarDec(Node n)
{
    // VarDec -> ID
    // VarDec -> VarDec LB INT RB
    if (n == NULL)
        return NULL;
    debug2(n);
    if (n->child_num == 1)
    {
        char *child_name = get_child(n, 0)->val.id;
        FieldList var = look_up(child_name);
        if (var->type->kind == BASIC)
        {
            return generate_Operand(Em_VARIABLE, -1, child_name);
        }
        else if (var->type->kind == ARRAY)
        {
            Operand res = generate_Operand(Em_ARR, -1, child_name);
            res->type = var->type->u.array.elem;
            res->size = var->type->u.array.size;
            Operand tmp = generate_Operand(Em_TEMP, temp_num++, NULL);
            generate_IR(IC_DEC, tmp, NULL, NULL, get_size(var->type), NULL);
            generate_IR(IC_ADDR, res, tmp, NULL, -1, NULL);
            return res;
        }
        else if (var->type->kind == STRUCTURE)
        {
            Operand res = generate_Operand(Em_STRUCT, -1, child_name);
            res->type = var->type;
            Operand tmp = generate_Operand(Em_TEMP, temp_num++, NULL);
            generate_IR(IC_DEC, tmp, NULL, NULL, get_size(var->type), NULL);
            generate_IR(IC_ADDR, res, tmp, NULL, -1, NULL);
            return res;
        }
    }
    else if (n->child_num == 4)
    {
        return translate_VarDec(get_child(n, 0));
    }
}

void translate_FunDec(Node n)
{
    // FunDec -> ID LP VarList RP
    // FunDec -> ID LP RP
    if (n == NULL)
        return;
    debug2(n);
    if (n->child_num == 3)
    {
        Operand op = generate_Operand(Em_FUNC, -1, get_child(n, 0)->val.id);
        generate_IR(IC_FUNC, op, NULL, NULL, -1, NULL);
    }
    else if (n->child_num == 4)
    {
        Operand func = generate_Operand(Em_FUNC, -1, get_child(n, 0)->val.id);
        generate_IR(IC_FUNC, func, NULL, NULL, -1, NULL);
        Operand op;
        FieldList field = look_up(get_child(n, 0)->val.id);
        FieldList args = field->type->u.function->param;
        func->size = 0;
        while (args != NULL)
        {
            func->size++;
            switch (args->type->kind)
            {
            case BASIC:
                op = generate_Operand(Em_VARIABLE, -1, args->name);
                break;
            case ARRAY:
                op = generate_Operand(Em_ARR, -1, args->name);
                break;
            case STRUCTURE:
                op = generate_Operand(Em_STRUCT, -1, args->name);
                break;
            default:
                break;
            }
            // printf("%s : %d\n", args->name, args->id);
            generate_IR(IC_PARAM, op, NULL, NULL, -1, NULL);
            args = args->tail;
        }
        // printf("%d %s\n", func->size, func->u.funcname);
        field->type->u.function->line = func->size;
    }
}

void translate_CompSt(Node n)
{
    // CompSt -> LC DefList StmtList RC
    if (n == NULL)
        return;
    debug2(n);
    if (strcmp(get_child(n, 1)->name, "DefList") == 0)
        translate_DefList(get_child(n, 1));
    else
        translate_StmtList(get_child(n, 1));
    if (strcmp(get_child(n, 2)->name, "StmtList") == 0)
        translate_StmtList(get_child(n, 2));
}

void translate_StmtList(Node n)
{
    // Stmtlist -> Stmt Stmtlist
    if (n == NULL)
        return;
    debug2(n);
    translate_Stmt(get_child(n, 0));
    translate_StmtList(get_child(n, 1));
}

void translate_Stmt(Node n)
{
    // Stmt -> Exp SEMI
    // Stmt -> CompSt
    // Stmt -> RETURN Exp SEMI
    // Stmt -> IF LP Exp RP Stmt
    // Stmt -> IF LP Exp RP Stmt ELSE Stmt
    // Stmt -> WHILE LP Exp RP Stmt
    if (n == NULL)
        return;
    debug2(n);
    if (n->child_num == 1)
    {
        translate_CompSt(get_child(n, 0));
    }
    else if (n->child_num == 2)
    {
        translate_Exp(get_child(n, 0), generate_Operand(Em_TEMP, temp_num++, NULL), NULL);
    }
    else if (n->child_num == 3)
    {
        Operand op = generate_Operand(Em_TEMP, temp_num++, NULL);
        translate_Exp(get_child(n, 1), op, NULL);
        op = load_value(op);
        generate_IR(IC_RETURN, op, NULL, NULL, -1, NULL);
    }
    else if (n->child_num == 5)
    {
        if (strcmp(get_child(n, 0)->name, "IF") == 0)
        {
            Operand l1 = generate_Operand(Em_LABEL, label_num++, NULL);
            Operand l2 = generate_Operand(Em_LABEL, label_num++, NULL);
            translate_Exp(get_child(n, 2), l1, l2);
            generate_IR(IC_LABEL, l1, NULL, NULL, -1, NULL);
            translate_Stmt(get_child(n, 4));
            generate_IR(IC_LABEL, l2, NULL, NULL, -1, NULL);
        }
        else
        {
            Operand l1 = generate_Operand(Em_LABEL, label_num++, NULL);
            Operand l2 = generate_Operand(Em_LABEL, label_num++, NULL);
            Operand l3 = generate_Operand(Em_LABEL, label_num++, NULL);
            generate_IR(IC_LABEL, l1, NULL, NULL, -1, NULL);
            translate_Exp(get_child(n, 2), l2, l3);
            generate_IR(IC_LABEL, l2, NULL, NULL, -1, NULL);
            translate_Stmt(get_child(n, 4));
            generate_IR(IC_GOTO, l1, NULL, NULL, -1, NULL);
            generate_IR(IC_LABEL, l3, NULL, NULL, -1, NULL);
        }
    }
    else if (n->child_num == 7)
    {
        Operand l1 = generate_Operand(Em_LABEL, label_num++, NULL);
        Operand l2 = generate_Operand(Em_LABEL, label_num++, NULL);
        Operand l3 = generate_Operand(Em_LABEL, label_num++, NULL);
        translate_Exp(get_child(n, 2), l1, l2);
        generate_IR(IC_LABEL, l1, NULL, NULL, -1, NULL);
        translate_Stmt(get_child(n, 4));
        generate_IR(IC_GOTO, l3, NULL, NULL, -1, NULL);
        generate_IR(IC_LABEL, l2, NULL, NULL, -1, NULL);
        translate_Stmt(get_child(n, 6));
        generate_IR(IC_LABEL, l3, NULL, NULL, -1, NULL);
    }
}

void translate_DefList(Node n)
{
    // DefList -> Def DefList
    if (n == NULL)
        return;
    debug2(n);
    translate_Def(get_child(n, 0));
    translate_DefList(get_child(n, 1));
}

void translate_Def(Node n)
{
    // Def -> Specifier DecList SEMI
    if (n == NULL)
        return;
    debug2(n);
    translate_DecList(get_child(n, 1));
}

void translate_DecList(Node n)
{
    // DecList -> Dec
    // DecList -> Dec COMMA DecList
    if (n == NULL)
        return;
    debug2(n);
    if (n->child_num == 1)
    {
        translate_Dec(get_child(n, 0));
    }
    else
    {
        translate_Dec(get_child(n, 0));
        translate_DecList(get_child(n, 2));
    }
}

void translate_Dec(Node n)
{
    // Dec -> VarDec
    // Dec -> VarDec ASSIGNOP Exp
    if (n == NULL)
        return;
    debug2(n);
    if (n->child_num == 1)
    {
        translate_VarDec(get_child(n, 0));
    }
    else
    {
        Operand var = translate_VarDec(get_child(n, 0));
        Operand op = generate_Operand(Em_TEMP, temp_num++, NULL);
        translate_Exp(get_child(n, 2), op, NULL);
        if (var->kind == Em_VARIABLE)
        {
            op = load_value(op);
            generate_IR(IC_ASSIGN, var, op, NULL, -1, NULL);
        }
        else if (var->kind == ARRAY)
        {
            int size = var->size < op->size ? var->size : op->size;
            Operand temp = generate_Operand(Em_TEMP, temp_num++, NULL);
            Operand left = generate_Operand(Em_ADDRESS, temp_num++, NULL);
            Operand right = generate_Operand(Em_ADDRESS, temp_num++, NULL);
            generate_IR(IC_LOAD, temp, op, NULL, -1, NULL);
            generate_IR(IC_STORE, var, temp, NULL, -1, NULL);
            for (int i = 4; i < size; i += 4)
            {
                Operand offset = generate_Operand(Em_CONSTANT, i, NULL);
                generate_IR(IC_ADD, left, var, offset, -1, NULL);
                generate_IR(IC_ADD, right, var, offset, -1, NULL);
                generate_IR(IC_LOAD, temp, right, NULL, -1, NULL);
                generate_IR(IC_STORE, left, temp, NULL, -1, NULL);
            }
        }
    }
}

void translate_Exp(Node n, Operand op1, Operand op2)
{
    // Exp -> Exp ASSIGNOP Exp
    // Exp -> Exp AND Exp
    // Exp -> Exp OR Exp
    // Exp -> Exp RELOP Exp
    // Exp -> Exp PLUS Exp
    // Exp -> Exp MINUS Exp
    // Exp -> Exp STAR Exp
    // Exp -> Exp DIV Exp
    // Exp -> LP Exp RP
    // Exp -> MINUS Exp
    // Exp -> NOT Exp
    // Exp -> ID LP Args RP
    // Exp -> ID LP RP
    // Exp -> Exp LB Exp RB
    // Exp -> Exp DOT ID
    // Exp -> ID
    // Exp -> INT
    // Exp -> FLOAT
    if (n == NULL)
        return;
    debug2(n);
    if (op2 != NULL)
    {
        // Exp -> NOT Exp
        // Exp -> Exp AND Exp
        // Exp -> Exp OR Exp
        // Exp -> Exp RELOP Exp
        if (n->child_num == 2)
        {
            // printf("  %s\n", get_child(n, 1)->name);
            translate_Exp(get_child(n, 1), op2, op1);
        }
        else if (n->child_num == 3)
        {
            if (strcmp(get_child(n, 0)->name, "LP") == 0)
            {
                translate_Exp(get_child(n, 1), op1, op2);
            }
            if (strcmp(get_child(n, 1)->name, "AND") == 0)
            {
                Operand l1 = generate_Operand(Em_LABEL, label_num++, NULL);
                translate_Exp(get_child(n, 0), l1, op2);
                generate_IR(IC_LABEL, l1, NULL, NULL, -1, NULL);
                translate_Exp(get_child(n, 2), op1, op2);
            }
            else if (strcmp(get_child(n, 1)->name, "OR") == 0)
            {
                // printf("%s\n", get_child(get_child(n, 0), 0)->val.id);
                // printf(" here\n");
                Operand l1 = generate_Operand(Em_LABEL, label_num++, NULL);
                translate_Exp(get_child(n, 0), op1, l1);
                generate_IR(IC_LABEL, l1, NULL, NULL, -1, NULL);
                translate_Exp(get_child(n, 2), op1, op2);
            }
            else if (strcmp(get_child(n, 1)->name, "RELOP") == 0)
            {
                Operand exp1 = generate_Operand(Em_TEMP, temp_num++, NULL);
                Operand exp2 = generate_Operand(Em_TEMP, temp_num++, NULL);
                translate_Exp(get_child(n, 0), exp1, NULL);
                translate_Exp(get_child(n, 2), exp2, NULL);
                exp1 = load_value(exp1);
                exp2 = load_value(exp2);
                char *relop = get_child(n, 1)->val.id;
                generate_IR(IC_IF, exp1, exp2, op1, -1, relop);
                generate_IR(IC_GOTO, op2, NULL, NULL, -1, NULL);
            }
        }
        else
        {
            Operand exp1 = generate_Operand(Em_TEMP, temp_num++, NULL);
            translate_Exp(n, exp1, NULL);
            exp1 = load_value(exp1);
            generate_IR(IC_IF, exp1, generate_Operand(Em_CONSTANT, 0, NULL), op1, -1, "!=");
            generate_IR(IC_GOTO, op2, NULL, NULL, -1, NULL);
        }
    }
    else
    {
        // 1Exp -> Exp ASSIGNOP Exp
        // 1Exp -> Exp AND Exp
        // 1Exp -> Exp OR Exp
        // 1Exp -> Exp RELOP Exp
        // 1Exp -> Exp PLUS Exp
        // 1Exp -> Exp MINUS Exp
        // 1Exp -> Exp STAR Exp
        // 1Exp -> Exp DIV Exp
        // 1Exp -> LP Exp RP
        // 1Exp -> MINUS Exp
        // 1Exp -> NOT Exp
        // 1Exp -> ID LP Args RP
        // 1Exp -> ID LP RP
        // 1Exp -> Exp LB Exp RB
        // 1Exp -> Exp DOT ID
        // 1Exp -> ID
        // 1Exp -> INT
        // 1Exp -> FLOAT
        if ((n->child_num == 2 && strcmp(get_child(n, 0)->name, "NOT") == 0) ||
            ((n->child_num == 3) && (strcmp(get_child(n, 1)->name, "AND") == 0 ||
                                     strcmp(get_child(n, 1)->name, "OR") == 0 || strcmp(get_child(n, 1)->name, "RELOP") == 0)))
        {
            Operand l1 = generate_Operand(Em_LABEL, label_num++, NULL);
            Operand l2 = generate_Operand(Em_LABEL, label_num++, NULL);
            generate_IR(IC_ASSIGN, op1, generate_Operand(Em_CONSTANT, 0, NULL), NULL, -1, NULL);
            translate_Exp(n, l1, l2);
            generate_IR(IC_LABEL, l1, NULL, NULL, -1, NULL);
            generate_IR(IC_ASSIGN, op1, generate_Operand(Em_CONSTANT, 1, NULL), NULL, -1, NULL);
            generate_IR(IC_LABEL, l2, NULL, NULL, -1, NULL);
        }
        else if (n->child_num == 1)
        {
            if (strcmp(get_child(n, 0)->name, "INT") == 0)
            {
                op1->kind = Em_CONSTANT;
                op1->u.val = atoi(get_child(n, 0)->val.id);
            }
            else if (strcmp(get_child(n, 0)->name, "ID") == 0)
            {
                FieldList var = look_up(get_child(n, 0)->val.id);
                if (var->type->kind == BASIC)
                {
                    if (var->id == 0)
                        var->id = var_num++;
                    op1->kind = Em_VARIABLE;
                    op1->u.varno = var->id;
                }
                else if (var->type->kind == ARRAY)
                {
                    if (var->id == 0)
                        var->id = var_num++;
                    op1->kind = Em_ARR;
                    op1->u.varno = var->id;
                    op1->type = var->type;
                }
                else if (var->type->kind == STRUCTURE)
                {
                    if (var->id == 0)
                        var->id = var_num++;
                    op1->kind = Em_STRUCT;
                    op1->u.varno = var->id;
                    op1->type = var->type;
                    op1->size = get_size(op1->type);
                    // printf(" %d\n", op1->type->kind);
                }
            }
            else
            {
                ;
            }
        }
        else if (n->child_num == 2)
        {
            Operand t1 = generate_Operand(Em_TEMP, -1, NULL);
            translate_Exp(get_child(n, 1), t1, NULL);
            t1 = load_value(t1);
            if (t1->kind == Em_CONSTANT)
            {
                op1->kind = Em_CONSTANT;
                op1->u.val = 0 - t1->u.val;
            }
            else
            {
                generate_IR(IC_SUB, op1, generate_Operand(Em_CONSTANT, 0, NULL), t1, -1, NULL);
            }
        }
        else if (n->child_num == 3)
        {
            if (strcmp(get_child(n, 0)->name, "ID") == 0)
            {
                // Exp -> ID LP RP
                FieldList func = look_up(get_child(n, 0)->val.id);
                if (strcmp(get_child(n, 0)->val.id, "read") == 0)
                {
                    generate_IR(IC_READ, op1, NULL, NULL, -1, NULL);
                }
                else
                {
                    // printf("here\n");
                    Operand op = generate_Operand(Em_FUNC, -1, func->name);
                    generate_IR(IC_CALL, op1, op, NULL, -1, NULL);
                }
            }
            else if (strcmp(get_child(n, 1)->name, "DOT") == 0)
            {
                // Exp -> Exp DOT ID
                Operand t1 = generate_Operand(Em_ADDRESS, temp_num++, NULL);
                translate_Exp(get_child(n, 0), t1, NULL);
                // printf(" %d\n", get_child(n, 0)->child_num);
                char *name = get_child(n, 2)->val.id;
                // printf(" %s\n", name);
                // printf(" %s\n", t1->type->u.structure->domain->name);
                FieldList field = t1->type->u.structure->domain->type->u.structure->domain;
                int offset = 0;
                while (field != NULL)
                {

                    if (strcmp(name, field->name) == 0)
                    {
                        op1->kind = Em_ADDRESS;
                        op1->type = field->type;
                        if (field->type->kind == ARRAY)
                        {
                            op1->kind = Em_ARR;
                            op1->size = field->type->u.array.size;
                        }
                        if (field->type->kind == STRUCTURE)
                        {
                            op1->kind = Em_STRUCT;
                        }
                        field = field->tail;
                        while (field != NULL)
                        {
                            offset += get_size(field->type);
                            // printf("%s-%d-%d ", field->name, offset, field->type->kind);
                            field = field->tail;
                        }
                        // printf("\n");
                        break;
                    }
                    else
                        field = field->tail;
                }
                generate_IR(IC_ADD, op1, t1, generate_Operand(Em_CONSTANT, offset, NULL), -1, NULL);
            }
            else if (strcmp(get_child(n, 0)->name, "LP") == 0)
            {
                // Exp -> LP Exp RP
                translate_Exp(get_child(n, 1), op1, op2);
            }
            else if (strcmp(get_child(n, 1)->name, "ASSIGNOP") == 0)
            {
                // Exp -> Exp ASSIGNOP Exp
                Operand var = generate_Operand(Em_TEMP, temp_num++, NULL);
                Operand op = generate_Operand(Em_TEMP, temp_num++, NULL);
                translate_Exp(get_child(n, 0), var, NULL);
                translate_Exp(get_child(n, 2), op, NULL);
                if (var->kind == Em_VARIABLE)
                {
                    op = load_value(op);
                    generate_IR(IC_ASSIGN, var, op, NULL, -1, NULL);
                }
                else if (var->kind == Em_ARR)
                {
                    int size = var->type->u.array.size < op->type->u.array.size ? var->type->u.array.size : op->type->u.array.size;
                    Operand temp = generate_Operand(Em_TEMP, temp_num++, NULL);
                    Operand left = generate_Operand(Em_ADDRESS, temp_num++, NULL);
                    Operand right = generate_Operand(Em_ADDRESS, temp_num++, NULL);
                    generate_IR(IC_LOAD, temp, op, NULL, -1, NULL);
                    generate_IR(IC_STORE, var, temp, NULL, -1, NULL);
                    for (int i = 4; i < size * 4; i += 4)
                    {
                        // printf(" %d\n", i);
                        Operand offset = generate_Operand(Em_CONSTANT, i, NULL);
                        generate_IR(IC_ADD, left, var, offset, -1, NULL);
                        generate_IR(IC_ADD, right, op, offset, -1, NULL);
                        generate_IR(IC_LOAD, temp, right, NULL, -1, NULL);
                        generate_IR(IC_STORE, left, temp, NULL, -1, NULL);
                    }
                }
                else if (var->kind == Em_ADDRESS)
                {
                    op = load_value(op);
                    generate_IR(IC_STORE, var, op, NULL, -1, NULL);
                }
                op1->kind = op->kind;
                op1->u = op->u;
                op1->type = op->type;
                op1->size = op->size;
            }
            else
            {
                // Exp -> Exp PLUS Exp
                // Exp -> Exp MINUS Exp
                // Exp -> Exp STAR Exp
                // Exp -> Exp DIV Exp
                if (strcmp(get_child(n, 1)->name, "STAR") == 0)
                {
                    Node div = get_child(n, 2);
                    if (div->child_num == 3 && strcmp(get_child(div, 1)->name, "DIV") == 0)
                    {
                        Operand t1 = generate_Operand(Em_TEMP, temp_num++, NULL);
                        Operand t2 = generate_Operand(Em_TEMP, temp_num++, NULL);
                        Operand t3 = generate_Operand(Em_TEMP, temp_num++, NULL);
                        translate_Exp(get_child(n, 0), t1, NULL);
                        translate_Exp(get_child(div, 0), t2, NULL);
                        translate_Exp(get_child(div, 2), t3, NULL);
                        t1 = load_value(t1);
                        t2 = load_value(t2);
                        t3 = load_value(t3);
                        generate_IR(IC_MUL, t1, t1, t2, -1, NULL);
                        generate_IR(IC_DIV, op1, t1, t3, -1, NULL);
                        return;
                    }
                }
                Operand t1 = generate_Operand(Em_TEMP, temp_num++, NULL);
                Operand t2 = generate_Operand(Em_TEMP, temp_num++, NULL);
                translate_Exp(get_child(n, 0), t1, NULL);
                translate_Exp(get_child(n, 2), t2, NULL);
                t1 = load_value(t1);
                t2 = load_value(t2);
                char *s = get_child(n, 1)->name;
                if (strcmp(s, "PLUS") == 0)
                {
                    generate_IR(IC_ADD, op1, t1, t2, -1, NULL);
                }
                else if (strcmp(s, "MINUS") == 0)
                {
                    generate_IR(IC_SUB, op1, t1, t2, -1, NULL);
                }
                else if (strcmp(s, "STAR") == 0)
                {
                    generate_IR(IC_MUL, op1, t1, t2, -1, NULL);
                }
                else if (strcmp(s, "DIV") == 0)
                {
                    generate_IR(IC_DIV, op1, t1, t2, -1, NULL);
                }
            }
        }
        else if (n->child_num == 4)
        {
            if (strcmp(get_child(n, 0)->name, "ID") == 0)
            {
                // Exp -> ID LP Args RP
                FieldList func = look_up(get_child(n, 0)->val.id);
                if (strcmp(get_child(n, 0)->val.id, "write") == 0)
                {
                    translate_Args(get_child(n, 2), true);
                    op1->kind = Em_CONSTANT;
                    op1->u.val = 0;
                }
                else
                {
                    translate_Args(get_child(n, 2), false);
                    Operand op = generate_Operand(Em_FUNC, -1, func->name);
                    generate_IR(IC_CALL, op1, op, NULL, -1, NULL);
                }
            }
            else if (strcmp(get_child(n, 0)->name, "Exp") == 0)
            {
                // Exp -> Exp LB Exp RB
                Operand t1 = generate_Operand(Em_TEMP, temp_num++, NULL);
                Operand t2 = generate_Operand(Em_TEMP, temp_num++, NULL);
                translate_Exp(get_child(n, 0), t1, NULL);
                translate_Exp(get_child(n, 2), t2, NULL);
                t2 = load_value(t2);
                Operand offset = generate_Operand(Em_TEMP, temp_num++, NULL);
                int wid = get_size(t1->type->u.array.elem);
                if (t2->kind == Em_CONSTANT)
                {
                    offset->kind = Em_CONSTANT;
                    offset->u.val = t2->u.val * wid;
                }
                else
                {
                    generate_IR(IC_MUL, offset, t2, generate_Operand(Em_CONSTANT, wid, NULL), -1, NULL);
                }
                op1->type = t1->type->u.array.elem;
                op1->size = t1->type->u.array.size;
                if (t1->type->u.array.elem->kind == STRUCTURE)
                {
                    op1->kind = Em_STRUCT;
                }
                else if (t1->type->u.array.elem->kind == BASIC)
                {
                    op1->kind = Em_ADDRESS;
                }
                // printf(" %d\n", op1->type->kind);
                // Operand t3 = generate_Operand(Em_TEMP, temp_num++, NULL);
                generate_IR(IC_ADD, op1, t1, offset, -1, NULL);
                // generate_IR(IC_ADDR, op1, t3, NULL, -1, NULL);
            }
        }
    }
}

void translate_Args(Node n, bool write)
{
    // Args -> Exp COMMA Args
    // Args -> Exp
    if (n == NULL)
        return;
    debug2(n);
    Operand arg = generate_Operand(Em_TEMP, temp_num++, NULL);
    translate_Exp(get_child(n, 0), arg, NULL);
    if (arg->kind != Em_STRUCT)
        arg = load_value(arg);
    if (n->child_num == 3)
    {
        translate_Args(get_child(n, 2), write);
    }

    // printf("%d ", arg->kind);

    if (write)
    {
        generate_IR(IC_WRITE, arg, NULL, NULL, -1, NULL);
    }
    else
    {
        generate_IR(IC_ARG, arg, NULL, NULL, -1, NULL);
    }
}

void print_IR(FILE *out)
{
    CodeList ir = code_head->next;
    while (ir != NULL && ir->code != NULL)
    {
        switch (ir->code->kind)
        {
        case IC_LABEL:
            fprintf(out, "LABEL ");
            print_op(ir->code->u.op, out);
            fprintf(out, ": ");
            break;
        case IC_FUNC:
            fprintf(out, "FUNCTION ");
            print_op(ir->code->u.op, out);
            fprintf(out, ": ");
            break;
        case IC_GOTO:
            fprintf(out, "GOTO ");
            print_op(ir->code->u.op, out);
            break;
        case IC_RETURN:
            fprintf(out, "RETURN ");
            print_op(ir->code->u.op, out);
            break;
        case IC_PARAM:
            fprintf(out, "PARAM ");
            print_op(ir->code->u.op, out);
            break;
        case IC_ARG:
            fprintf(out, "ARG ");
            print_op(ir->code->u.op, out);
            break;
        case IC_WRITE:
            fprintf(out, "WRITE ");
            print_op(ir->code->u.op, out);
            break;
        case IC_READ:
            fprintf(out, "READ ");
            print_op(ir->code->u.op, out);
            break;
        case IC_ASSIGN:
            print_op(ir->code->u.assign.left, out);
            fprintf(out, ":= ");
            print_op(ir->code->u.assign.right, out);
            break;
        case IC_ADDR:
            print_op(ir->code->u.assign.left, out);
            fprintf(out, ":= &");
            print_op(ir->code->u.assign.right, out);
            break;
        case IC_CALL:
            print_op(ir->code->u.assign.left, out);
            fprintf(out, ":= CALL ");
            print_op(ir->code->u.assign.right, out);
            break;
        case IC_LOAD:
            print_op(ir->code->u.assign.left, out);
            fprintf(out, ":= *");
            print_op(ir->code->u.assign.right, out);
            break;
        case IC_STORE:
            fprintf(out, "*");
            print_op(ir->code->u.assign.left, out);
            fprintf(out, ":= ");
            print_op(ir->code->u.assign.right, out);
            break;
        case IC_ADD:
            print_op(ir->code->u.binop.result, out);
            fprintf(out, ":= ");
            print_op(ir->code->u.binop.op1, out);
            fprintf(out, "+ ");
            print_op(ir->code->u.binop.op2, out);
            break;
        case IC_SUB:
            print_op(ir->code->u.binop.result, out);
            fprintf(out, ":= ");
            print_op(ir->code->u.binop.op1, out);
            fprintf(out, "- ");
            print_op(ir->code->u.binop.op2, out);
            break;
        case IC_MUL:
            print_op(ir->code->u.binop.result, out);
            fprintf(out, ":= ");
            print_op(ir->code->u.binop.op1, out);
            fprintf(out, "* ");
            print_op(ir->code->u.binop.op2, out);
            break;
        case IC_DIV:
            print_op(ir->code->u.binop.result, out);
            fprintf(out, ":= ");
            print_op(ir->code->u.binop.op1, out);
            fprintf(out, "/ ");
            print_op(ir->code->u.binop.op2, out);
            break;
        case IC_DEC:
            fprintf(out, "DEC ");
            print_op(ir->code->u.dec.op, out);
            fprintf(out, "%d ", ir->code->u.dec.size);
            break;
        case IC_IF:
            fprintf(out, "IF ");
            print_op(ir->code->u.if_goto.op1, out);
            fprintf(out, "%s ", ir->code->u.if_goto.relop);
            print_op(ir->code->u.if_goto.op2, out);
            fprintf(out, "GOTO ");
            print_op(ir->code->u.if_goto.op3, out);
            break;
        default:
            break;
        }
        fprintf(out, "\n");
        ir = ir->next;
    }
}

void print_op(Operand op, FILE *out)
{
    if (op == NULL)
        return;
    switch (op->kind)
    {
    case Em_ADDRESS:
        fprintf(out, "addr%d ", op->u.addrno);
        break;
    case Em_ARR:
        fprintf(out, "array%d ", op->u.varno);
        break;
    case Em_CONSTANT:
        fprintf(out, "#%d ", op->u.val);
        break;
    case Em_FUNC:
        fprintf(out, "%s ", op->u.funcname);
        break;
    case Em_LABEL:
        fprintf(out, "label%d ", op->u.labelno);
        break;
    case Em_STRUCT:
        fprintf(out, "struct%d ", op->u.varno);
        break;
    case Em_TEMP:
        fprintf(out, "t%d ", op->u.tempno);
        break;
    case Em_VARIABLE:
        fprintf(out, "var%d ", op->u.varno);
        break;
    default:
        break;
    }
}
