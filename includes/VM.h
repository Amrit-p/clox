#ifndef VM_H
#define VM_H
#include "compiler.h"
#include "table.h"

#define VM_CURRENT_CHUNK (vm->function->chunk)
#define VM_CURRENT_CHUNK_BASE (vm->function->chunk->items)
#define STACK_SIZE 256

typedef struct
{
    Value items[STACK_SIZE];
    size_t count;
} Stack;

typedef struct
{
    ObjFunction *function;
    uint8_t *ip;
    Value *slots;
} CallFrame;

typedef struct
{
    Values *values;
    Stack stack;
    Value *sp;
    uint8_t *ip;
    int row;
    int col;
    char *message;
    Table *strings;
    Table *globals;
    Table *variables;
    char *file_path;
    ObjFunction *function;
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

VM *init_vm(Compiler *compiler);
void vm_free(VM *vm);
void vm_dump(VM *vm, FILE *stream);
VM_Error vm_interpret(VM *vm);
#endif