#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>

typedef struct Type_* Type;
typedef struct FieldList_* FieldList;
typedef struct Hashcode_* Hashcode;
typedef struct Structure_* Structure;
typedef struct Function_* Function;
Hashcode hashtab[1024];

struct Type_ {
    enum { BASIC, CONST, ARRAY, STRUCTURE, FUNCTION } kind;
    union {
        enum { BASIC_INT, BASIC_FLOAT } basic;
        struct {
            Type elem;
            int size;
        } array;
        Structure structure;
        Function function;
    } u;
    enum {
        LEFT,
        RIGHT,
        BOTH
    } assign;
    bool def;
};

struct FieldList_ {
    char *name;
    Type type;
    FieldList tail;
    int id;
};

struct Structure_{
    char *name;
    FieldList domain;
};

struct Function_{
    char *name;
    int line;
    Type ret;
    FieldList param;
    FieldList next;
    bool def;
};

struct Hashcode_{
    FieldList data;
    Hashcode next;
};

void initial_hashtab()
{
    for(int i = 0; i < 1024; i++)
    {
        hashtab[i] = NULL;
    }
}

int get_key(char* name)
{
    if(name == NULL) return -1;
    unsigned int hash = 5381;
    while(*name){
        int temp = ( hash << 5 ) + (*name++);
        hash = temp + hash;
    }
    return (hash & 0x7FFFFFFF) % 1024;
}

FieldList look_up(char* name)
{
    // printf("    lookup %s\n",name);
    if(name == NULL) return NULL;
    int key = get_key(name);
    Hashcode code = hashtab[key];  
    while(code != NULL && strcmp(code->data->name, name) != 0)
    {
        code = code->next;
    }
    if(code != NULL)
    {
        return code->data;
    }
    else 
        return NULL;
}

bool insert_node(FieldList field)
{
    char* name = field->name;
    if(name == NULL) return false;
    int key = get_key(name);
    Hashcode newcode = (Hashcode)malloc(sizeof(struct Hashcode_));
    newcode->data = field;
    newcode->next = hashtab[key];
    hashtab[key] = newcode;
    // printf("    insert %s\n",hashtab[key]->data->name);
    return true;
}



