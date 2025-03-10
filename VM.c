#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "VM.h"
#include "object.h"
#include "helper.h"

#define STACK_SIZE 256

Stack *init_stack()
{
    Stack *stack = calloc(1, sizeof(Stack));
    init_array(stack);
    return stack;
}
VM *init_vm()
{
    VM *vm = calloc(1, sizeof(VM));
    vm->values = init_values();
    vm->chunk = init_chunk();
    vm->stack = init_stack();
    vm->strings = init_table();
    vm->globals = init_table();
    vm->variables = init_table();
    vm->ip = vm->chunk->items;
    vm->sp = vm->stack->items;
    // vm->function = new_function();
    // vm->function->name = new_string("main", 4);
    // vm->type = TYPE_SCRIPT;
    // vm->function->chunk = init_chunk();
    return vm;
}
void vm_free(VM *vm)
{
    chunk_free(vm->chunk);
    values_free(vm->values);
    array_free(vm->stack);
    free(vm->stack);
    table_free(vm->strings);
    table_free(vm->globals);
    table_free(vm->variables);
    free(vm);
}
void vm_dump(VM *vm, FILE *stream)
{
    fprintf(stream, "==== .data ====\n");
    values_dump(vm->values, stream);
    fprintf(stream, "==== .text ====\n");
    chunk_dump(vm->chunk, stream);
}
Value vm_stack_peek(VM *vm, size_t offset)
{
    return array_at(vm->stack, offset);
}

char *value_typeof(Value value)
{
    switch (value.type)
    {
    case VAL_BOOL:
        return "Boolean";
    case VAL_NULL:
        return "null";
    case VAL_NUMBER:
        return "Number";
    case VAL_OBJ:
    {
        Obj *obj = AS_OBJ(value);
        switch (obj->type)
        {
        case OBJ_STRING:
            return "String";
        default:
            return "Object";
        }
    }
    default:
        return "UNKNOWN";
    }
}
bool is_falsey(Value value)
{
    return IS_NULL(value) ||
           (IS_NUMBER(value) && AS_NUMBER(value) == 0) ||
           (IS_STRING(value) && AS_STRING(value)->length == 0) ||
           (IS_BOOL(value) && !AS_BOOL(value));
}
VM_Error vm_interpret(VM *vm)
{
    vm->ip = vm->chunk->items;

#define VM_STACK_PUSH(value)                     \
    do                                           \
    {                                            \
        if (array_size(vm->stack) >= STACK_SIZE) \
        {                                        \
            vm->message = "stack overflow";      \
            return VM_STACK_OVERFLOW;            \
        }                                        \
        array_push(vm->stack, value);            \
        if (vm->sp == NULL)                      \
            vm->sp = vm->stack->items;           \
        vm->sp++;                                \
    } while (0)

#define VM_STACK_POP(ident)                  \
    do                                       \
    {                                        \
        if (array_size(vm->stack) == 0)      \
        {                                    \
            vm->message = "stack underflow"; \
            return VM_STACK_UNDERFLOW;       \
        }                                    \
        (void)array_pop(vm->stack);          \
        vm->sp--;                            \
        ident = *vm->sp;                     \
    } while (0)

#define VM_STACK_PEEK(ident, offset)         \
    do                                       \
    {                                        \
        if (array_size(vm->stack) == 0)      \
        {                                    \
            vm->message = "stack underflow"; \
            return VM_STACK_UNDERFLOW;       \
        }                                    \
        ident = vm->sp[-1 - offset];         \
    } while (0)

#define VM_READ_BYTE() (*(vm)->ip++)
#define VM_READ_CONSTANT() (vm->values->items[VM_READ_BYTE()])
#define VM_READ_STRING() AS_STRING(VM_READ_CONSTANT())
#define VM_READ_SHORT() (vm->ip += 2, (uint16_t)((vm->ip[-2] << 8) | vm->ip[-1]))
#define VM_TYPE_ERROR(must, given, _row, _col)                                          \
    do                                                                                  \
    {                                                                                   \
        char *template = "Operand must be a type of " must ".But given type is \"%s\""; \
        char *message = calloc(strlen(template) + strlen((given)) + 1, sizeof(char));   \
        sprintf(message, template, (given));                                            \
        vm->message = message;                                                          \
        vm->row = (_row);                                                               \
        vm->col = (_col);                                                               \
        return VM_TYPE_ERROR;                                                           \
    } while (0)

#define VM_REF_ERROR(obj_str, value)                                                    \
    do                                                                                  \
    {                                                                                   \
        char *template = "'%s' is not defined";                                         \
        char *message = calloc(strlen(template) + (obj_str)->length + 1, sizeof(char)); \
        sprintf(message, template, (obj_str)->chars);                                   \
        vm->row = (value).row;                                                          \
        vm->col = (value).col;                                                          \
        vm->message = message;                                                          \
        return VM_REFERENCE_ERROR;                                                      \
    } while (0);

#define LOGICAL_OP(op)                                                       \
    do                                                                       \
    {                                                                        \
        if (array_size(vm->stack) < 2)                                       \
        {                                                                    \
            vm->message = "stack underflow";                                 \
            return VM_STACK_UNDERFLOW;                                       \
        }                                                                    \
        Value b = {0};                                                       \
        VM_STACK_POP(b);                                                     \
        Value a = {0};                                                       \
        VM_STACK_POP(a);                                                     \
        if (b.type != a.type)                                                \
        {                                                                    \
            VM_STACK_PUSH(BOOL_VAL(false, 0, 0));                            \
        }                                                                    \
        else                                                                 \
            switch (b.type)                                                  \
            {                                                                \
            case VAL_BOOL:                                                   \
                VM_STACK_PUSH(BOOL_VAL(AS_BOOL(a) op AS_BOOL(b), 0, 0));     \
                break;                                                       \
            case VAL_NUMBER:                                                 \
                VM_STACK_PUSH(BOOL_VAL(AS_NUMBER(a) op AS_NUMBER(b), 0, 0)); \
                break;                                                       \
            case VAL_NULL:                                                   \
                VM_STACK_PUSH(BOOL_VAL(true, 0, 0));                         \
                break;                                                       \
            case VAL_OBJ:                                                    \
                VM_STACK_PUSH(BOOL_VAL(AS_OBJ(a) op AS_OBJ(b), 0, 0));       \
                break;                                                       \
            default:                                                         \
                NOTREACHABLE;                                                \
            }                                                                \
    } while (0)

#define ARITHMETIC_OP(op, AS)                                       \
    do                                                              \
    {                                                               \
        if (array_size(vm->stack) < 2)                              \
        {                                                           \
            vm->message = "stack underflow";                        \
            return VM_STACK_UNDERFLOW;                              \
        }                                                           \
        Value b = {0};                                              \
        VM_STACK_POP(b);                                            \
        Value a = {0};                                              \
        VM_STACK_POP(a);                                            \
        if (!IS_NUMBER(b) || !IS_NUMBER(a))                         \
        {                                                           \
            int row = !IS_NUMBER(a) ? a.row : b.row;                \
            int col = !IS_NUMBER(a) ? a.col : b.col;                \
            char *given_type = value_typeof(!IS_NUMBER(a) ? a : b); \
            VM_TYPE_ERROR("Number", given_type, row, col);          \
        }                                                           \
        VM_STACK_PUSH(NUMBER_VAL(AS(a) op AS(b), b.row, b.col));    \
    } while (0)

#define UNARY_ARITHMETIC_OP(op, AS)                                    \
    do                                                                 \
    {                                                                  \
        Value value = {0};                                             \
        VM_STACK_POP(value);                                           \
        if (!IS_NUMBER(value))                                         \
        {                                                              \
            char *given_type = value_typeof(value);                    \
            VM_TYPE_ERROR("Number", given_type, value.row, value.row); \
        }                                                              \
        VM_STACK_PUSH(NUMBER_VAL(op AS(value), value.row, value.col)); \
    } while (0)

    for (;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        Value *slot = vm->stack->items;
        printf("          ");
        for (; slot < vm->sp; slot++)
        {
            printf("[");
            value_print(*slot, 0);
            printf("]");
        }
        printf("\n");
        chunk_print_instruction(vm->chunk, (size_t)(vm->ip - vm->chunk->items), stdout);
#endif
        byte instruction = VM_READ_BYTE();
        switch (instruction)
        {
        case OP_DUP:
        {
            Value value = {0};
            VM_STACK_PEEK(value, 0);
            VM_STACK_PUSH(value);
            break;
        }
        case OP_LOOP:
        {
            uint16_t offset = VM_READ_SHORT();
            vm->ip -= offset;
            break;
        }
        case OP_JMP:
        {
            uint16_t offset = VM_READ_SHORT();
            vm->ip = vm->chunk->items + offset;
            break;
        }
        case OP_JMP_IF_FALSE:
        {
            uint16_t offset = VM_READ_SHORT();
            Value value;
            VM_STACK_PEEK(value, 0);
            bool fval = is_falsey(value);
            if (fval)
                vm->ip = vm->chunk->items + offset;
            break;
        }
        case OP_POP:
        {
            Value value;
            VM_STACK_POP(value);
            (void)value;
            break;
        }
        case OP_SET_LOCAL:
        {
            byte slot = VM_READ_BYTE();
            Value value;
            VM_STACK_PEEK(value, 0);
            array_at(vm->stack, slot) = value;
            break;
        }
        case OP_GET_LOCAL:
        {
            byte slot = VM_READ_BYTE();
            Value name = VM_READ_CONSTANT();
            Value value = array_at(vm->stack, slot);
            value.row = name.row;
            value.col = name.col;
            VM_STACK_PUSH(value);
            break;
        }
        case OP_SET_GLOBAL:
        {
            Value name = VM_READ_CONSTANT();
            ObjString *str = AS_STRING(name);
            Value value = {0};
            VM_STACK_PEEK(value, 0);
            if (table_set(vm->globals, str, value))
            {
                table_remove(vm->globals, str);
                VM_REF_ERROR(str, value);
            }
            break;
        }
        case OP_DEFINE_GLOBAL:
        {
            ObjString *name = VM_READ_STRING();
            Value value = {0};
            VM_STACK_PEEK(value, 0);
            table_set(vm->globals, name, value);
            VM_STACK_POP(value);
            break;
        }
        case OP_GET_GLOBAL:
        {
            Value name = VM_READ_CONSTANT();
            ObjString *str = AS_STRING(name);
            Value value = {0};
            if (!table_get(vm->globals, str, &value))
            {
                VM_REF_ERROR(str, name);
            }
            value.row = name.row;
            value.col = name.col;
            VM_STACK_PUSH(value);
            break;
        }
        case OP_PRINT:
        {
            Value value = {0};
            VM_STACK_POP(value);
            value_print(value, 1);
            break;
        }
        case OP_RETURN:
        {
            // Value value = {0};
            // VM_STACK_POP(value);
            // (void)value;
            return VM_OK;
        }
        case OP_CONSTANT:
        {
            Value constant = VM_READ_CONSTANT();
            VM_STACK_PUSH(constant);
            break;
        }
        case OP_ADD:
        {
            Value b = {0};
            Value a = {0};
            VM_STACK_POP(b);
            VM_STACK_POP(a);
            if (IS_STRING(b) && IS_STRING(a))
            {
                ObjString *bString = AS_STRING(b);
                ObjString *aString = AS_STRING(a);
                ObjString *result = new_string(NULL, aString->length + bString->length);
                strcpy(result->chars, aString->chars);
                strcat(result->chars, bString->chars);
                result->hash = table_hash_string(result->chars, result->length);
                VM_STACK_PUSH(OBJ_VAL(result, 0, 0));
            }
            else if (IS_NUMBER(b) && IS_NUMBER(a))
            {
                VM_STACK_PUSH(NUMBER_VAL(AS_NUMBER(a) + AS_NUMBER(b), 0, 0));
            }
            else
            {
                uint32_t row = !IS_NUMBER(a) ? a.row : b.row;
                uint32_t col = !IS_NUMBER(a) ? a.col : b.col;
                char *given_type = value_typeof(!IS_NUMBER(a) ? a : b);
                VM_TYPE_ERROR("\"Number\"", given_type, row, col);
            }
            break;
        }
        case OP_SUBTRACT:
            ARITHMETIC_OP(-, AS_NUMBER);
            break;
        case OP_MULTIPLY:
            ARITHMETIC_OP(*, AS_NUMBER);
            break;
        case OP_DIVIDE:
            ARITHMETIC_OP(/, AS_NUMBER);
            break;
        case OP_MOD:
            ARITHMETIC_OP(%, AS_INTEGRAL);
            break;
        case OP_BITWISE_AND:
            ARITHMETIC_OP(&, AS_INTEGRAL);
            break;
        case OP_BITWISE_OR:
            ARITHMETIC_OP(|, AS_INTEGRAL);
            break;
        case OP_BITWISE_NOT:
            UNARY_ARITHMETIC_OP(~, AS_INTEGRAL);
            break;
        case OP_LEFT_SHIFT:
            ARITHMETIC_OP(<<, AS_INTEGRAL);
            break;
        case OP_RIGHT_SHIFT:
            ARITHMETIC_OP(>>, AS_INTEGRAL);
            break;
        case OP_EQUAL:
            LOGICAL_OP(==);
            break;
        case OP_NOT_EQUAL:
            LOGICAL_OP(!=);
            break;
        case OP_LT:
            LOGICAL_OP(<);
            break;
        case OP_LTE:
            LOGICAL_OP(<=);
            break;
        case OP_GT:
            LOGICAL_OP(>);
            break;
        case OP_GTE:
            LOGICAL_OP(>=);
            break;
        case OP_NOT:
        {
            Value val = {0};
            VM_STACK_POP(val);
            VM_STACK_PUSH(BOOL_VAL(is_falsey(val), val.row, val.col));
            break;
        }
        case OP_NEGATE:
        {
            UNARY_ARITHMETIC_OP(-, AS_NUMBER);
            break;
        }
        default:
            fprintf(stderr, "[ERROR] illegal instruction pointer(%zu)\n", (size_t)(vm->ip - vm->chunk->items) - 1);
            return VM_ILLEGAL_INSTRUCTION;
        }
    }
#undef VM_READ_BYTE
#undef VM_READ_CONSTANT
#undef VM_READ_SHORT
#undef VM_READ_STRING
#undef VM_STACK_PUSH
#undef VM_STACK_POP
#undef VM_TYPE_ERROR
}
int vm_resolve_local(VM *vm, Token token)
{
    int i = vm->local_count - 1;
    for (; i >= 0; i--)
    {
        Local *local = &vm->locals[i];
        if (token_equal(token, local->name))
        {
            if (local->depth == -1)
            {
                return -2;
            }
            break;
        }
    }
    return i;
}