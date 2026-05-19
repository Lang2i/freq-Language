#ifndef FREQ_LEXER_H
#define FREQ_LEXER_H

#include "common.h"

typedef struct {
    const char* source;
    int pos;
    int read_pos;
    char ch;
    int line;
    int column;
} Lexer;

Lexer* lexer_new(const char* source);
void lexer_free(Lexer* lexer);
Token lexer_next_token(Lexer* lexer);

#endif
