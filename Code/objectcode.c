#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef struct node* Node;
typedef struct Type_* Type;
typedef struct FieldList_* FieldList;
typedef struct Hashcode_* Hashcode;
typedef struct Structure_* Structure;
typedef struct Function_* Function;
typedef struct _Operand* Operand;
typedef struct _InterCode* InterCode;
typedef struct _CodeList* CodeList;
typedef struct _ArgList* ArgList;
typedef struct _Variable* Variable;
extern Hashcode hashtab[1024];
extern struct node* root;
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
    struct node* left;
    struct node* right;
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
    char* name;
    Type type;
    FieldList tail;
    int id;
};

struct Structure_
{
    char* name;
    FieldList domain;
};

struct Function_
{
    char* name;
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
        char* funcname;
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
    char* name;
    Operand op;
    Variable next;
};
extern CodeList code_head, code_tail; // 双链表的首部和尾部
typedef struct Register_* Register;
typedef struct Variable_* Vari;
typedef struct VariableList_* VariableList;
struct Register_
{
    char name[10];
    enum
    {
        FREE,
        BUSY
    } state;
    Vari var;
} regs[32];

struct Variable_
{
    Operand op;
    int offset;
    int reg_no;
};
struct VariableList_
{
    Vari var;
    VariableList next;
};

VariableList local_varlist;
int local_offset;
int arg_num;
int param_num;
int old_offset1 = 1, old_offset2 = 1, old_offset3 = 1;
void init_environment(FILE* out)
{
    // head
    fprintf(out, ".data\n");
    fprintf(out, "_prompt: .asciiz \"Enter an integer:\"\n");
    fprintf(out, "_ret: .asciiz \"\\n\"\n");
    fprintf(out, ".globl main\n");
    // read
    fprintf(out, ".text\n");
    fprintf(out, "read:\n");
    fprintf(out, "  li $v0, 4\n");
    fprintf(out, "  la $a0, _prompt\n");
    fprintf(out, "  syscall\n");
    fprintf(out, "  li $v0, 5\n");
    fprintf(out, "  syscall\n");
    fprintf(out, "  jr $ra\n");
    fprintf(out, "\n");
    // write
    fprintf(out, "write:\n");
    fprintf(out, "  li $v0, 1\n");
    fprintf(out, "  syscall\n");
    fprintf(out, "  li $v0, 4\n");
    fprintf(out, "  la $a0, _ret\n");
    fprintf(out, "  syscall\n");
    fprintf(out, "  move $v0, $0\n");
    fprintf(out, "  jr $ra\n");
    fprintf(out, "\n");
}

void generate_objectcode(FILE* out)
{
    init_environment(out);
    char* reg_names[] = { "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
                         "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra" };
    for (int i = 0; i < 32; i++)
    {
        strcpy(regs[i].name, reg_names[i]);
        regs[i].state = FREE;
        regs[i].var = NULL;
    }
    local_offset = 0;
    local_varlist = NULL;
    arg_num = 0;
    param_num = 0;
    CodeList tmp = code_head->next;
    for (; tmp != NULL && tmp->code != NULL; tmp = tmp->next)
    {
        init_symbol(tmp->code, out);
        get_objectcode(tmp->code, out);
    }
}

void init_symbol(InterCode ir, FILE* out)
{
    switch (ir->kind)
    {
    case IC_LABEL:
    case IC_FUNC:
    case IC_GOTO:
        break;
    case IC_PARAM:
    {
        Vari var = (Vari)malloc(sizeof(struct Variable_));
        var->reg_no = -1;
        var->op = ir->u.op;
        var->offset = 8 + 4 * param_num;
        insert_var(var);
        param_num++;
        break;
    }
    case IC_RETURN:
    case IC_ARG:
    case IC_READ:
    case IC_WRITE:
        insert_op(ir->u.op, out);
        break;
    case IC_DEC:
        local_offset -= (ir->u.dec.size);
        fprintf(out, "  addi $sp, $sp, %d\n", 0 - ir->u.dec.size);
        insert_op(ir->u.dec.op, out);
        break;
    case IC_ASSIGN:
    case IC_ADDR:
    case IC_LOAD:
    case IC_STORE:
        insert_op(ir->u.assign.left, out);
        insert_op(ir->u.assign.right, out);
        break;
    case IC_CALL:
        break;
    case IC_ADD:
    case IC_SUB:
    case IC_MUL:
    case IC_DIV:
        insert_op(ir->u.binop.result, out);
        insert_op(ir->u.binop.op1, out);
        insert_op(ir->u.binop.op2, out);
        break;
    case IC_IF:
        insert_op(ir->u.if_goto.op1, out);
        insert_op(ir->u.if_goto.op2, out);
        break;
    default:
        break;
    }
}

void get_objectcode(InterCode ir, FILE* out)
{
    switch (ir->kind)
    {
    case IC_LABEL:
        ir_LABEL(ir, out);
        break;
    case IC_FUNC:
        ir_FUNC(ir, out);
        break;
    case IC_GOTO:
        ir_GOTO(ir, out);
        break;
    case IC_RETURN:
        ir_RETURN(ir, out);
        break;
    case IC_PARAM:
        ir_PARAM(ir, out);
        break;
    case IC_ARG:
        ir_ARG(ir, out);
        break;
    case IC_WRITE:
        ir_WRITE(ir, out);
        break;
    case IC_READ:
        ir_READ(ir, out);
        break;
    case IC_ASSIGN:
    case IC_ADDR:
        ir_ASSIGN_ADDR(ir, out);
        break;
    case IC_CALL:
        ir_CALL(ir, out);
        break;
    case IC_LOAD:
        ir_LOAD(ir, out);
        break;
    case IC_STORE:
        ir_STORE(ir, out);
        break;
    case IC_ADD:
    case IC_SUB:
    case IC_MUL:
    case IC_DIV:
        ir_MATH(ir, out);
        break;
    case IC_DEC:
        ir_DEC(ir, out);
        break;
    case IC_IF:
        ir_IF(ir, out);
        break;
    default:
        break;
    }
}
Vari find_var(Operand op);
void insert_op(Operand op, FILE* out)
{
    if (op->kind != Em_CONSTANT && find_var(op) == NULL)
    {
        Vari var = (Vari)malloc(sizeof(struct Variable_));
        var->reg_no = -1;
        var->op = op;
        local_offset = local_offset - 4;
        fprintf(out, "  addi $sp, $sp, -4\n");
        var->offset = local_offset;
        insert_var(var);
    }
}

Vari find_var(Operand op)
{
    if (op->kind == Em_CONSTANT)
        return NULL;
    VariableList tmp = NULL;
    for (tmp = local_varlist; tmp != NULL; tmp = tmp->next)
    {
        if (tmp->var->op->kind != op->kind)
            continue;
        switch (op->kind)
        {
        case Em_ADDRESS:
        case Em_TEMP:
            if (tmp->var->op->u.tempno == op->u.tempno)
                return tmp->var;
            break;
        case Em_ARR:
        case Em_VARIABLE:
            if (tmp->var->op->u.varno == op->u.varno)
                return tmp->var;
            break;
        default:
            break;
        }
    }
    return NULL;
}

void insert_var(Vari var)
{
    VariableList varl = (VariableList)malloc(sizeof(struct VariableList_));
    varl->var = var;
    varl->next = local_varlist;
    local_varlist = varl;
}

int get_reg(Operand op, bool isleft, FILE* out)
{
    int i;
    for (i = 8; i < 16; i++)
    {
        if (regs[i].state == FREE)
            break;
    }
    regs[i].state = BUSY;
    if (op->kind == Em_CONSTANT)
    {
        fprintf(out, "  li $%s, %lld\n", regs[i].name, op->u.val);
    }
    else
    {
        Vari var = find_var(op);
        if (var == NULL)
        {
            insert_op(op, out);
            var = find_var(op);
        }
        var->reg_no = i;
        regs[i].var = var;
        switch (op->kind)
        {
        case Em_TEMP:
        case Em_VARIABLE:
        case Em_ADDRESS:
            if (!isleft)
                fprintf(out, "  lw $%s, %d($fp)\n", regs[i].name, var->offset);
            break;
        case Em_ARR:
            fprintf(out, "  addi $%s, $fp, %d\n", regs[i].name, var->offset);
            break;
        }
    }
    return i;
}

void store_reg(int reg_no, FILE* out)
{
    if (regs[reg_no].var != NULL)
    {
        fprintf(out, "  sw $%s, %d($fp)\n", regs[reg_no].name, regs[reg_no].var->offset);
    }
    clear_reg(reg_no);
}

void clear_reg(int reg_no)
{
    regs[reg_no].state = FREE;
}

void ir_LABEL(InterCode ir, FILE* out)
{
    fprintf(out, "  label%d:\n", ir->u.op->u.labelno);
    if (old_offset3 != 1 && old_offset3 != local_offset)
    {
        fprintf(out, "  addi $sp, $sp, -%d\n", old_offset3 - local_offset);
        old_offset3 = 0;
    }
    else if (old_offset2 != 1 && old_offset2 != local_offset)
    {
        fprintf(out, "  addi $sp, $sp, -%d\n", old_offset2 - local_offset);
        old_offset2 = 0;
    }
    else if (old_offset1 != 1 && old_offset1 != local_offset)
    {
        fprintf(out, "  addi $sp, $sp, -%d\n", old_offset1 - local_offset);
        old_offset1 = 0;
    }
}
void ir_FUNC(InterCode ir, FILE* out)
{
    local_offset = 0;
    arg_num = 0;
    param_num = 0;
    fprintf(out, "%s:\n", ir->u.op->u.funcname);
    fprintf(out, "  move $fp, $sp\n");
}
void ir_GOTO(InterCode ir, FILE* out)
{
    fprintf(out, "  j label%d\n", ir->u.op->u.labelno);

}
void ir_RETURN(InterCode ir, FILE* out)
{
    int reg = get_reg(ir->u.op, false, out);
    fprintf(out, "  move $v0, $%s\n", regs[reg].name);
    fprintf(out, "  jr $ra\n");
    clear_reg(reg);
}
void ir_PARAM(InterCode ir, FILE* out)
{
}
void ir_ARG(InterCode ir, FILE* out)
{
    arg_num++;
    int reg = get_reg(ir->u.op, false, out);
    fprintf(out, "  addi $sp, $sp, -4\n");
    fprintf(out, "  sw $%s, 0($sp)\n", regs[reg].name);
    clear_reg(reg);
}
void ir_WRITE(InterCode ir, FILE* out)
{
    int reg = get_reg(ir->u.op, false, out);
    fprintf(out, "  move $a0, $%s\n", regs[reg].name);
    fprintf(out, "  addi $sp, $sp, -4\n");
    fprintf(out, "  sw $ra, 0($sp)\n");
    fprintf(out, "  jal write\n");
    fprintf(out, "  lw $ra, 0($sp)\n");
    fprintf(out, "  addi $sp, $sp, 4\n");
    clear_reg(reg);
}
void ir_READ(InterCode ir, FILE* out)
{
    fprintf(out, "  addi $sp, $sp, -4\n");
    fprintf(out, "  sw $ra, 0($sp)\n");
    fprintf(out, "  jal read\n");
    fprintf(out, "  lw $ra, 0($sp)\n");
    fprintf(out, "  addi $sp, $sp, 4\n");
    int reg = get_reg(ir->u.op, true, out);
    fprintf(out, "  move $%s, $v0\n", regs[reg].name);
    store_reg(reg, out);
}
void ir_ASSIGN_ADDR(InterCode ir, FILE* out)
{
    int left = get_reg(ir->u.assign.left, true, out);
    int right = get_reg(ir->u.assign.right, false, out);
    fprintf(out, "  move $%s, $%s\n", regs[left].name, regs[right].name);
    store_reg(left, out);
    clear_reg(right);
}
void ir_CALL(InterCode ir, FILE* out)
{
    fprintf(out, "  addi $sp, $sp, -8\n");
    fprintf(out, "  sw $fp, 0($sp)\n");
    fprintf(out, "  sw $ra, 4($sp)\n");
    fprintf(out, "  jal %s\n", ir->u.assign.right->u.funcname);
    fprintf(out, "  move $sp, $fp\n");
    fprintf(out, "  lw $ra, 4($sp)\n");
    fprintf(out, "  lw $fp, 0($sp)\n");
    fprintf(out, "  addi $sp, $sp, %d\n", 8 + ir->u.assign.right->size * 4);
    arg_num -= ir->u.assign.right->size;
    insert_op(ir->u.assign.left, out);
    int reg = get_reg(ir->u.assign.left, true, out);
    fprintf(out, "  move $%s, $v0\n", regs[reg].name);
    store_reg(reg, out);
}
void ir_LOAD(InterCode ir, FILE* out)
{
    int left, right;
    left = get_reg(ir->u.assign.left, true, out);
    right = get_reg(ir->u.assign.right, false, out);
    fprintf(out, "  lw $%s, 0($%s)\n", regs[left].name, regs[right].name);
    store_reg(left, out);
    clear_reg(right);
}
void ir_STORE(InterCode ir, FILE* out)
{
    int left, right;
    left = get_reg(ir->u.assign.left, false, out);
    right = get_reg(ir->u.assign.right, false, out);
    fprintf(out, "  sw $%s, 0($%s)\n", regs[right].name, regs[left].name);
    clear_reg(left);
    clear_reg(right);
}
void ir_MATH(InterCode ir, FILE* out)
{
    int res = get_reg(ir->u.binop.result, true, out);
    int op1 = get_reg(ir->u.binop.op1, false, out);
    int op2 = get_reg(ir->u.binop.op2, false, out);
    char* arith_op = NULL;
    switch (ir->kind)
    {
    case IC_ADD:
        arith_op = "add";
        break;
    case IC_SUB:
        arith_op = "sub";
        break;
    case IC_MUL:
        arith_op = "mul";
        break;
    case IC_DIV:
        arith_op = "div";
        break;
    default:
        break;
    }
    if (ir->kind == IC_DIV)
    {
        fprintf(out, "  %s $%s,$%s\n", arith_op, regs[op1].name, regs[op2].name);
        fprintf(out, "  mflo $%s\n", regs[res].name);
    }
    else
    {
        fprintf(out, "  %s $%s, $%s, $%s\n", arith_op, regs[res].name, regs[op1].name, regs[op2].name);
    }
    store_reg(res, out);
    clear_reg(op1);
    clear_reg(op2);
}
void ir_DEC(InterCode ir, FILE* out)
{
}
void ir_IF(InterCode ir, FILE* out)
{
    int x = get_reg(ir->u.if_goto.op1, false, out);
    int y = get_reg(ir->u.if_goto.op2, false, out);
    if (old_offset1 == 1)
    {
        old_offset1 = local_offset;
    }
    else if (old_offset2 == 1)
    {
        old_offset2 = local_offset;
    }
    else if (old_offset3 == 1)
    {
        old_offset3 = local_offset;
    }
    char* rel_op = NULL;
    if (strcmp(ir->u.if_goto.relop, "==") == 0)
    {
        rel_op = "beq";
    }
    else if (strcmp(ir->u.if_goto.relop, "!=") == 0)
    {
        rel_op = "bne";
    }
    else if (strcmp(ir->u.if_goto.relop, ">") == 0)
    {
        rel_op = "bgt";
    }
    else if (strcmp(ir->u.if_goto.relop, "<") == 0)
    {
        rel_op = "blt";
    }
    else if (strcmp(ir->u.if_goto.relop, ">=") == 0)
    {
        rel_op = "bge";
    }
    else if (strcmp(ir->u.if_goto.relop, "<=") == 0)
    {
        rel_op = "ble";
    }
    fprintf(out, "  %s $%s, $%s, label%d\n", rel_op, regs[x].name, regs[y].name, ir->u.if_goto.op3->u.labelno);
    clear_reg(x);
    clear_reg(y);
}