#ifndef FREQ_PARSER_H
#define FREQ_PARSER_H

#include "common.h"
#include "lexer.h"

typedef struct {
    Lexer* lexer;
    Token current_token;
    Token peek_token;
    int has_error;
} Parser;

Parser* parser_new(Lexer* lexer);
void parser_free(Parser* parser);
AST* parser_parse(Parser* parser);
void ast_free(AST* ast);

#endif
