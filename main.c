#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "helper.h"

#ifndef PROD
#define LOG_IMPLEMENTATION
#endif
#include "log.h"

#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "AST.h"
#include "compiler.h"
#include "VM.h"


#define ERROR_PREFIX "Error: "

static char *readFile(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        fprintf(stderr, ERROR_PREFIX "failed to open file \"%s\".\nMessage: %s.\n", path, strerror(errno));
        exit(1);
    }
    if (fseek(file, 0L, SEEK_END) < 0)
    {
        fprintf(stderr, ERROR_PREFIX "failed to seek in file \"%s\".\nMessage: %s.\n", path, strerror(errno));
        exit(1);
    }
    long fileSize = ftell(file);
    if (fileSize < 0)
    {
        fprintf(stderr, ERROR_PREFIX "failed get the size of file \"%s\".\nMessage: %s.\n", path, strerror(errno));
        exit(1);
    }
    rewind(file);

    char *buffer = calloc((size_t)fileSize + 1, sizeof(char));
    if (buffer == NULL)
    {
        fprintf(stderr, ERROR_PREFIX "failed to allocate memory for reading file \"%s\".\nMessage: %s.\n", path, strerror(errno));
        fclose(file);
        exit(1);
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < (size_t)fileSize)
    {
        fprintf(stderr, ERROR_PREFIX "failed to read file \"%s\".\nMessage: %s.\n", path, strerror(errno));
        fclose(file);
        free(buffer);
        exit(1);
    }
    buffer[bytesRead] = '\0';
    fclose(file);
    return buffer;
}
typedef enum
{
    OUTPUT_NONE,
    OUTPUT_IR,
    OUTPUT_AST,
    OUTPUT_TOKEN,
} OutputType;

OutputType parse_output_flag(char *arg)
{
    const char *prefix = "--out=";
    OutputType flag = OUTPUT_NONE;

    if (strncmp(arg, prefix, strlen(prefix)) != 0)
    {
        fprintf(stderr, "Error: invalid flag format should be --out=TOK|AST|IR: %s\n", arg);
        return flag;
    }

    // Extract the value part (after '=')
    char *value = arg + strlen(prefix);

    if (value == NULL)
    {
        fprintf(stderr, "Error: missing flag value --out=TOK|AST|IR: %s\n", arg);
        return flag;
    }
    char token[256]; // Buffer to store a mutable copy
    strncpy(token, value, sizeof(token) - 1);
    token[sizeof(token) - 1] = '\0';
    if (strcmp(token, "TOK") == 0)
    {
        return OUTPUT_TOKEN;
    }
    else if (strcmp(token, "AST") == 0)
    {
        return OUTPUT_AST;
    }
    else if (strcmp(token, "IR") == 0)
    {
        return OUTPUT_IR;
    }
    else
    {
        fprintf(stderr, "Error: flag %s  option \"%s\" not in this set {TOK,AST,IR} \n", prefix, token);
        return OUTPUT_NONE;
    }
}
void usage(char *argv[])
{
    fprintf(stderr, ERROR_PREFIX "%s [--out=TOK|AST|IR] <filename>\n", argv[0]);
}
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        usage(argv);
        return 1;
    }
    char *output_flag = NULL;
    char *source_file = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strncmp(argv[i], "--out=", 4) == 0)
            output_flag = argv[i];
        else
            source_file = argv[i];
    }
    OutputType type = OUTPUT_NONE;
    if (output_flag)
    {
        type = parse_output_flag(output_flag);
    }
    if (!source_file)
    {
        usage(argv);
        return 1;
    }
    char *path = source_file;
    log_info("reading %s", path);
    char *source = readFile(path);

    if (strlen(source) == 0)
    {
        free(source);
        return 0;
    }
    log_info("initializing lexer");
    Lexer *lexer = init_lexer(source, path);
    if (type == OUTPUT_TOKEN)
    {
        Token token = lexer_next_token(lexer);
        while (token.type != TOKEN_EOF)
        {
            token_print(token);
            token = lexer_next_token(lexer);
        }
        exit(1);
    }
    log_info("initializing parser");
    Parser *parser = init_parser(lexer);
    log_info("starting parsing...");
    AST *ast = parser_parse(parser);
    if (type == OUTPUT_AST)
    {
        ast_print(ast);
        exit(1);
    }
    if (parser->had_error)
    {
        log_error("parser had a syntax error.");
    }
    else if (parser->had_error == 0)
    {
        Compiler compiler = {0};
        init_compiler(&compiler, TYPE_SCRIPT);
        compiler.file_path = lexer->file_path;
        log_info("converting AST to bytecode");
        ast_to_byte(ast, &compiler);
        free(source);

        if (compiler.had_error == false)
        {
            compiler_end(&compiler);

            if (type == OUTPUT_IR)
            {
                char *bytecode_path = "bytecode.txt";
                FILE *bytefd = fopen(bytecode_path, "wb");
                if (bytefd == NULL)
                {
                    log_error("failed to open '%s', %s", bytecode_path, strerror(errno));
                }
                else
                {
                    compiler_dump(&compiler, bytefd);
                    fclose(bytefd);
                }
                exit(1);
            }
            log_info("starting interpreting...");
            VM *vm = init_vm(&compiler);
            VM_Error error = vm_interpret(vm);
            if (error != VM_OK)
            {
                for (int i = vm->frame_count - 1; i >= 0; --i)
                {
                    CallFrame *frame = &vm->frames[i];
                    ObjFunction *function = frame->function;
                    fprintf(stderr, "#%d ", i);
                    if (function->name == NULL)
                    {
                        fprintf(stderr, "%s\n", vm->file_path);
                    }
                    else
                    {
                        fprintf(stderr, "%s()\n", function->name->chars);
                    }
                }
            }
            switch (error)
            {
            case VM_TYPE_ERROR:
                fprintf(stderr, "TypeError at %s:%d:%d %s\n", vm->file_path, vm->row, vm->col, vm->message);
                break;
            case VM_STACK_OVERFLOW:
            case VM_STACK_UNDERFLOW:
                fprintf(stderr, "%s InternalError %s\n", vm->file_path, vm->message);
                break;
            case VM_REFERENCE_ERROR:
                fprintf(stderr, "ReferenceError at %s:%d:%d %s\n", vm->file_path, vm->row, vm->col, vm->message);
                break;
            case VM_TOO_FEW_ARGUMENTS:
                fprintf(stderr, "TooFewArguments at %s:%d:%d %s\n", vm->file_path, vm->row, vm->col, vm->message);
                break;
            case VM_TOO_MANY_ARGUMENTS:
                fprintf(stderr, "TooManyArguments at %s:%d:%d %s\n", vm->file_path, vm->row, vm->col, vm->message);
                break;
            default:
                break;
            }
            compiler_free(&compiler);
            vm_free(vm);
            objects_free();
        }
        log_info("finished interpreting");
    }
    ast_free(ast);
    parser_free(parser);
    lexer_free(lexer);
}