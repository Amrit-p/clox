#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define parser_error_prefix()                                            \
    do                                                                   \
    {                                                                    \
        fprintf(stderr, "ParserError at %s:", parser->lexer->file_path); \
    } while (0)

#define parser_token_location(token)                           \
    do                                                         \
    {                                                          \
        fprintf(stderr, "%zu:%zu ", (token).row, (token).col); \
    } while (0)
#define parser_token_error(token, message) \
    do                                     \
    {                                      \
        parser->had_error = true;          \
        parser_error_prefix();             \
        parser_token_location(token);      \
        fprintf(stderr, "%s\n", message);  \
    } while (0)

char *parser_unexpected_token(Token token, char *message)
{
    char *template = "unexpected '%s'";
    size_t message_len = 0;
    if (message)
    {
        message_len = strlen(message) + 2;
    }
    char *token_value = token_text(token);
    char *buffer = calloc(strlen(template) + strlen(token_value) + message_len + 1, sizeof(char));
    sprintf(buffer, template, token_value);
    if (message)
    {
        strcat(buffer, ", ");
        strcat(buffer, message);
    }

    free(token_value);
    return buffer;
}

static ParseRule rules[] = {
    [TOKEN_LPAREN] = {parser_parse_group, parser_parse_call, PREC_POSTIFX},
    [TOKEN_RPAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {parser_parse_prefix, parser_parse_infix, PREC_TERM},
    [TOKEN_PLUS] = {NULL, parser_parse_infix, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_DIV] = {NULL, parser_parse_infix, PREC_FACTOR},
    [TOKEN_MUL] = {NULL, parser_parse_infix, PREC_FACTOR},
    [TOKEN_NUMBER] = {parser_parse_number, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, parser_parse_comma, PREC_COMMA},
    [TOKEN_QUESTION] = {NULL, parser_parse_ternary, PREC_TERNARY},
    [TOKEN_TRUE] = {parser_parse_primary, NULL, PREC_NONE},
    [TOKEN_FALSE] = {parser_parse_primary, NULL, PREC_NONE},
    [TOKEN_NULL] = {parser_parse_primary, NULL, PREC_NONE},
    [TOKEN_ID] = {parser_parse_primary, NULL, PREC_NONE},
    [TOKEN_STRING] = {parser_parse_string, NULL, PREC_NONE},
    [TOKEN_EQUALS] = {NULL, parser_parse_infix, PREC_EQUALITY},
    [TOKEN_ASSIGNMENT] = {NULL, parser_parse_infix, PREC_ASSIGNMENT},
    [TOKEN_NOT] = {parser_parse_prefix, NULL, PREC_UNARY},
    [TOKEN_NOT_EQUALS] = {NULL, parser_parse_infix, PREC_EQUALITY},
    [TOKEN_MOD] = {NULL, parser_parse_infix, PREC_FACTOR},
    [TOKEN_RIGHT_SHIFT] = {NULL, parser_parse_infix, PREC_SHIFT},
    [TOKEN_LEFT_SHIFT] = {NULL, parser_parse_infix, PREC_SHIFT},
    [TOKEN_AND] = {NULL, parser_parse_infix, PREC_LOGICAL_AND},
    [TOKEN_BITWISE_AND] = {NULL, parser_parse_infix, PREC_BITWISE_AND},
    [TOKEN_OR] = {NULL, parser_parse_infix, PREC_LOGICAL_OR},
    [TOKEN_BITWISE_OR] = {NULL, parser_parse_infix, PREC_BITWISE_OR},
    [TOKEN_LTE] = {NULL, parser_parse_infix, PREC_COMPARISON},
    [TOKEN_LT] = {NULL, parser_parse_infix, PREC_COMPARISON},
    [TOKEN_GTE] = {NULL, parser_parse_infix, PREC_COMPARISON},
    [TOKEN_GT] = {NULL, parser_parse_infix, PREC_COMPARISON},
    [TOKEN_BITWISE_NOT] = {parser_parse_prefix, NULL, PREC_UNARY},
    [TOKEN_INCREMENT] = {parser_parse_prefix, parser_parse_postfix, PREC_UNARY},
    [TOKEN_DECREMENT] = {parser_parse_prefix, parser_parse_postfix, PREC_UNARY},
    [TOKEN_LCURLY] = {NULL, NULL, PREC_NONE},
    [TOKEN_RCURLY] = {NULL, NULL, PREC_NONE},
};
Parser *init_parser(Lexer *lexer)
{
    Parser *parser = calloc(1, sizeof(Parser));
    parser->current_token = lexer_next_token(lexer);
    parser->had_error = false;
    parser->panic_mode = false;
    parser->lexer = lexer;
    parser->parsing_call = false;
    return parser;
}
char *parser_prec_to_str(Precedence prec)
{
    switch (prec)
    {
    case PREC_NONE:
        return "PREC_NONE";
    case PREC_ASSIGNMENT:
        return "PREC_ASSIGNMENT";
    case PREC_BITWISE_AND:
        return "PREC_BITWISE_AND";
    case PREC_BITWISE_OR:
        return "PREC_BITWISE_XOR";
    case PREC_POSTIFX:
        return "PREC_POSTIFX";
    case PREC_UNARY:
        return "PREC_UNARY";
    case PREC_TERNARY:
        return "PREC_TERNNARY";
    case PREC_COMMA:
        return "PREC_COMMA";
    case PREC_COMPARISON:
        return "PREC_COMPARISON";
    case PREC_EQUALITY:
        return "PREC_EQUALITY";
    case PREC_FACTOR:
        return "PREC_FACTOR";
    case PREC_TERM:
        return "PREC_TERM";
    case PREC_LOGICAL_AND:
        return "PREC_LOGICAL_AND";
    case PREC_LOGICAL_OR:
        return "PREC_LOGICAL_OR";
    case PREC_SHIFT:
        return "PREC_SHIFT";
    default:
        return "PREC_UNKNOWN";
    }
}
Token parser_advance(Parser *parser)
{
    parser->current_token = lexer_next_token(parser->lexer);
    return parser->current_token;
}
Token parser_eat(Parser *parser, TokenType type, char *message)
{
    if ((type != parser->current_token.type || type == TOKEN_ERROR) && parser->panic_mode == 0)
    {
        parser_error(parser, parser_unexpected_token(parser->current_token, message));
        return parser->current_token;
    }
    else
    {
        parser->panic_mode = 0;
        Token token = parser->current_token;
        parser_advance(parser);
        return token;
    }
}
ParseRule *parser_production(TokenType type)
{
    return &rules[type];
}
void parser_current_token(Parser *parser)
{
    printf("current token %s\n", token_to_str(parser->current_token));
}
void parser_error(Parser *parser, char *message)
{
    if (parser->panic_mode)
        return;

    char *_message = message;
    if (parser->current_token.message)
    {
        _message = parser->current_token.message;
    }
    parser->had_error = true;
    parser->panic_mode = true;

    parser_error_prefix();
    parser_token_location(parser->current_token);
    fprintf(stderr, "%s\n", _message);
}
AST *parser_parse_string(Parser *parser)
{
    AST *string = init_ast(AST_STRING);
    string->token = parser->current_token;
    string->name = token_text(parser->current_token);
    parser_advance(parser);
    return string;
}
AST *parser_parse_id(Parser *parser)
{
    AST *id = init_ast(AST_ID);
    id->name = token_text(parser->current_token);
    Token token = parser_eat(parser, TOKEN_ID, "expected an identifier");
    id->token = token;
    return id;
}
AST *parser_parse_primary(Parser *parser)
{
    switch (parser->current_token.type)
    {
    case TOKEN_TRUE:
    {

        AST *ast_true = init_ast(AST_TRUE);
        ast_true->token = parser->current_token;
        parser_advance(parser);
        return ast_true;
    }
    case TOKEN_FALSE:
    {
        AST *ast_false = init_ast(AST_FALSE);
        ast_false->token = parser->current_token;
        parser_advance(parser);
        return ast_false;
    }
    case TOKEN_NULL:
    {

        AST *ast_null = init_ast(AST_NULL);
        ast_null->token = parser->current_token;
        parser_advance(parser);
        return ast_null;
    }
    default:
    {
        return parser_parse_id(parser);
    }
    }
}
AST *parser_parse_precedence(Parser *parser, Precedence precedence)
{
    ParsePrefixFn prefix_handler = parser_production(parser->current_token.type)->prefix;
    if (prefix_handler == NULL)
    {
        parser_error(parser, parser_unexpected_token(parser->current_token, "expected an expr."));
        if (parser->current_token.type == TOKEN_EOF)
            return NULL;
        parser_advance(parser);
        return NULL;
    }
    AST *prefix = prefix_handler(parser);
    while (precedence <= parser_production(parser->current_token.type)->precedence)
    {
        ParseInfixFn infix_handler = parser_production(parser->current_token.type)->infix;
        if (infix_handler == NULL)
        {
            printf("infix_handler is null for token %s\n", token_to_str(parser->current_token));
            return prefix;
        }
        else
        {
            prefix = infix_handler(parser, prefix);
        }
    }

    return prefix;
}
AST *parser_parse_number(Parser *parser)
{
    AST *number = init_ast(AST_NUMBER);
    number->token = parser->current_token;
    char buffer[parser->current_token.length + 1];
    sprintf(buffer, "%.*s", (int)parser->current_token.length, parser->current_token.start);
    parser_advance(parser);
    number->number = atof(buffer);
    return number;
}

AST *parser_parse_group(Parser *parser)
{
    parser_eat(parser, TOKEN_LPAREN, "expected '('.");
    if (parser->current_token.type == TOKEN_RPAREN && (parser->parsing_call == 1))
    {
        parser_advance(parser);
        return NULL;
    }
    AST *group = parser_parse_expr(parser);
    if (group)
        parser_eat(parser, TOKEN_RPAREN, "expected ',' or ')' after expression.");
    return group;
}
AST *parser_parse_call(Parser *parser, AST *callee)
{
    if (callee->type != AST_ID)
    {
        parser_token_error(callee->token, parser_unexpected_token(callee->token, "callee should be a identifier."));
    }
    AST *call = init_ast(AST_FUNCTION_CALL);
    call->left = callee;
    parser->parsing_call = 1;
    call->value = parser_parse_group(parser);
    parser->parsing_call = 0;
    return call;
}
AST *parser_parse_prefix(Parser *parser)
{
    AST *unary = init_ast(AST_UNARY);
    unary->token = parser->current_token;
    unary->name = token_text(parser->current_token);
    Token token = parser->current_token;
    parser_advance(parser);
    AST *operand = parser_parse_precedence(parser, PREC_UNARY);
    if ((token.type == TOKEN_INCREMENT || token.type == TOKEN_DECREMENT) && operand && operand->type != AST_ID)
    {
        parser_token_error(operand->token, "invalid expr in prefix operation.");
    }
    if (operand == NULL)
        return NULL;
    unary->value = operand;
    return unary;
}
AST *parser_parse_comma(Parser *parser, AST *prefix)
{
    AST *exprs = init_ast(AST_SEQUENCEEXPR);
    ast_push(exprs, prefix);
    while (parser->current_token.type == TOKEN_COMMA)
    {
        parser_advance(parser);
        AST *child = parser_parse_expr(parser);
        if (child == NULL)
        {
            parser->panic_mode = 1;
            return NULL;
        }
        ast_push(exprs, child);
    }
    return exprs;
}
AST *parser_parse_infix(Parser *parser, AST *prefix)
{
    AST *bin = init_ast(AST_BINARY);
    bin->token = parser->current_token;
    Token token = parser->current_token;
    parser_advance(parser);
    if (token.type == TOKEN_ASSIGNMENT && prefix && prefix->type != AST_ID && prefix->token.type != TOKEN_ASSIGNMENT)
    {
        parser_token_error(prefix->token, "lvalue cannot be a constant.");
    }

    bin->name = token_text(token);
    ParseRule *rule = parser_production(token.type);
    Precedence precedence = token.type == TOKEN_ASSIGNMENT ? PREC_ASSIGNMENT : rule->precedence + 1;
    AST *right = parser_parse_precedence(parser, precedence);

    if (right == NULL)
        return NULL;
    bin->left = prefix;
    bin->right = right;

    return bin;
}
AST *parser_parse_ternary(Parser *parser, AST *condition)
{
    AST *ternary = init_ast(AST_TERNARY);
    ternary->token = parser->current_token;
    ternary->name = token_text(parser->current_token);
    ternary->value = condition;
    parser_advance(parser);
    AST *then = parser_parse_expr(parser);
    if (parser->current_token.type != TOKEN_COLON)
    {
        parser_error(parser, "expect ':' after expr.");
        return NULL;
    }
    then->token = parser->current_token;
    parser_advance(parser);
    AST *_else = parser_parse_expr(parser);
    ternary->left = then;
    ternary->right = _else;
    return ternary;
}
AST *parser_parse_postfix(Parser *parser, AST *oprand)
{
    if (oprand && oprand->type != AST_VAR && oprand->type != AST_ID)
    {
        char *template = "Operand must be a variable.But given is \"%.*s\"";
        char *str = calloc(strlen(template) + oprand->token.length + 1, sizeof(char));
        sprintf(str, template, (int)oprand->token.length, oprand->token.start);
        parser_token_error(oprand->token, str);
        free(str);
    }
    AST *postfix = init_ast(AST_POSTFIX);
    postfix->token = parser->current_token;
    postfix->name = token_text(parser->current_token);
    postfix->value = oprand;
    parser_eat(parser, parser->current_token.type, "expected posfix something");
    return postfix;
}
void parser_state(Parser *parser)
{
    parser_current_token(parser);
    printf("had_error %d\npanic_mode %d\n", parser->had_error, parser->panic_mode);
}
AST *parser_parse_expr(Parser *parser)
{
    return parser_parse_precedence(parser, PREC_COMMA);
}
AST *parser_parse_print(Parser *parser)
{
    AST *print = init_ast(AST_STMT);
    print->token = parser->current_token;
    print->name = token_text(parser->current_token);
    parser_advance(parser);
    print->value = parser_parse_expr(parser);
    if (print->value == NULL)
        return NULL;
    return print;
}
AST *parser_parse_var(Parser *parser)
{
    AST *var = init_ast(AST_VAR);
    parser_advance(parser);
    Token identifier = parser_eat(parser, TOKEN_ID, "expected identifier name.");
    var->name = token_text(identifier);
    var->token = identifier;
    if (parser->current_token.type == TOKEN_ASSIGNMENT)
    {
        parser_advance(parser);
        var->value = parser_parse_expr(parser);
    }
    else
    {
        var->value = init_ast(AST_NULL);
        var->value->token = var->token;
    }
    if (parser->had_error)
        return NULL;
    return var;
}

AST *parser_parse_if(Parser *parser)
{
    parser_advance(parser);
    AST *_if = init_ast(AST_IF);
    _if->value = parser_parse_group(parser);

    if (parser->current_token.type == TOKEN_SEMICOLON)
        parser_advance(parser);
    else
    {
        _if->left = parser_parse_stmt(parser);
        if (parser->current_token.type == TOKEN_ELSE)
        {
            parser_advance(parser);
            _if->right = parser_parse_decl(parser);
        }
    }

    return _if;
}
AST *parser_parse_while(Parser *parser)
{
    parser->is_looping++;
    AST *loop = init_ast(AST_WHILE);
    loop->token = parser->current_token;
    parser_advance(parser);
    loop->value = parser_parse_group(parser);
    loop->left = parser_parse_stmt(parser);
    parser->is_looping--;
    return loop;
}
AST *parser_parse_for(Parser *parser)
{
    parser->is_looping = true;
    AST *loop = init_ast(AST_FOR_LOOP);
    loop->token = parser->current_token;
    parser_advance(parser);
    parser_eat(parser, TOKEN_LPAREN, "expected '(' after for.");
    AST *init = NULL;
    if (parser->current_token.type == TOKEN_VAR)
        init = parser_parse_var(parser);
    else if (parser->current_token.type != TOKEN_SEMICOLON)
        init = parser_parse_expr(parser);

    ast_push(loop, init);
    parser_eat(parser, TOKEN_SEMICOLON, "expected an ';' after initializer.");

    if (parser->current_token.type == TOKEN_SEMICOLON)
    {
        parser_advance(parser);
    }
    else
    {
        AST *condition = parser_parse_expr(parser);
        ast_push(loop, condition);
        parser_eat(parser, TOKEN_SEMICOLON, "expected an ';' after expression");
    }
    if (parser->current_token.type == TOKEN_SEMICOLON || parser->current_token.type == TOKEN_RPAREN)
    {
        parser_advance(parser);
    }
    else
    {
        AST *end_expr = parser_parse_expr(parser);
        ast_push(loop, end_expr);
        parser_eat(parser, TOKEN_RPAREN, "expected ')'");
    }
    loop->value = parser_parse_stmt(parser);
    parser->is_looping = false;
    return loop;
}
AST *parser_parse_block(Parser *parser)
{
    Token lcurly = parser_eat(parser, TOKEN_LCURLY, "expected '{'");
    if (parser->current_token.type == TOKEN_RCURLY)
    {
        parser_advance(parser);
        return NULL;
    }
    AST *block = init_ast(AST_BLOCK);
    block->token = lcurly;
    while (parser->current_token.type != TOKEN_EOF && parser->current_token.type != TOKEN_RCURLY)
    {
        AST *child = parser_parse_decl(parser);
        ast_push(block, child);
    }
    parser_eat(parser, TOKEN_RCURLY, "expected '}'");
    return block;
}
AST *parser_parse_return(Parser *parser)
{
    AST *ret = init_ast(AST_STMT);
    ret->token = parser_eat(parser, TOKEN_RETURN, "expeted 'return' keyword.");
    ret->value = parser_parse_expr(parser);
    if (ret->value == NULL)
    {
        ret->value = init_ast(AST_NULL);
        ret->value->token = ret->token;
    }
    parser_eat(parser, TOKEN_SEMICOLON, "expected semicolon after return statement");
    return ret;
}
AST *parser_parse_stmt(Parser *parser)
{
    TokenType token_type = parser->current_token.type;
    AST *stmt = NULL;
    switch (token_type)
    {
    case TOKEN_FOR:
        return parser_parse_for(parser);
    case TOKEN_WHILE:
        return parser_parse_while(parser);
    case TOKEN_IF:
        return parser_parse_if(parser);
    case TOKEN_LCURLY:
        return parser_parse_block(parser);
    case TOKEN_PRINT:
        stmt = parser_parse_print(parser);
        break;
    case TOKEN_ELSE:
    {
        parser_error(parser, "cannot have 'else' without 'if'.");
        parser_advance(parser);
        return NULL;
    }
    case TOKEN_RCURLY:
    {
        parser_error(parser, parser_unexpected_token(parser->current_token, 0));
        parser_advance(parser);
        return NULL;
    }
    case TOKEN_RETURN:
    {
        return parser_parse_return(parser);
    }
    case TOKEN_CONTINUE:
    case TOKEN_BREAK:
    {
        if (parser->is_looping == 0)
        {
            parser_error(parser, "'break' or 'continue' statement used outside of a loop.");
        }
        stmt = init_ast(AST_STMT);
        stmt->token = parser->current_token;
        parser_advance(parser);
        break;
    }
    default:
        stmt = parser_parse_expr(parser);
    }
    if (stmt)
        parser_eat(parser, TOKEN_SEMICOLON, "statement should be ended with semicolon.");
    return stmt;
}
AST *parser_parse_function(Parser *parser)
{
    parser_advance(parser);
    AST *function = init_ast(AST_FUNCTION_DECL);
    Token function_name = parser_eat(parser, TOKEN_ID, "expected function name.");
    function->token = function_name;
    parser_eat(parser, TOKEN_LPAREN, "expected '(' after function name.");
    while (parser->current_token.type != TOKEN_RPAREN)
    {
        AST *var = init_ast(AST_VAR);
        var->name = token_text(parser->current_token);
        Token token = parser_eat(parser, TOKEN_ID, "expected an identifier");
        var->token = token;
        if (parser->current_token.type == TOKEN_COMMA)
        {
            parser_advance(parser);
        }
        ast_push(function, var);
    }
    parser_eat(parser, TOKEN_RPAREN, "expected ')' after function agruments.");
    function->value = parser_parse_block(parser);
    return function;
}
AST *parser_parse_decl(Parser *parser)
{
    TokenType token_type = parser->current_token.type;
    switch (token_type)
    {
    case TOKEN_VAR:
    {
        AST *var = parser_parse_var(parser);
        parser_eat(parser, TOKEN_SEMICOLON, "variable declaration should be end with semicolon.");
        return var;
    }
    case TOKEN_FUNCTION:
        return parser_parse_function(parser);
    default:
        return parser_parse_stmt(parser);
    }
}
AST *parser_parse_compound(Parser *parser)
{
    AST *compound = init_ast(AST_COMPOUND);
    while (parser->current_token.type != TOKEN_EOF)
    {
        AST *child = parser_parse_decl(parser);
        if (child)
            ast_push(compound, child);
    }
    return compound;
}
AST *parser_parse(Parser *parser)
{
    return parser_parse_compound(parser);
}

void parser_free(Parser *parser)
{
    free(parser);
}