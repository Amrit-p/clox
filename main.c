#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chunk.h"
#include "VM.h"
#include "token.h"
#include "helper.h"
#include "parser.h"
#include "lexer.h"
#ifndef PROD
#define LOG_IMPLEMENTATION
#endif
#include "log.h"
#include "AST.h"
#include <errno.h>
#include "compiler.h"

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
void usage(char *argv[])
{
    fprintf(stderr, ERROR_PREFIX "%s <filename>\n", argv[0]);
}
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        usage(argv);
        return 1;
    }
    char *path = argv[1];
    log_info("reading %s", path);
    char *source = readFile(path);

    if (strlen(source) == 0)
    {
        free(source);
        return 0;
    }
    log_info("initializing lexer");
    Lexer *lexer = init_lexer(source, path);
    log_info("initializing parser");
    Parser *parser = init_parser(lexer);
    log_info("starting parsing...");
    AST *ast = parser_parse(parser);
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
            log_info("starting interpreting...");
            VM *vm = init_vm(&compiler);
            VM_Error error = vm_interpret(vm);
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
            default:
                break;
            }
            compiler_free(&compiler);
            vm_free(vm);
        }
        log_info("finished interpreting");
    }
    ast_free(ast);
    parser_free(parser);
    lexer_free(lexer);
}