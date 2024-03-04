#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct Type_ *Type;
typedef struct FieldList_ *FieldList;
typedef struct Hashcode_ *Hashcode;
typedef struct Structure_ *Structure;
typedef struct Function_ *Function;
bool translate_begin = true;
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

extern Hashcode hashtab[1024];

void initial_hashtab();
int get_key(char *name);
FieldList look_up(char *name);
bool insert_node(FieldList field);

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
extern struct node *root;

struct node *get_child(struct node *n, int num);
bool check_type(Type left, Type right);
void Program(struct node *n);
void ExtDefList(struct node *n);
void ExtDef(struct node *n);
void ExtDecList(struct node *n, Type type);
Type Specifier(struct node *n);
Type StructSpecifier(struct node *n);
char *OptTag(struct node *n);
char *Tag(struct node *n);
FieldList VarDec(struct node *n, Type t);
void FunDec(struct node *n, Type type, bool dec);
FieldList VarList(struct node *n, bool dec);
FieldList ParamDec(struct node *n, bool dec);
void CompSt(struct node *n, Type type);
void StmtList(struct node *n, Type type);
void Stmt(struct node *n, Type type);
void DefList(struct node *n, FieldList field);
void Def(struct node *n, FieldList field);
void DecList(struct node *n, FieldList field, Type type);
void Dec(struct node *n, FieldList field, Type type);
Type Exp(struct node *n);
FieldList Args(struct node *n);

Function head = NULL;

bool debug_flag = false;
void debug(struct node *n)
{
    if (debug_flag)
    {
        printf("%s\n", n->name);
    }
}

struct node *get_child(struct node *n, int num)
{
    struct node *ret = n->left;
    for (int i = 0; i < num; i++)
    {
        if (ret != NULL)
        {
            ret = ret->right;
        }
    }
    return ret;
}

bool check_type(Type left, Type right)
{
    if (left == NULL || right == NULL)
        return false;
    if (left->kind == BASIC && right->kind == CONST && left->u.basic == right->u.basic)
        return true;
    else if (left->kind == CONST && right->kind == BASIC && left->u.basic == right->u.basic)
        return true;
    else if (left->kind == BASIC && right->kind == BASIC && left->u.basic == right->u.basic)
        return true;
    else if (left->kind == CONST && right->kind == CONST && left->u.basic == right->u.basic)
        return true;
    else if (left->kind == ARRAY && right->kind == ARRAY && check_type(left->u.array.elem, right->u.array.elem))
        return true;
    else if (left->kind == STRUCTURE && right->kind == STRUCTURE && strcmp(left->u.structure->domain->name, right->u.structure->domain->name) == 0)
        return true;
    else if (left->kind == FUNCTION && right->kind == FUNCTION && left->u.function == right->u.function)
        return true;
    return false;
}
void check_dec()
{
    while (head != NULL)
    {
        if (!look_up(head->name)->type->u.function->def)
        {
            printf("Error type %d at Line %d: .\n", 18, head->line);
        }
        else
        {
            FieldList decla = look_up(head->name);
            if (!check_type(head->ret, decla->type->u.function->ret))
            {
                printf("Error type %d at Line %d: .\n", 18, head->line);
                return;
            }
            else
            {
                FieldList param1 = head->param;
                FieldList param2 = decla->type->u.function->param;
                while (param1 != NULL && param2 != NULL)
                {
                    if (!check_type(param1->type, param2->type))
                    {
                        printf("Error type %d at Line %d: .\n", 18, head->line);
                        return;
                    }
                    param1 = param1->tail;
                    param2 = param2->tail;
                }
                if (param1 != NULL && param2 != NULL)
                {
                    printf("Error type %d at Line %d: .\n", 18, head->line);
                    return;
                }
            }
        }
        head = head->next;
    }
}

void Program(struct node *n)
{
    // ExDefList
    if (n == NULL)
        return;
    debug(n);
    initial_hashtab();
    FieldList field1 = (FieldList)malloc(sizeof(struct FieldList_));
    char r1[5];
    char r2[5];
    field1->name = r1;
    strcpy(r1, "read");
    strcpy(r2, "read");
    field1->type = (Type)malloc(sizeof(struct Type_));
    field1->type->kind = FUNCTION;
    field1->type->def = true;
    field1->type->u.function = (Function)malloc(sizeof(struct Function_));
    field1->type->u.function->def = true;
    field1->type->u.function->name = r2;
    field1->type->u.function->param = NULL;
    field1->type->u.function->ret = (Type)malloc(sizeof(struct Type_));
    field1->type->u.function->ret->kind = BASIC;
    field1->type->u.function->ret->u.basic = BASIC_INT;
    insert_node(field1);
    FieldList field = (FieldList)malloc(sizeof(struct FieldList_));
    char w1[5];
    char w2[5];
    field->name = w1;
    strcpy(w1, "write");
    field->type = (Type)malloc(sizeof(struct Type_));
    field->type->kind = FUNCTION;
    field->type->def = true;
    field->type->u.function = (Function)malloc(sizeof(struct Function_));
    field->type->u.function->def = true;
    field->type->u.function->name = w2;
    field->type->u.function->param = (FieldList)malloc(sizeof(struct FieldList_));
    field->type->u.function->param->type = (Type)malloc(sizeof(struct Type_));
    field->type->u.function->param->type->kind = BASIC;
    field->type->u.function->param->type->u.basic = BASIC_INT;
    field->type->u.function->ret = (Type)malloc(sizeof(struct Type_));
    field->type->u.function->ret->kind = BASIC;
    field->type->u.function->ret->u.basic = BASIC_INT;
    insert_node(field);
    ExtDefList(get_child(n, 0));
    check_dec();
}

void ExtDefList(struct node *n)
{
    // ExDef ExDefList
    if (n == NULL)
        return;
    debug(n);
    ExtDef(get_child(n, 0));
    ExtDefList(get_child(n, 1));
}

void ExtDef(struct node *n)
{
    // Specifier ExtDecList SEMI
    // Specifier SEMI
    // Specifier FunDec CompSt
    // Specifier FunDec SEMI
    if (n == NULL)
        return;
    debug(n);
    struct node *child1 = get_child(n, 0);
    struct node *child2 = get_child(n, 1);
    struct node *child3 = get_child(n, 2);
    Type type = Specifier(child1);
    if (n->child_num == 2)
    {
        ;
    }
    else if (n->child_num == 3)
    {
        if (strcmp(child2->name, "ExtDecList") == 0)
        {
            ExtDecList(child2, type);
        }
        else if (strcmp(child2->name, "FunDec") == 0)
        {
            if (strcmp(child3->name, "SEMI") == 0)
            {
                FunDec(child2, type, true);
            }
            else
            {
                FunDec(child2, type, false);
                CompSt(child3, type);
            }
        }
    }
}

void ExtDecList(struct node *n, Type type)
{
    // VarDec
    // VarDec COMMA ExtDecList

    if (n == NULL)
        return;
    debug(n);
    if (n->child_num == 1)
    {
        FieldList ins = VarDec(get_child(n, 0), type);
        if (look_up(ins->name) != NULL)
        {
            printf("Error type %d at Line %d: .\n", 3, n->line);
            return NULL;
        }
        insert_node(ins);
    }
    else if (n->child_num == 3)
    {
        FieldList ins = VarDec(get_child(n, 0), type);
        if (look_up(ins->name) != NULL)
        {
            printf("Error type %d at Line %d: .\n", 3, n->line);
            return NULL;
        }
        insert_node(ins);
        ExtDecList(get_child(n, 2), type);
    }
}

Type Specifier(struct node *n)
{
    // TYPE
    // StructSpecifier
    if (n == NULL)
        return NULL;
    debug(n);
    Type type = NULL;
    struct node *child = get_child(n, 0);
    if (strcmp(child->name, "TYPE") == 0)
    {
        type = (Type)malloc(sizeof(struct Type_));
        type->kind = BASIC;
        if (strcmp(child->val.id, "int") == 0)
        {
            type->u.basic = BASIC_INT;
        }
        else
        {
            type->u.basic = BASIC_FLOAT;
        }
    }
    else if (strcmp(child->name, "StructSpecifier") == 0)
    {
        type = StructSpecifier(child);
    }
    return type;
}

Type StructSpecifier(struct node *n)
{
    // STRUCT OptTag LC DefList RC
    // STRUCT Tag
    if (n == NULL)
        return NULL;
    debug(n);

    Type type = NULL;
    FieldList field = (FieldList)malloc(sizeof(struct FieldList_));
    if (n->child_num == 5)
    {
        struct node *child2 = get_child(n, 1);
        struct node *child4 = get_child(n, 3);
        char *opttag = "";
        if (strcmp(child2->name, "OptTag") == 0)
            opttag = OptTag(child2);
        // printf("  %s\n",opttag);
        if (opttag != "" && look_up(opttag) != NULL)
        {
            printf("Error type %d at Line %d: .\n", 16, n->line);
            // semantic_error(16);
            return NULL;
        }
        field->name = opttag;
        field->tail = NULL;
        field->type = (Type)malloc(sizeof(struct Type_));
        field->type->kind = STRUCTURE;
        field->type->u.structure = (Structure)malloc(sizeof(struct Structure_));
        field->type->u.structure->name = NULL;
        field->type->u.structure->domain = NULL;
        field->type->def = false;
        if (opttag != "" && look_up(field->name) != NULL)
        {
            printf("Error type %d at Line %d: .\n", 3, n->line);
            return NULL;
        }
        if (strcmp(child2->name, "OptTag") == 0)
            DefList(child4, field);
        else
            DefList(get_child(n, 2), field);
        field->type->def = true;
        insert_node(field);
    }
    else if (n->child_num == 2)
    {
        char *tag = Tag(get_child(n, 1));
        // printf(" %s\n",tag);
        field = look_up(tag);
        if (field == NULL)
        {
            printf("Error type %d at Line %d: .\n", 17, n->line);
            return NULL;
        }
    }
    type = (Type)malloc(sizeof(struct Type_));
    type->kind = STRUCTURE;
    type->u.structure = (Structure)malloc(sizeof(struct Structure_));
    type->u.structure->name = n->val.id;
    type->u.structure->domain = field;
    return type;
}

char *OptTag(struct node *n)
{
    // ID
    if (n == NULL)
        return NULL;
    debug(n);
    return get_child(n, 0)->val.id;
};

char *Tag(struct node *n)
{
    // ID
    if (n == NULL)
        return NULL;
    debug(n);
    return get_child(n, 0)->val.id;
};

FieldList VarDec(struct node *n, Type t)
{
    // ID
    // VarDec LB INT RB

    if (n == NULL)
        return NULL;
    debug(n);
    if (n->child_num == 1)
    {
        FieldList field = (FieldList)malloc(sizeof(struct FieldList_));
        struct node *child = get_child(n, 0);
        field->name = child->val.id;
        // printf("%s\n", field->name);
        field->type = t;
        field->tail = NULL;
        return field;
    }
    else if (n->child_num == 4)
    {
        Type type = (Type)malloc(sizeof(struct Type_));
        struct node *child1 = get_child(n, 0);
        struct node *child3 = get_child(n, 2);
        type->kind = ARRAY;
        type->u.array.size = atoi(child3->val.id);
        type->u.array.elem = t;

        return VarDec(child1, type);
    }
    return NULL;
}

void FunDec(struct node *n, Type type, bool dec)
{
    // ID LP VarList RP
    // ID LP RP
    if (n == NULL)
        return;
    debug(n);
    if (dec)
    {
        struct node *id = get_child(n, 0);
        FieldList field = (FieldList)malloc(sizeof(struct FieldList_));
        ;
        if (n->child_num == 3)
        {
            field->name = id->val.id;
            field->tail = NULL;
            field->type = (Type)malloc(sizeof(struct Type_));
            field->type->kind = FUNCTION;
            field->type->u.function = (Function)malloc(sizeof(struct Function_));
            field->type->u.function->name = id->val.id;
            field->type->u.function->ret = type;
            field->type->u.function->param = NULL;
            field->type->u.function->def = false;
            field->type->u.function->line = n->line;
        }
        else if (n->child_num == 4)
        {
            field->name = id->val.id;
            field->tail = NULL;
            field->type = (Type)malloc(sizeof(struct Type_));
            field->type->kind = FUNCTION;
            field->type->u.function = (Function)malloc(sizeof(struct Function_));
            field->type->u.function->name = id->val.id;
            field->type->u.function->ret = type;
            field->type->u.function->param = VarList(get_child(n, 2), true);
            field->type->u.function->def = false;
            field->type->u.function->line = n->line;
        }
        if (look_up(field->name) == NULL)
        {
            Function tail = field->type->u.function;
            tail->next = head;
            head = tail;
            insert_node(field);
        }
        else if (look_up(field->name)->type->kind == FUNCTION)
        {
            FieldList decla = look_up(field->name);
            if (!check_type(field->type->u.function->ret, decla->type->u.function->ret))
            {
                printf("Error type %d at Line %d: .\n", 19, n->line);
                return;
            }
            else
            {
                FieldList param1 = field->type->u.function->param;
                FieldList param2 = decla->type->u.function->param;
                while (param1 != NULL && param2 != NULL)
                {
                    if (!check_type(param1->type, param2->type))
                    {
                        printf("Error type %d at Line %d: .\n", 19, n->line);
                        return;
                    }
                    param1 = param1->tail;
                    param2 = param2->tail;
                }
                if (param1 != NULL && param2 != NULL)
                {
                    printf("Error type %d at Line %d: .\n", 19, n->line);
                    return;
                }
                else
                {
                    Function tail = field->type->u.function;
                    tail->next = head;
                    head = tail;
                    return;
                }
            }
        }
        else
        {
            printf("Error type %d at Line %d: .\n", 4, n->line);
        }
    }
    else
    {
        struct node *id = get_child(n, 0);
        FieldList field = (FieldList)malloc(sizeof(struct FieldList_));
        ;
        if (n->child_num == 3)
        {
            field->name = id->val.id;
            field->tail = NULL;
            field->type = (Type)malloc(sizeof(struct Type_));
            field->type->kind = FUNCTION;
            field->type->u.function = (Function)malloc(sizeof(struct Function_));
            field->type->u.function->name = id->val.id;
            field->type->u.function->ret = type;
            field->type->u.function->param = NULL;
            field->type->u.function->def = true;
            field->type->u.function->line = n->line;
        }
        else if (n->child_num == 4)
        {
            field->name = id->val.id;
            field->tail = NULL;
            field->type = (Type)malloc(sizeof(struct Type_));
            field->type->kind = FUNCTION;
            field->type->u.function = (Function)malloc(sizeof(struct Function_));
            field->type->u.function->name = id->val.id;
            field->type->u.function->ret = type;
            field->type->u.function->param = VarList(get_child(n, 2), false);
            field->type->u.function->def = true;
            field->type->u.function->line = n->line;
        }
        if (look_up(field->name) == NULL)
        {
            // printf("  %s %d\n",field->name,field->type->kind);
            insert_node(field);
        }
        else if (look_up(field->name)->type->kind == FUNCTION && look_up(field->name)->type->u.function->def == false)
        {
            FieldList decla = look_up(field->name);
            if (!check_type(field->type->u.function->ret, decla->type->u.function->ret))
            {
                printf("Error type %d at Line %d: .\n", 19, n->line);
                insert_node(field);
                return;
            }
            else
            {
                FieldList param1 = field->type->u.function->param;
                FieldList param2 = decla->type->u.function->param;
                while (param1 != NULL && param2 != NULL)
                {
                    if (!check_type(param1->type, param2->type))
                    {
                        printf("Error type %d at Line %d: .\n", 19, n->line);
                        insert_node(field);
                        return;
                    }
                    param1 = param1->tail;
                    param2 = param2->tail;
                }
                if (param1 != NULL && param2 != NULL)
                {
                    printf("Error type %d at Line %d: .\n", 19, n->line);
                    insert_node(field);
                    return;
                }
                else
                {
                    insert_node(field);
                    return;
                }
            }
        }
        else
        {
            printf("Error type %d at Line %d: .\n", 4, n->line);
        }
    }
}

FieldList VarList(struct node *n, bool dec)
{
    // ParamDec COMMA VarList
    // ParamDec

    if (n == NULL)
        return NULL;
    debug(n);
    FieldList varlist = NULL;
    if (n->child_num == 1)
    {
        struct node *child = get_child(n, 0);
        varlist = ParamDec(child, dec);
        if (varlist == NULL)
            varlist = (FieldList)malloc(sizeof(struct FieldList_));
        varlist->tail = NULL;
    }
    else if (n->child_num == 3)
    {
        struct node *child = get_child(n, 0);
        varlist = ParamDec(child, dec);
        if (varlist == NULL)
            varlist = (FieldList)malloc(sizeof(struct FieldList_));
        varlist->tail = VarList(get_child(n, 2), dec);
    }
    return varlist;
}

FieldList ParamDec(struct node *n, bool dec)
{
    // Specifier VarDec

    if (n == NULL)
        return NULL;
    debug(n);
    Type type = Specifier(get_child(n, 0));
    FieldList field = VarDec(get_child(n, 1), type);
    if (look_up(field->name) != NULL)
    {
        if (!dec)
            printf("Error type %d at Line %d:  .\n", 3, n->line);
    }
    if (field->type->kind == STRUCTURE)
    {
        // printf(" %s\n",field->type->u.structure->domain->name);
        // if(field->type->u.structure->domain->type->kind == STRUCTURE)
        // printf("  %s\n",field->type->u.structure->domain->type->u.structure->domain->name);
    }
    if (!dec)
        insert_node(field);
    return field;
}

void CompSt(struct node *n, Type type)
{
    // LC DefList StmtList RC
    if (n == NULL)
        return;
    debug(n);
    if (strcmp(get_child(n, 1)->name, "StmtList") == 0)
    {
        StmtList(get_child(n, 1), type);
    }
    else
    {
        DefList(get_child(n, 1), NULL);
        StmtList(get_child(n, 2), type);
    }
}

void StmtList(struct node *n, Type type)
{

    // Stmt StmtList

    if (n == NULL || strcmp(n->name, "StmtList") != 0)
        return;
    debug(n);
    Stmt(get_child(n, 0), type);
    StmtList(get_child(n, 1), type);
}

void Stmt(struct node *n, Type type)
{
    // Exp SEMI
    // CompSt
    // RETURN Exp SEMI
    // IF LP Exp RP Stmt
    // IF LP Exp RP Stmt ELSE Stmt
    // WHILE LP Exp RP Stmt

    if (n == NULL)
        return;
    debug(n);
    if (n->child_num == 2)
    {
        Exp(get_child(n, 0));
    }
    else if (n->child_num == 1)
    {
        CompSt(get_child(n, 0), type);
    }
    else if (n->child_num == 3)
    {
        Type rettype = Exp(get_child(n, 1));
        if (rettype == NULL)
            return;
        if (!check_type(type, rettype))
        {
            printf("Error type %d at Line %d:  .\n", 8, n->line);
        }
    }
    else if (n->child_num == 5)
    {
        Exp(get_child(n, 2));
        Stmt(get_child(n, 4), type);
    }
    else if (n->child_num == 7)
    {
        Exp(get_child(n, 2));
        Stmt(get_child(n, 4), type);
        Stmt(get_child(n, 6), type);
    }
}

void DefList(struct node *n, FieldList field)
{

    // Def DefList
    if (n == NULL || strcmp(n->name, "DefList") != 0)
        return;
    debug(n);
    // printf("%s %d\n", field->name, field->type->kind);
    Def(get_child(n, 0), field);
    DefList(get_child(n, 1), field);
}

void Def(struct node *n, FieldList field)
{

    // Specifier DecList SEMI
    if (n == NULL)
        return;
    debug(n);
    // printf("%s %d\n", field->name, field->type->kind);
    Type type = Specifier(get_child(n, 0));
    DecList(get_child(n, 1), field, type);
}

void DecList(struct node *n, FieldList field, Type type)
{

    // Dec
    // Dec COMMA DecList
    if (n == NULL)
        return;
    debug(n);
    // printf("%s %d\n", field->name, field->type->kind);
    if (n->child_num == 1)
    {
        Dec(get_child(n, 0), field, type);
    }
    else if (n->child_num == 3)
    {
        Dec(get_child(n, 0), field, type);
        DecList(get_child(n, 2), field, type);
    }
}

void Dec(struct node *n, FieldList field, Type type)
{

    // VarDec
    // VarDec ASSIGNOP Exp
    if (n == NULL)
        return;
    debug(n);
    // printf("%s %d\n", field->name, field->type->kind);
    if (field == NULL)
    {
        field = (FieldList)malloc(sizeof(struct FieldList_));
    }
    if (field->type == NULL)
    {
        field->type = (Type)malloc(sizeof(struct Type_));
    }
    if (n->child_num == 1 || (field != NULL && field->type->kind == STRUCTURE))
    {
        if (field != NULL && field->type->kind == STRUCTURE)
        {
            if (n->child_num == 3)
            {
                printf("Error type %d at Line %d:  .\n", 15, n->line);
            }
            FieldList f = VarDec(get_child(n, 0), type);
            if (field->type->u.structure == NULL)
            {
                field->type->u.structure = (Structure)malloc(sizeof(struct Structure_));
                field->type->u.structure->domain = NULL;
            }
            FieldList l = field->type->u.structure->domain;
            char *s;
            s = f->name;
            while (l != NULL && (strcmp(s, l->name) != 0))
            {
                l = l->tail;
            }
            if (l != NULL)
            {
                printf("Error type %d at Line %d: .\n", 15, n->line);
                return NULL;
            }
            f->tail = field->type->u.structure->domain;
            field->type->u.structure->domain = f;
        }
        else
        {
            FieldList ins = VarDec(get_child(n, 0), type);
            if (look_up(ins->name) != NULL)
            {
                // printf("%s ", field->name);
                printf("Error type %d at Line %d: .\n", 3, n->line);
                return NULL;
            }
            insert_node(ins);
        }
    }
    else if (n->child_num == 3)
    {
        FieldList ins = VarDec(get_child(n, 0), type);
        if (look_up(ins->name) != NULL)
        {
            printf("Error type %d at Line %d: .\n", 3, n->line);
            return NULL;
        }
        insert_node(ins);
        Type right = Exp(get_child(n, 2));
        if (right == NULL)
            return NULL;
        if (!check_type(type, right))
        {
            printf("Error type %d at Line %d:  .\n", 5, n->line);
            return NULL;
        }
    }
}

Type Exp(struct node *n)
{
    // Exp ASSIGNOP Exp
    // Exp AND Exp
    // Exp OR Exp
    // Exp RELOP Exp
    // Exp PLUS Exp
    // Exp MINUS Exp
    // Exp STAR Exp
    // Exp DIV Exp
    // LP Exp RP
    // MINUS Exp
    // NOT Exp
    // ID LP Args RP
    // ID LP RP
    // Exp LB Exp RB
    // Exp DOT ID
    // ID
    // INT
    // FLOAT

    if (n == NULL)
        return NULL;
    debug(n);
    Type type = (Type)malloc(sizeof(struct Type_));
    if (n->child_num == 1)
    {
        struct node *child = get_child(n, 0);
        if (strcmp(child->name, "ID") == 0)
        {
            if (look_up(child->val.id) == NULL)
            {
                // printf("  %d %s\n",get_key(child->val.id), hashtab[get_key(child->val.id)]->data->name);
                printf("Error type %d at Line %d: %s.\n", 1, n->line, child->val.id);
                return NULL;
            }
            else
                type = look_up(child->val.id)->type;
        }
        else if (strcmp(child->name, "INT") == 0)
        {
            type->kind = CONST;
            type->u.basic = BASIC_INT;
        }
        else if (strcmp(child->name, "FLOAT") == 0)
        {
            type->kind = CONST;
            type->u.basic = BASIC_FLOAT;
        }
    }
    else if (n->child_num == 2)
    {
        struct node *child = get_child(n, 0);
        if (strcmp(child->name, "MINUS") == 0)
        {
            type = Exp(get_child(n, 1));
            if (type == NULL)
                return NULL;
            if (type->kind != CONST)
            {
                printf("Error type %d at Line %d: %s.\n", 7, n->line, child->val.id);
                return NULL;
            }
        }
        else
            type = Exp(get_child(n, 1));
        if (type == NULL)
            return NULL;
    }
    else if (n->child_num == 3)
    {
        struct node *child = get_child(n, 0);
        if (strcmp(child->name, "ID") == 0)
        {
            if (look_up(child->val.id) == NULL)
            {
                printf("Error type %d at Line %d: .\n", 2, n->line);
                return NULL;
            }
            else if (strcmp(child->val.id, "read") == 0)
            {
                return NULL;
            }
            else if (look_up(child->val.id)->type->kind != FUNCTION)
            {
                printf("Error type %d at Line %d: .\n", 11, n->line);
                return Exp(child);
            }
            else if (look_up(child->val.id)->type->u.function->param != NULL)
            {
                printf("Error type %d at Line %d: .\n", 9, n->line);
            }
            else
                type = look_up(child->val.id)->type->u.function->ret;
        }
        else if (strcmp(child->name, "LP") == 0)
        {
            type = Exp(get_child(n, 1));
            if (type == NULL)
                return NULL;
        }
        else if (strcmp(get_child(n, 1)->name, "DOT") == 0)
        {
            Type t = Exp(child);
            if (t == NULL)
                return NULL;
            if (t->kind != STRUCTURE)
            {
                printf("Error type %d at Line %d: %d.%s .\n", 13, n->line, t->kind, get_child(n, 2)->val.id);
                return NULL;
            }
            char *s = get_child(n, 2)->val.id;
            FieldList l = t->u.structure->domain->type->u.structure->domain;
            if (strcmp(t->u.structure->domain->name, t->u.structure->domain->type->u.structure->domain->name) == 0)
            {
                l = l->type->u.structure->domain;
            }
            while (l != NULL && strcmp(s, l->name) != 0)
            {
                // printf("  %s\n",l->name);
                l = l->tail;
            }
            if (l != NULL)
            {
                type = l->type;
            }
            else
            {
                printf("Error type %d at Line %d: %s.\n", 14, n->line, s);
                return NULL;
            }
        }
        else if (strcmp(get_child(n, 1)->name, "ASSIGNOP") == 0)
        {
            type = Exp(child);
            if (type == NULL)
                return NULL;
            // printf("  %d", type->kind);
            Type right = Exp(get_child(n, 2));
            if (right == NULL)
                return NULL;
            if (type->kind == CONST || type->kind == FUNCTION || type->assign == RIGHT)
            {
                printf("Error type %d at Line %d: .\n", 6, n->line);
                return NULL;
            }
            if (!check_type(type, right))
            {
                printf("Error type %d at Line %d: .\n", 5, n->line);
                return NULL;
            }
        }
        else
        {
            // printf("  %s\n",get_child(n,1)->name);
            type = Exp(child);
            // printf("here\n");
            if (type == NULL)
                return NULL;
            Type right = Exp(get_child(n, 2));
            if (right == NULL)
                return NULL;
            if (!check_type(type, right))
            {
                // printf(" %d %d %s\n",type->kind,right->kind, get_child(n,1)->name);
                printf("Error type %d at Line %d: .\n", 7, n->line);
                return NULL;
            }
            if (strcmp(get_child(n, 1)->name, "RELOP") == 0)
            {
                type->u.basic = BASIC_INT;
            }
        }
    }
    else if (n->child_num == 4)
    {

        // printf("%s\n", get_child(n, 0)->val.id);
        // fflush(stdout);
        struct node *child = get_child(n, 0);
        if (strcmp(child->name, "ID") == 0)
        {
            if (look_up(child->val.id) == NULL)
            {
                printf("Error type %d at Line %d: .\n", 2, n->line);
                return NULL;
            }
            else if (look_up(child->val.id)->type->kind != FUNCTION)
            {
                printf("Error type %d at Line %d: this is %d.\n", 11, n->line, look_up(child->val.id)->type->kind);
                return Exp(child);
            }
            else if (strcmp(child->val.id, "write") == 0)
            {
                return NULL;
            }
            else
            {
                if (look_up(child->val.id)->type->u.function->param == NULL)
                {
                    printf("Error type %d at Line %d: .\n", 9, n->line);
                    return NULL;
                }
                else
                {
                    FieldList args = Args(get_child(n, 2));
                    FieldList fargs = look_up(child->val.id)->type->u.function->param;
                    while (args != NULL && fargs != NULL)
                    {
                        if (check_type(fargs->type, args->type))
                        {
                            args = args->tail;
                            fargs = fargs->tail;
                        }
                        else
                        {
                            printf("Error type %d at Line %d: .\n", 9, n->line);
                            return NULL;
                        }
                    }
                    if (args != NULL || fargs != NULL)
                    {
                        printf("Error type %d at Line %d: .\n", 9, n->line);
                        return NULL;
                    }
                }
            }
            look_up(child->val.id)->type->u.function->ret->assign = RIGHT;
            type = look_up(child->val.id)->type->u.function->ret;
        }
        else if (strcmp(child->name, "Exp") == 0)
        {
            if (child->child_num > 1 && strcmp(get_child(child, 1)->name, "LB") == 0)
            {
                translate_begin = false;
                printf("Cannot translate: Code contains variables of multi-dimensional array type or parameters of array type.\n");
            }
            Type arr = Exp(child);
            if (arr == NULL)
                return NULL;
            if (arr->kind == ARRAY)
            {
                Type idx = Exp(get_child(n, 2));
                if (idx == NULL)
                    return NULL;
                if (idx->kind > 1 || idx->u.basic != BASIC_INT)
                {
                    printf("Error type %d at Line %d: .\n", 12, n->line);
                }
                type = Exp(child)->u.array.elem;
            }
            else
            {
                printf("Error type %d at Line %d: .\n", 10, n->line);
                return NULL;
            }
        }
    }
    return type;
}

FieldList Args(struct node *n)
{
    // Exp
    // Exp COMMA Args

    if (n == NULL)
        return NULL;
    debug(n);
    FieldList field = (FieldList)malloc(sizeof(struct FieldList_));
    struct node *child = get_child(n, 0);
    if (n->child_num == 1)
    {
        field->name = child->name;
        field->tail = NULL;
        field->type = Exp(get_child(n, 0));
        if (field->type->kind == ARRAY)
        {
            translate_begin = false;
            printf("Cannot translate: Code contains variables of multi-dimensional array type or parameters of array type.\n");
        }
        // printf("  here %s\n",field->type->u.structure->name);
        return field;
    }
    else if (n->child_num == 3)
    {
        field->name = child->name;
        field->tail = NULL;
        field->type = Exp(get_child(n, 0));
        field->tail = Args(get_child(n, 2));
        if (field->type->kind == ARRAY)
        {
            translate_begin = false;
            printf("Cannot translate: Code contains variables of multi-dimensional array type or parameters of array type.\n");
        }
        return field;
    }
    return NULL;
}
