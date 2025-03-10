#ifndef AST_H
#define AST_H
#include "token.h"
#include "array.h"
#include "VM.h"

typedef enum
{
    AST_NUMBER,
    AST_GROUP,
    AST_BINARY,
    AST_TERNARY,
    AST_UNARY,
    AST_COMPOUND,
    AST_STMT,
    AST_ID,
    AST_FALSE,
    AST_TRUE,
    AST_NULL,
    AST_STRING,
    AST_POSTFIX,
    AST_FUNCTION_CALL,
    AST_SEQUENCEEXPR,
    AST_IF,
    AST_VAR,
    AST_WHILE,
    AST_FOR_LOOP,
    AST_BLOCK,
    AST_FUNCTION_DECL,
} AST_Type;
typedef struct AST_STRUCT AST;
struct AST_STRUCT
{
    char *name;
    AST_Type type;
    double number;
    AST *value;
    AST *left;
    AST *right;
    Token token;
    struct
    {
        AST **items;
        size_t count;
        size_t capacity;
    } childs;
};
AST *init_ast(AST_Type type);
char *ast_type_to_str(int type);
void ast_print(AST *root);
size_t ast_push(AST *ast, AST *child);
void ast_free(AST *ast);
void ast_to_byte(AST *ast, VM *vm);
#endif