#include "compiler.h"
#include <stdlib.h>

void compiler_free(Compiler *compiler)
{
    table_free(compiler->strings);
    table_free(compiler->globals);
    table_free(compiler->variables);
}
void init_compiler(Compiler *compiler, FunctionType type)
{
    compiler->function = new_function();
    compiler->type = type;
    compiler->local_count = 0;

    if (type == TYPE_SCRIPT)
    {
        compiler->strings = init_table();
        compiler->globals = init_table();
        compiler->variables = init_table();
    }
    Local *local = &compiler->locals[compiler->local_count++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
}

ObjFunction *compiler_end(Compiler *compiler)
{
    chunk_push(compiler->function->chunk, OP_RETURN);
    ObjFunction *entry = compiler->function;
    return entry;
}

int compiler_resolve_local(Compiler *compiler, Token token)
{
    int i = compiler->local_count - 1;
    for (; i >= 0; i--)
    {
        Local *local = &compiler->locals[i];
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
void compiler_dump(Compiler *compiler, FILE *stream)
{
    fprintf(stream, "==== .data ====\n");
    values_dump(compiler->function->values, stream);
    fprintf(stream, "==== .text ====\n");
    fprintf(stream, ".%s\n", compiler->function->name != NULL ? compiler->function->name->chars : "entry");
    chunk_dump(compiler->function->chunk, stream);
}