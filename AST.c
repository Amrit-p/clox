#include "AST.h"
#include "helper.h"
#include "object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FILE_ROW_COL_FORMAT "%s:%zu:%zu"
#define AST_ERROR_PREFIX_FORMART "CompileError at " FILE_ROW_COL_FORMAT

#define AST_REDEC_ERROR(compiler, token, previous_row, previous_col)                                         \
    do                                                                                                       \
    {                                                                                                        \
        (compiler)->had_error = true;                                                                        \
        fprintf(stderr, AST_ERROR_PREFIX_FORMART " '%.*s' is already declared at " FILE_ROW_COL_FORMAT "\n", \
                (compiler)->file_path,                                                                       \
                (token).row,                                                                                 \
                (token).col,                                                                                 \
                (int)(token).length,                                                                         \
                (token).start,                                                                               \
                (compiler)->file_path,                                                                       \
                (size_t)(previous_row),                                                                      \
                (size_t)(previous_col));                                                                     \
    } while (0)

#define AST_REF_ERROR(compiler, token)                                       \
    do                                                                       \
    {                                                                        \
        (compiler)->had_error = true;                                        \
        fprintf(stderr, AST_ERROR_PREFIX_FORMART " '%.*s' is not defined\n", \
                (compiler)->file_path,                                       \
                (token).row,                                                 \
                (token).col,                                                 \
                (int)(token).length,                                         \
                (token).start);                                              \
    } while (0)

#define AST_VAR_SELF_INIT(compiler, token)                                                                    \
    do                                                                                                        \
    {                                                                                                         \
        compiler->had_error = true;                                                                           \
        fprintf(stderr, AST_ERROR_PREFIX_FORMART " cannot read variable '%.*s' in its own initialization.\n", \
                (compiler)->file_path,                                                                        \
                (token).row,                                                                                  \
                (token).col,                                                                                  \
                (int)(token).length,                                                                          \
                (token).start);                                                                               \
    } while (0)

AST *init_ast(AST_Type type)
{
    AST *ast = calloc(1, sizeof(AST));
    if (type == AST_COMPOUND ||
        type == AST_SEQUENCEEXPR ||
        type == AST_FOR_LOOP ||
        type == AST_BLOCK ||
        type == AST_FUNCTION_DECL ||
        type == AST_FUNCTION_CALL)
    {
        init_array(&ast->childs);
    }
    ast->type = type;
    return ast;
}
size_t ast_push(AST *ast, AST *child)
{
    size_t i = array_size(&ast->childs);
    array_push(&ast->childs, child);
    return i;
}
char *ast_to_str(AST *ast)
{
    return ast_type_to_str(ast->type);
}
char *ast_type_to_str(int type)
{
    switch (type)
    {
    case AST_NUMBER:
        return "NUMBER";
    case AST_BINARY:
        return "BINARYEXPR";
    case AST_UNARY:
        return "UNARYEXPR";
    case AST_GROUP:
        return "GROUP";
    case AST_COMPOUND:
        return "COMPOUND";
    case AST_STMT:
        return "STATEMENT";
    case AST_TERNARY:
        return "TERNARY";
    case AST_NULL:
        return "NULL";
    case AST_FALSE:
        return "FALSE";
    case AST_TRUE:
        return "TRUE";
    case AST_ID:
        return "ID";
    case AST_STRING:
        return "STRING";
    case AST_POSTFIX:
        return "POSTFIX";
    case AST_FUNCTION_CALL:
        return "FUNCTION_CALL";
    case AST_SEQUENCEEXPR:
        return "SEQUENCEEXPR";
    case AST_IF:
        return "IF";
    case AST_VAR:
        return "VAR";
    case AST_WHILE:
        return "WHILE";
    case AST_FOR_LOOP:
        return "FOR_LOOP";
    case AST_BLOCK:
        return "BLOCK";
    case AST_FUNCTION_DECL:
        return "AST_FUNCTION_DECL";
    default:
        return "UNKNOWN";
    }
}
char *ast_to_json(AST *ast)
{
    if (ast == NULL)
        return NULL;
    char *c = calloc(2, sizeof(char));
    strcpy(c, "{");
    char *type_key = "\"type\": \"AST_%s\"";
    char *type = calloc(strlen(type_key) + strlen(ast_type_to_str(ast->type)) + 1, sizeof(char));
    sprintf(type, type_key, ast_type_to_str(ast->type));
    c = realloc(c, strlen(c) + strlen(type) + 1 * sizeof(char));
    strcat(c, type);
    free(type);

    if (ast->name)
    {
        char *name_key = ",\"name\": \"%s\"";
        char *name = calloc(strlen(name_key) + strlen(ast->name) + 1, sizeof(char));
        sprintf(name, name_key, ast->name);
        c = realloc(c, strlen(c) + strlen(name) + 1 * sizeof(char));
        strcat(c, name);
        free(name);
    }
    if ((ast->type == AST_NUMBER))
    {
        char str[50];
        sprintf(str, "%f", ast->number);
        size_t digits = strlen(str);
        char *int_value_key = ",\"number\": \"%f\"";
        char *number = calloc(strlen(int_value_key) + digits + 1, sizeof(char));
        sprintf(number, int_value_key, ast->number);
        c = (char *)realloc(c, strlen(c) + strlen(number) + 1 * sizeof(char));
        strcat(c, number);
        free(number);
    }

    if (ast->left != NULL)
    {
        char *left_key = ",\"left\": ";
        c = realloc(c, strlen(c) + strlen(left_key) + 1 * sizeof(char));
        strcat(c, left_key);
        AST *left = ast->left;
        char *temp = ast_to_json(left);
        if (temp)
        {
            c = realloc(c, strlen(c) + strlen(temp) + 1 * sizeof(char));
            strcat(c, temp);
            free(temp);
        }
    }
    if (ast->right != NULL)
    {
        char *right_key = ",\"right\": ";
        c = realloc(c, strlen(c) + strlen(right_key) + 1 * sizeof(char));
        strcat(c, right_key);
        AST *right = ast->right;
        char *temp = ast_to_json(right);
        if (temp)
        {
            c = realloc(c, strlen(c) + strlen(temp) + 1 * sizeof(char));
            strcat(c, temp);
            free(temp);
        }
    }
    if (ast->value != NULL)
    {
        char *value = ",\"value\": ";
        c = realloc(c, strlen(c) + strlen(value) + 1 * sizeof(char));
        strcat(c, value);
        char *temp = ast_to_json(ast->value);
        if (temp)
        {
            c = realloc(c, strlen(c) + strlen(temp) + 1 * sizeof(char));
            strcat(c, temp);
            free(temp);
        }
    }
    if (array_size(&ast->childs) != 0)
    {
        char *children = ",\"children\": [";
        c = realloc(c, strlen(c) + strlen(children) + 1 * sizeof(char));
        strcat(c, children);
        for (size_t i = 0; i < array_size(&ast->childs); i++)
        {
            AST *child = array_at(&ast->childs, i);
            if (child)
            {
                char *temp = ast_to_json(child);
                if (temp)
                {
                    c = realloc(c, strlen(c) + strlen(temp) + 1 * sizeof(char));
                    strcat(c, temp);
                    free(temp);
                }
                if (i != array_size(&ast->childs) - 1)
                {
                    c = realloc(c, strlen(c) + 2 * sizeof(char));
                    strcat(c, ",");
                }
            }
        }
        c = realloc(c, strlen(c) + 2 * sizeof(char));
        strcat(c, "]");
    }
    c = realloc(c, strlen(c) + 2 * sizeof(char));
    strcat(c, "}");
    return c;
}
void ast_print(AST *root)
{
    char *temp = ast_to_json(root);
    fprintf(stdout, "%s\n", temp);
    free(temp);
}
void ast_free(AST *ast)
{
    if (!ast)
        return;
    for (size_t i = 0; i < array_size(&ast->childs); i++)
    {
        AST *child = array_at(&ast->childs, i);
        ast_free(child);
    }
    if (array_size(&ast->childs))
        array_free(&ast->childs);
    if (ast->value)
        ast_free(ast->value);
    if (ast->left)
        ast_free(ast->left);
    if (ast->right)
        ast_free(ast->right);

    if (ast->name)
        free(ast->name);
    free(ast);
}
size_t ast_emit_jump(Compiler *compiler, OpCode instruction)
{
    chunk_push(compiler->function->chunk, instruction);
    chunk_push(compiler->function->chunk, 0xff);
    chunk_push(compiler->function->chunk, 0xff);
    return array_size(compiler->function->chunk) - 2;
}
void ast_emit_loop(Compiler *compiler, size_t loop_start)
{
    chunk_push(compiler->function->chunk, OP_LOOP);
    size_t offset = array_size(compiler->function->chunk) - loop_start + 2;
    if (offset > UINT16_MAX)
    {
        compiler->had_error = true;
        fprintf(stderr, "Loop body too large.\n");
    }
    chunk_push(compiler->function->chunk, (offset >> 8) & 0xff);
    chunk_push(compiler->function->chunk, offset & 0xff);
}
void ast_patch_jump(Compiler *compiler, size_t offset)
{
    size_t jmp = array_size(compiler->function->chunk);
    if (jmp > UINT16_MAX)
    {
        compiler->had_error = true;
        fprintf(stderr, "Too much code to jump over.\n");
    }
    array_at(compiler->function->chunk, offset) = (jmp >> 8) & 0xff;
    array_at(compiler->function->chunk, offset + 1) = jmp & 0xff;
}
void ast_patch_jump_with(Compiler *compiler, size_t offset, size_t with)
{
    if (with > UINT16_MAX)
    {
        compiler->had_error = true;
        fprintf(stderr, "Too much code to jump over.\n");
    }
    array_at(compiler->function->chunk, offset) = (with >> 8) & 0xff;
    array_at(compiler->function->chunk, offset + 1) = with & 0xff;
}
void ast_begin_scope(Compiler *compiler)
{
    compiler->scope_depth++;
}
void ast_end_scope(Compiler *compiler)
{
    compiler->scope_depth--;
    while (compiler->local_count > 0 && compiler->locals[compiler->local_count - 1].depth > compiler->scope_depth)
    {
        chunk_push(compiler->function->chunk, OP_POP);
        compiler->local_count--;
    }
}
void ast_resolve_breaks(Compiler *compiler)
{
    for (byte i = 0; i < compiler->break_stack_count; i++)
    {
        byte offset = compiler->break_stack[i];
        ast_patch_jump(compiler, offset);
    }
    compiler->break_stack_count = 0;
}
void ast_resolve_continues(Compiler *compiler, size_t loop_start)
{
    for (byte i = 0; i < compiler->continue_stack_count; i++)
    {
        byte offset = compiler->continue_stack[i];
        ast_patch_jump_with(compiler, offset, loop_start);
    }
    compiler->continue_stack_count = 0;
}
void ast_binary_to_byte(AST *binary, Compiler *compiler)
{
    if (binary->token.type != TOKEN_ASSIGNMENT &&
        binary->token.type != TOKEN_AND &&
        binary->token.type != TOKEN_OR)
    {
        ast_to_byte(binary->left, compiler);
        ast_to_byte(binary->right, compiler);
    }
    switch (binary->token.type)
    {
    case TOKEN_PLUS:
        chunk_push(compiler->function->chunk, OP_ADD);
        break;
    case TOKEN_MINUS:
        chunk_push(compiler->function->chunk, OP_SUBTRACT);
        break;
    case TOKEN_MUL:
        chunk_push(compiler->function->chunk, OP_MULTIPLY);
        break;
    case TOKEN_DIV:
        chunk_push(compiler->function->chunk, OP_DIVIDE);
        break;
    case TOKEN_MOD:
        chunk_push(compiler->function->chunk, OP_MOD);
        break;
    case TOKEN_EQUALS:
        chunk_push(compiler->function->chunk, OP_EQUAL);
        break;
    case TOKEN_ASSIGNMENT:
    {
        ast_to_byte(binary->right, compiler);
        int arg = -1;
        if (compiler->scope_depth > 0)
        {
            arg = compiler_resolve_local(compiler, binary->left->token);
        }
        if (arg != -1)
        {
            chunk_push(compiler->function->chunk, OP_SET_LOCAL);
            chunk_push(compiler->function->chunk, (byte)arg);
        }
        else if (arg == -1)
        {
            ObjString *interned = table_find_string(compiler->globals, binary->left->name);
            if (interned == NULL)
                AST_REF_ERROR(compiler, binary->left->token);
            Value str = OBJ_VAL(interned, binary->left->token.row, binary->left->token.col);
            chunk_push(compiler->function->chunk, OP_SET_GLOBAL);
            chunk_push(compiler->function->chunk, value_push(compiler->function->values, str));
        }
        break;
    }
    case TOKEN_NOT_EQUALS:
        chunk_push(compiler->function->chunk, OP_NOT_EQUAL);
        break;
    case TOKEN_LT:
        chunk_push(compiler->function->chunk, OP_LT);
        break;
    case TOKEN_LTE:
        chunk_push(compiler->function->chunk, OP_LTE);
        break;
    case TOKEN_GT:
        chunk_push(compiler->function->chunk, OP_GT);
        break;
    case TOKEN_GTE:
        chunk_push(compiler->function->chunk, OP_LTE);
        break;
    case TOKEN_AND:
    {
        ast_to_byte(binary->left, compiler);
        size_t offset = ast_emit_jump(compiler, OP_JMP_IF_FALSE);
        chunk_push(compiler->function->chunk, OP_POP);
        ast_to_byte(binary->right, compiler);
        ast_patch_jump(compiler, offset);
        break;
    }
    case TOKEN_BITWISE_AND:
        chunk_push(compiler->function->chunk, OP_BITWISE_AND);
        break;
    case TOKEN_OR:
    {
        ast_to_byte(binary->left, compiler);
        size_t else_offset = ast_emit_jump(compiler, OP_JMP_IF_FALSE);
        size_t end_offset = ast_emit_jump(compiler, OP_JMP);
        ast_patch_jump(compiler, else_offset);
        chunk_push(compiler->function->chunk, OP_POP);
        ast_to_byte(binary->right, compiler);
        ast_patch_jump(compiler, end_offset);
        break;
    }
    case TOKEN_BITWISE_OR:
        chunk_push(compiler->function->chunk, OP_BITWISE_OR);
        break;
    case TOKEN_LEFT_SHIFT:
        chunk_push(compiler->function->chunk, OP_LEFT_SHIFT);
        break;
    case TOKEN_RIGHT_SHIFT:
        chunk_push(compiler->function->chunk, OP_RIGHT_SHIFT);
        break;
    default:
        ast_print(binary);
        NOTREACHABLE;
    }
}
void ast_unary_to_byte(AST *ast, Compiler *compiler)
{
    ast_to_byte(ast->value, compiler);
    switch (ast->token.type)
    {
    case TOKEN_MINUS:
        chunk_push(compiler->function->chunk, OP_NEGATE);
        break;
    case TOKEN_NOT:
        chunk_push(compiler->function->chunk, OP_NOT);
        break;
    case TOKEN_BITWISE_NOT:
        chunk_push(compiler->function->chunk, OP_BITWISE_NOT);
        break;
    case TOKEN_DECREMENT:
    case TOKEN_INCREMENT:
    {
        chunk_push(compiler->function->chunk, OP_CONSTANT);
        chunk_push(compiler->function->chunk, value_push(compiler->function->values, NUMBER_VAL(1, ast->token.row, ast->token.col)));

        OpCode code = ast->token.type == TOKEN_INCREMENT ? OP_ADD : OP_SUBTRACT;
        chunk_push(compiler->function->chunk, code);

        int arg = -1;
        if (compiler->scope_depth > 0)
        {
            arg = compiler_resolve_local(compiler, ast->value->token);
            if (arg != -1)
            {
                chunk_push(compiler->function->chunk, OP_SET_LOCAL);
                chunk_push(compiler->function->chunk, (byte)arg);
            }
        }
        if (arg == -1)
        {
            ObjString *interned = table_find_string(compiler->globals, ast->value->name);
            Value str = OBJ_VAL(interned, ast->value->token.row, ast->value->token.col);
            chunk_push(compiler->function->chunk, OP_SET_GLOBAL);
            chunk_push(compiler->function->chunk, value_push(compiler->function->values, str));
        }
        break;
    }
    default:
        ast_print(ast);
        NOTREACHABLE;
    }
}
void ast_to_byte(AST *ast, Compiler *compiler)
{
    if (!ast)
        return;
    switch (ast->type)
    {
    case AST_BLOCK:
    case AST_COMPOUND:
    {
        if (ast->type == AST_BLOCK)
            ast_begin_scope(compiler);
        for (size_t i = 0; i < array_size(&ast->childs); i++)
        {
            AST *child = array_at(&ast->childs, i);
            ast_to_byte(child, compiler);
        }
        if (ast->type == AST_BLOCK)
        {
            ast_end_scope(compiler);
        }
        break;
    }
    case AST_NUMBER:
    {
        chunk_push(compiler->function->chunk, OP_CONSTANT);
        chunk_push(compiler->function->chunk, value_push(compiler->function->values, NUMBER_VAL(ast->number, ast->token.row, ast->token.col)));
        break;
    }
    case AST_STRING:
    {
        chunk_push(compiler->function->chunk, OP_CONSTANT);
        ObjString *interned = table_find_string(compiler->strings, ast->name);
        ObjString *obj = interned == NULL ? cstr_to_objstr(ast->name) : interned;
        Value str = OBJ_VAL(obj, ast->token.row, ast->token.col);
        chunk_push(compiler->function->chunk, value_push(compiler->function->values, str));
        table_set(compiler->strings, AS_STRING(str), NULL_VAL(str.row, str.col));
        break;
    }
    case AST_BINARY:
    {
        ast_binary_to_byte(ast, compiler);
        break;
    }
    case AST_UNARY:
    {
        ast_unary_to_byte(ast, compiler);
        break;
    }
    case AST_SEQUENCEEXPR:
    {
        for (size_t i = 0; i < array_size(&ast->childs); i++)
        {
            ast_to_byte(array_at(&ast->childs, i), compiler);
            if (i < array_size(&ast->childs) - 1)
                chunk_push(compiler->function->chunk, OP_POP);
        }
        break;
    }
    case AST_TRUE:
    {
        chunk_push(compiler->function->chunk, OP_CONSTANT);
        chunk_push(compiler->function->chunk, value_push(compiler->function->values, BOOL_VAL(true, ast->token.row, ast->token.col)));
        break;
    }
    case AST_FALSE:
    {
        chunk_push(compiler->function->chunk, OP_CONSTANT);
        chunk_push(compiler->function->chunk, value_push(compiler->function->values, BOOL_VAL(false, ast->token.row, ast->token.col)));
        break;
    }
    case AST_NULL:
    {
        chunk_push(compiler->function->chunk, OP_CONSTANT);
        chunk_push(compiler->function->chunk, value_push(compiler->function->values, NULL_VAL(ast->token.row, ast->token.col)));
        break;
    }
    case AST_STMT:
    {
        switch (ast->token.type)
        {
        case TOKEN_PRINT:
        {
            ast_to_byte(ast->value, compiler);
            chunk_push(compiler->function->chunk, OP_PRINT);
            break;
        }
        case TOKEN_BREAK:
        {
            size_t offset = ast_emit_jump(compiler, OP_JMP);
            compiler->break_stack[compiler->break_stack_count++] = (byte)offset;
            break;
        }
        case TOKEN_CONTINUE:
        {
            size_t offset = ast_emit_jump(compiler, OP_JMP);
            compiler->continue_stack[compiler->continue_stack_count++] = (byte)offset;
            break;
        }
        case TOKEN_RETURN:
        {
            if (compiler->type == TYPE_SCRIPT)
            {
                compiler->had_error = true;
                fprintf(stderr, AST_ERROR_PREFIX_FORMART " can't have return from top-level code.\n",
                        compiler->file_path,
                        ast->token.row,
                        ast->token.col);
            }
            ast_to_byte(ast->value, compiler);
            chunk_push(compiler->function->chunk, OP_RETURN);
            break;
        }
        default:
            NOTREACHABLE;
        }
        break;
    }
    case AST_VAR:
    {
        if (compiler->scope_depth > 0)
        {
            for (int i = compiler->local_count - 1; i >= 0; i--)
            {
                Local *local = &compiler->locals[i];
                if (local->depth != -1 && local->depth < compiler->scope_depth)
                    break;
                if (token_equal(ast->token, local->name))
                {
                    AST_REDEC_ERROR(compiler, ast->token, local->name.row, local->name.col);
                    return;
                }
            }
            Local *local = &compiler->locals[compiler->local_count++];
            local->name = ast->token;
            local->depth = -1;
            if (ast->value)
                ast_to_byte(ast->value, compiler);
            compiler->locals[compiler->local_count - 1].depth = compiler->scope_depth;
            if (ast->value)
            {
                int arg = compiler_resolve_local(compiler, ast->token);
                if (arg == -1)
                {
                    compiler->had_error = true;
                    // arg shouldn't be -1 in this case because in the above we setting it up
                    // if this conddtion is true something is over cooked
                    fprintf(stderr, BOLDRED "NOT REACHABLE" RESET ": %s:%d in %s\n", __FILE__, __LINE__, __func__);
                }
                chunk_push(compiler->function->chunk, OP_SET_LOCAL);
                chunk_push(compiler->function->chunk, (byte)arg);

                ObjString *interned = table_find_string(compiler->variables, ast->name);
                ObjString *obj = interned ? interned : cstr_to_objstr(ast->name);
                Value str = OBJ_VAL(obj, ast->token.row, ast->token.col);
                table_set(compiler->variables, obj, str);
            }
            return;
        }
        ObjString *interned = table_find_string(compiler->globals, ast->name);
        if (interned)
        {
            Value previous;
            table_get(compiler->globals, interned, &previous);
            AST_REDEC_ERROR(compiler, ast->token, previous.row, previous.col);
        }
        ObjString *obj = interned ? interned : cstr_to_objstr(ast->name);
        Value str = OBJ_VAL(obj, ast->token.row, ast->token.col);
        table_set(compiler->globals, obj, UNDEF_VAL(str.row, str.row));
        ast_to_byte(ast->value, compiler);
        chunk_push(compiler->function->chunk, OP_DEFINE_GLOBAL);
        chunk_push(compiler->function->chunk, value_push(compiler->function->values, str));
        table_set(compiler->globals, obj, NULL_VAL(str.row, str.row));
        break;
    }
    case AST_ID:
    {
        int arg = -1;
        if (compiler->scope_depth > 0)
        {
            arg = compiler_resolve_local(compiler, ast->token);
            if (arg == -2)
            {
                AST_VAR_SELF_INIT(compiler, ast->token);
            }
            else if (arg != -1)
            {
                chunk_push(compiler->function->chunk, OP_GET_LOCAL);
                chunk_push(compiler->function->chunk, (byte)arg);
                ObjString *interend = table_find_string(compiler->variables, ast->name);
                ObjString *obj = interend ? interend : cstr_to_objstr(ast->name);
                Value str = OBJ_VAL(obj, ast->token.row, ast->token.col);
                chunk_push(compiler->function->chunk, value_push(compiler->function->values, str));
            }
        }
        if (arg == -1)
        {
            ObjString *interned = table_find_string(compiler->globals, ast->name);
            if (interned == NULL)
                AST_REF_ERROR(compiler, ast->token);
            if (interned)
            {
                Value init;
                table_get(compiler->globals, interned, &init);
                if (IS_UNDEF(init))
                    AST_VAR_SELF_INIT(compiler, ast->token);
            }
            Value str = OBJ_VAL(interned, ast->token.row, ast->token.col);
            chunk_push(compiler->function->chunk, OP_GET_GLOBAL);
            chunk_push(compiler->function->chunk, value_push(compiler->function->values, str));
        }
        break;
    }
    case AST_TERNARY:
    case AST_IF:
    {
        ast_to_byte(ast->value, compiler);
        size_t then_offset = ast_emit_jump(compiler, OP_JMP_IF_FALSE);
        chunk_push(compiler->function->chunk, OP_POP);
        ast_to_byte(ast->left, compiler);
        ast_patch_jump(compiler, then_offset);

        if (ast->right)
        {
            size_t else_offset = ast_emit_jump(compiler, OP_JMP);
            ast_patch_jump(compiler, then_offset);
            chunk_push(compiler->function->chunk, OP_POP);
            ast_to_byte(ast->right, compiler);
            ast_patch_jump(compiler, else_offset);
        }
        break;
    }
    case AST_WHILE:
    {
        size_t loop_start = array_size(compiler->function->chunk);
        ast_to_byte(ast->value, compiler);

        size_t exit_jmp = ast_emit_jump(compiler, OP_JMP_IF_FALSE);
        chunk_push(compiler->function->chunk, OP_POP);
        ast_to_byte(ast->left, compiler);
        ast_emit_loop(compiler, loop_start);
        ast_patch_jump(compiler, exit_jmp);
        if (compiler->break_stack_count == 0)
        {
            chunk_push(compiler->function->chunk, OP_POP);
        }
        ast_resolve_breaks(compiler);
        ast_resolve_continues(compiler, loop_start);
        break;
    }
    case AST_POSTFIX:
    {
        ast_to_byte(ast->value, compiler);
        chunk_push(compiler->function->chunk, OP_DUP);
        chunk_push(compiler->function->chunk, OP_CONSTANT);
        chunk_push(compiler->function->chunk, value_push(compiler->function->values, NUMBER_VAL(1, ast->token.row, ast->token.col)));

        OpCode code = ast->token.type == TOKEN_INCREMENT ? OP_ADD : OP_SUBTRACT;
        chunk_push(compiler->function->chunk, code);

        int arg = -1;
        if (compiler->scope_depth > 0)
        {
            arg = compiler_resolve_local(compiler, ast->value->token);
            if (arg != -1)
            {
                chunk_push(compiler->function->chunk, OP_SET_LOCAL);
                chunk_push(compiler->function->chunk, (byte)arg);
            }
        }
        if (arg == -1)
        {
            ObjString *interned = table_find_string(compiler->globals, ast->value->name);
            if (interned == NULL)
                AST_REF_ERROR(compiler, ast->value->token);
            Value str = OBJ_VAL(interned, ast->value->token.row, ast->value->token.col);
            chunk_push(compiler->function->chunk, OP_SET_GLOBAL);
            chunk_push(compiler->function->chunk, value_push(compiler->function->values, str));
        }

        chunk_push(compiler->function->chunk, OP_POP);
        break;
    }
    case AST_FOR_LOOP:
    {
        ast_begin_scope(compiler);

        AST *initializer = array_at(&ast->childs, 0);
        ast_to_byte(initializer, compiler);

        size_t loop_start = array_size(compiler->function->chunk);

        AST *condition = array_at(&ast->childs, 1);
        size_t end_offset = 0;
        if (condition)
        {
            ast_to_byte(condition, compiler);
            end_offset = ast_emit_jump(compiler, OP_JMP_IF_FALSE);
            chunk_push(compiler->function->chunk, OP_POP);
        }

        ast_to_byte(ast->value, compiler);

        AST *end_expr = array_at(&ast->childs, 2);
        ast_to_byte(end_expr, compiler);

        ast_emit_loop(compiler, loop_start);
        if (condition)
            ast_patch_jump(compiler, end_offset);

        ast_resolve_breaks(compiler);
        ast_resolve_continues(compiler, loop_start);

        ast_end_scope(compiler);
        break;
    }
    case AST_FUNCTION_DECL:
    {
        char *name = token_text(ast->token);
        ObjString *interned = table_find_string(compiler->globals, name);

        if (interned)
        {
            Value previous;
            table_get(compiler->globals, interned, &previous);
            if (IS_NATIVE(previous))
            {
                table_remove(compiler->globals, interned);
            }
            else
            {
                AST_REDEC_ERROR(compiler, ast->token, previous.row, previous.col);
            }
        }

        ObjString *function_name = interned ? interned : cstr_to_objstr(name);
        free(name);

        Compiler tempCompiler;
        init_compiler(&tempCompiler, TYPE_FUNCTION);
        Local *local = &tempCompiler.locals[0];
        local->name = ast->token;

        tempCompiler.file_path = compiler->file_path;
        tempCompiler.globals = compiler->globals;
        tempCompiler.strings = compiler->strings;
        tempCompiler.variables = compiler->variables;

        tempCompiler.function->name = function_name;
        tempCompiler.function->arity = (int)ast->childs.count;

        Value function_val = OBJ_VAL(tempCompiler.function, ast->token.row, ast->token.col);
        table_set(compiler->globals, function_name, function_val);

        ast_begin_scope(&tempCompiler);
        local->depth = tempCompiler.scope_depth;

        if (ast->childs.count > 255)
        {
            compiler->had_error = true;
            fprintf(stderr, AST_ERROR_PREFIX_FORMART " Can't have more than 255 parameters.\n", compiler->file_path, ast->token.row, ast->token.col);
        }
        else
        {
            for (size_t i = 0; i < ast->childs.count; i++)
            {
                AST *parameter = array_at(&ast->childs, i);
                ast_to_byte(parameter, &tempCompiler);
            }
        }
        bool has_return = false;
        if (ast->value)
            for (size_t i = 0; i < ast->value->childs.count; i++)
            {
                AST *child = array_at(&ast->value->childs, i);
                if (child->type == AST_STMT && child->token.type == TOKEN_RETURN)
                    has_return = true;
                ast_to_byte(child, &tempCompiler);
            }
        if (!has_return)
        {
            chunk_push(tempCompiler.function->chunk, OP_CONSTANT);
            chunk_push(tempCompiler.function->chunk,
                       value_push(tempCompiler.function->values,
                                  NULL_VAL(ast->token.row, ast->token.col)));
            compiler_end(&tempCompiler);
        }
        if (compiler->had_error == false)
            compiler->had_error = tempCompiler.had_error;

        break;
    }
    case AST_FUNCTION_CALL:
    {
        ObjString *function = table_find_string(compiler->globals, ast->left->name);
        if (function == NULL)
        {
            AST_REF_ERROR(compiler, ast->left->token);
            return;
        }
        Value value = {0};
        table_get(compiler->globals, function, &value);
        int arg_count = 0;
        if (ast->value == NULL)
        {
            arg_count = 0;
        }
        else if (ast->value->type == AST_SEQUENCEEXPR)
        {
            arg_count = ast->value->childs.count;
        }
        else
        {
            arg_count = 1;
        }
        if (IS_FUNCTION(value) || IS_NATIVE(value))
        {

            int arity = 0;
            if(IS_FUNCTION(value))
            {
                ObjFunction *function_obj = AS_FUNCTION(value);
                arity = function_obj->arity;
            }else{
                ObjNative *native = AS_NATIVE(value);
                arity = native->arity;
            }

            if (arity != arg_count)
            {
                compiler->had_error = true;
                fprintf(stderr, AST_ERROR_PREFIX_FORMART " Function '%.*s' expected %d arguments but got %d arguments\n",
                        compiler->file_path,
                        ast->left->token.row,
                        ast->left->token.col,
                        (int)ast->left->token.length,
                        ast->left->token.start,
                        arity,
                        arg_count);
            }
            if (arg_count > 255)
            {
                compiler->had_error = true;
                fprintf(stderr, AST_ERROR_PREFIX_FORMART " Function '%.*s' can't have more then 255 arguments\n",
                        compiler->file_path,
                        ast->left->token.row,
                        ast->left->token.col,
                        (int)ast->left->token.length,
                        ast->left->token.start
                        );
            }
        }
        Value function_val = OBJ_VAL(function, ast->left->token.row, ast->left->token.col);
        chunk_push(compiler->function->chunk, OP_GET_GLOBAL);
        chunk_push(compiler->function->chunk, value_push(compiler->function->values, function_val));

        if (arg_count == 1)
        {
            ast_to_byte(ast->value, compiler);
        }
        else if (arg_count > 1)
        {
            for (int i = 0; i < arg_count; i++)
            {
                AST *arg = array_at(&ast->value->childs, i);
                ast_to_byte(arg, compiler);
            }
        }
        chunk_push(compiler->function->chunk, OP_CALL);
        chunk_push(compiler->function->chunk, (byte)arg_count);
        break;
    }
    default:
        ast_print(ast);
        NOTREACHABLE;
    }
}