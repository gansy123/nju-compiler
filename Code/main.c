#include <stdio.h>
#include <stdbool.h>
#include "syntax.tab.h"
extern bool lexical_error;
extern bool syntax_error;
extern int yylex();
extern FILE *yyin;
YYSTYPE yylval;
int yyparse(void);
void yyrestart(FILE *input_file);
void print_tree(struct node *root, int depth);
extern struct node;
extern struct node *root;
extern bool translate_begin;
void Program(struct node *n);
void translate_Program(struct node *n);
void print_IR(FILE *out);
void generate_objectcode(FILE *out);
int main(int argc, char **argv)
{
    if (argc <= 1)
        return 1;
    FILE *f = fopen(argv[1], "r");
    FILE *out;
    if (!f)
    {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    // yydebug = 1;
    yyparse();
    fclose(f);
    Program(root);
    if (translate_begin)
    {
        if (argv[2] == NULL)
        {
            out = fopen("output.s", "w");
        }
        else
            out = fopen(argv[2], "w");
        FILE *out2 = fopen("out.txt", "w");
        translate_Program(root);
        print_IR(out2);
        generate_objectcode(out);
        fclose(out);
    }
    // print_tree(root, 0);
    return 0;
}
