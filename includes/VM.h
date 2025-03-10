#ifndef VM_H
#define VM_H
#include "chunk.h"
#include "table.h"
#include <stdio.h>
#include "token.h"

typedef struct
{
    Value *items;
    size_t count;
    size_t capacity;
} Stack;

#define UINT8_COUNT (UINT8_MAX + 1)

typedef struct
{
    Token name;
    int depth;
} Local;

typedef enum {
  TYPE_FUNCTION,
  TYPE_SCRIPT
} FunctionType;

typedef struct
{
    Chunk *chunk;
    Values *values;
    Stack *stack;
    Value *sp;
    byte *ip;
    int row;
    int col;
    bool had_error;
    char *message;
    Table *strings;
    Table *globals;
    Table *variables;
    char *file_path;
    ObjFunction *function;
    FunctionType type;
    int scope_depth;
    int local_count;
    Local locals[UINT8_COUNT];
    uint8_t break_stack[UINT8_COUNT];
    uint8_t break_stack_count;
    uint8_t continue_stack[UINT8_COUNT];
    uint8_t continue_stack_count;
} VM;

typedef enum
{
    VM_OK,
    VM_COMPILE_ERROR,
    VM_TYPE_ERROR,
    VM_ILLEGAL_INSTRUCTION,
    VM_STACK_OVERFLOW,
    VM_STACK_UNDERFLOW,
    VM_REFERENCE_ERROR,
} VM_Error;

VM *init_vm();
void vm_free(VM *vm);
void vm_dump(VM *vm, FILE *stream);
VM_Error vm_interpret(VM *vm);
int vm_resolve_local(VM *vm, Token token);
#endif