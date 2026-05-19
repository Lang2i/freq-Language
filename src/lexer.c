#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

static void lexer_read_char(Lexer* lexer) {
    if (lexer->read_pos >= strlen(lexer->source)) {
        lexer->ch = 0;
    } else {
        lexer->ch = lexer->source[lexer->read_pos];
    }
    lexer->pos = lexer->read_pos;
    lexer->read_pos++;
    if (lexer->ch == '\n') {
        lexer->line++;
        lexer->column = 0;
    } else {
        lexer->column++;
    }
}

static char lexer_peek_char(Lexer* lexer) {
    if (lexer->read_pos >= strlen(lexer->source)) {
        return 0;
    }
    return lexer->source[lexer->read_pos];
}

static void lexer_skip_whitespace(Lexer* lexer) {
    while (lexer->ch == ' ' || lexer->ch == '\t' || lexer->ch == '\r') {
        lexer_read_char(lexer);
    }
}

static void lexer_skip_comment(Lexer* lexer) {
    if (lexer->ch == '/' && lexer_peek_char(lexer) == '/') {
        while (lexer->ch != '\n' && lexer->ch != 0) {
            lexer_read_char(lexer);
        }
    }
}

static char* lexer_read_identifier(Lexer* lexer) {
    int pos = lexer->pos;
    while (isalnum(lexer->ch) || lexer->ch == '_') {
        lexer_read_char(lexer);
    }
    int len = lexer->pos - pos;
    char* ident = (char*)malloc(len + 1);
    strncpy(ident, lexer->source + pos, len);
    ident[len] = '\0';
    return ident;
}

static char* lexer_read_unquoted_string(Lexer* lexer) {
    int pos = lexer->pos;
    while (lexer->ch != '\n' && lexer->ch != '\r' && lexer->ch != '(' && 
           lexer->ch != ')' && lexer->ch != ',' && lexer->ch != '=' && 
           lexer->ch != ';' && lexer->ch != 0) {
        lexer_read_char(lexer);
    }
    int len = lexer->pos - pos;
    char* str = (char*)malloc(len + 1);
    strncpy(str, lexer->source + pos, len);
    str[len] = '\0';
    return str;
}

static char* lexer_read_number(Lexer* lexer, int* is_float) {
    int pos = lexer->pos;
    *is_float = 0;
    while (isdigit(lexer->ch)) {
        lexer_read_char(lexer);
    }
    if (lexer->ch == '.' && isdigit(lexer_peek_char(lexer))) {
        *is_float = 1;
        lexer_read_char(lexer);
        while (isdigit(lexer->ch)) {
            lexer_read_char(lexer);
        }
    }
    int len = lexer->pos - pos;
    char* num = (char*)malloc(len + 1);
    strncpy(num, lexer->source + pos, len);
    num[len] = '\0';
    return num;
}

static char* process_escape_sequences(const char* str) {
    int len = strlen(str);
    char* result = (char*)malloc(len + 1);
    int j = 0;
    for (int i = 0; i < len; i++) {
        if (str[i] == '\\' && i + 1 < len) {
            switch (str[i + 1]) {
                case 'n':
                    result[j++] = '\n';
                    i++;
                    break;
                case 't':
                    result[j++] = '\t';
                    i++;
                    break;
                case 'r':
                    result[j++] = '\r';
                    i++;
                    break;
                case '0':
                    result[j++] = '\0';
                    i++;
                    break;
                case '\\':
                    result[j++] = '\\';
                    i++;
                    break;
                case '"':
                    result[j++] = '"';
                    i++;
                    break;
                case '\'':
                    result[j++] = '\'';
                    i++;
                    break;
                default:
                    result[j++] = str[i];
                    break;
            }
        } else {
            result[j++] = str[i];
        }
    }
    result[j] = '\0';
    return result;
}

static char* lexer_read_quoted_string(Lexer* lexer) {
    lexer_read_char(lexer);
    int pos = lexer->pos;
    while (lexer->ch != '"' && lexer->ch != 0) {
        if (lexer->ch == '\\' && lexer_peek_char(lexer) == '"') {
            lexer_read_char(lexer);
        }
        lexer_read_char(lexer);
    }
    int len = lexer->pos - pos;
    char* str = (char*)malloc(len + 1);
    strncpy(str, lexer->source + pos, len);
    str[len] = '\0';
    if (lexer->ch == '"') {
        lexer_read_char(lexer);
    }
    
    char* processed = process_escape_sequences(str);
    free(str);
    return processed;
}

static char* lexer_read_char_literal(Lexer* lexer) {
    lexer_read_char(lexer);
    char* ch = (char*)malloc(2);
    ch[0] = lexer->ch;
    ch[1] = '\0';
    lexer_read_char(lexer);
    if (lexer->ch == '\'') {
        lexer_read_char(lexer);
    }
    return ch;
}

static TokenType lexer_lookup_keyword(const char* ident) {
    if (strcmp(ident, "var") == 0) return TOKEN_VAR;
    if (strcmp(ident, "let") == 0) return TOKEN_LET;
    if (strcmp(ident, "const") == 0) return TOKEN_CONST;
    if (strcmp(ident, "int") == 0) return TOKEN_INT;
    if (strcmp(ident, "int64") == 0) return TOKEN_INT64;
    if (strcmp(ident, "long") == 0) return TOKEN_LONG;
    if (strcmp(ident, "float") == 0) return TOKEN_FLOAT_TYPE;
    if (strcmp(ident, "double") == 0) return TOKEN_DOUBLE;
    if (strcmp(ident, "bool") == 0) return TOKEN_BOOL;
    if (strcmp(ident, "string") == 0) return TOKEN_STRING_TYPE;
    if (strcmp(ident, "char") == 0) return TOKEN_CHAR_TYPE;
    if (strcmp(ident, "print") == 0) return TOKEN_PRINT;
    if (strcmp(ident, "printn") == 0) return TOKEN_PRINTN;
    if (strcmp(ident, "printf") == 0) return TOKEN_PRINTF;
    if (strcmp(ident, "printfn") == 0) return TOKEN_PRINTFN;
    if (strcmp(ident, "if") == 0) return TOKEN_IF;
    if (strcmp(ident, "else") == 0) return TOKEN_ELSE;
    if (strcmp(ident, "elif") == 0) return TOKEN_ELIF;
    if (strcmp(ident, "and") == 0) return TOKEN_AND;
    if (strcmp(ident, "or") == 0) return TOKEN_OR;
    if (strcmp(ident, "true") == 0) return TOKEN_TRUE;
    if (strcmp(ident, "false") == 0) return TOKEN_FALSE;
    if (strcmp(ident, "switch") == 0) return TOKEN_SWITCH;
    if (strcmp(ident, "default") == 0) return TOKEN_DEFAULT;
    if (strcmp(ident, "while") == 0) return TOKEN_WHILE;
    if (strcmp(ident, "for") == 0) return TOKEN_FOR;
    if (strcmp(ident, "fo") == 0) return TOKEN_FO;
    if (strcmp(ident, "break") == 0) return TOKEN_BREAK;
    if (strcmp(ident, "continue") == 0) return TOKEN_CONTINUE;
    if (strcmp(ident, "quit") == 0) return TOKEN_QUIT;
    if (strcmp(ident, "exit") == 0) return TOKEN_EXIT;
    if (strcmp(ident, "func") == 0) return TOKEN_FUNC;
    if (strcmp(ident, "return") == 0) return TOKEN_RETURN;
    if (strcmp(ident, "struct") == 0) return TOKEN_STRUCT;
    if (strcmp(ident, "enum") == 0) return TOKEN_ENUM;
    if (strcmp(ident, "mod") == 0) return TOKEN_MOD;
    if (strcmp(ident, "import") == 0) return TOKEN_IMPORT;
    if (strcmp(ident, "use") == 0) return TOKEN_USE;
    if (strcmp(ident, "out") == 0) return TOKEN_OUT;
    return TOKEN_IDENTIFIER;
}

Lexer* lexer_new(const char* source) {
    Lexer* lexer = (Lexer*)malloc(sizeof(Lexer));
    lexer->source = source;
    lexer->pos = 0;
    lexer->read_pos = 0;
    lexer->line = 1;
    lexer->column = 0;
    lexer_read_char(lexer);
    return lexer;
}

void lexer_free(Lexer* lexer) {
    free(lexer);
}

Token lexer_next_token(Lexer* lexer) {
    Token tok;
    tok.line = lexer->line;
    tok.column = lexer->column;
    
    lexer_skip_whitespace(lexer);
    lexer_skip_comment(lexer);
    lexer_skip_whitespace(lexer);
    
    switch (lexer->ch) {
        case '\n':
            tok.type = TOKEN_NEWLINE;
            tok.value = NULL;
            lexer_read_char(lexer);
            break;
        case ',':
            tok.type = TOKEN_COMMA;
            tok.value = NULL;
            lexer_read_char(lexer);
            break;
        case '=':
            lexer_read_char(lexer);
            if (lexer->ch == '=') {
                tok.type = TOKEN_EQUAL_EQUAL;
                tok.value = NULL;
                lexer_read_char(lexer);
            } else {
                tok.type = TOKEN_EQUALS;
                tok.value = NULL;
            }
            break;
        case '>':
            lexer_read_char(lexer);
            if (lexer->ch == '=') {
                tok.type = TOKEN_GREATER_EQUAL;
                tok.value = NULL;
                lexer_read_char(lexer);
            } else {
                tok.type = TOKEN_GREATER;
                tok.value = NULL;
            }
            break;
        case '<':
            lexer_read_char(lexer);
            if (lexer->ch == '=') {
                tok.type = TOKEN_LESS_EQUAL;
                tok.value = NULL;
                lexer_read_char(lexer);
            } else {
                tok.type = TOKEN_LESS;
                tok.value = NULL;
            }
            break;
        case '!':
            lexer_read_char(lexer);
            if (lexer->ch == '=') {
                tok.type = TOKEN_NOT_EQUAL;
                tok.value = NULL;
                lexer_read_char(lexer);
            } else {
                tok.type = TOKEN_ILLEGAL;
                tok.value = NULL;
            }
            break;
        case '{':
            tok.type = TOKEN_LBRACE;
            tok.value = NULL;
            lexer_read_char(lexer);
            break;
        case '}':
            tok.type = TOKEN_RBRACE;
            tok.value = NULL;
            lexer_read_char(lexer);
            break;
        case '*':
            tok.type = TOKEN_STAR;
            tok.value = NULL;
            lexer_read_char(lexer);
            break;
        case '&':
            tok.type = TOKEN_AMPERSAND;
            tok.value = NULL;
            lexer_read_char(lexer);
            break;
        case '(':
            tok.type = TOKEN_LPAREN;
            tok.value = NULL;
            lexer_read_char(lexer);
            break;
        case ')':
            tok.type = TOKEN_RPAREN;
            tok.value = NULL;
            lexer_read_char(lexer);
            break;
        case '[':
            tok.type = TOKEN_LBRACKET;
            tok.value = NULL;
            lexer_read_char(lexer);
            break;
        case ']':
            tok.type = TOKEN_RBRACKET;
            tok.value = NULL;
            lexer_read_char(lexer);
            break;
        case ';':
            tok.type = TOKEN_SEMICOLON;
            tok.value = NULL;
            lexer_read_char(lexer);
            break;
        case ':':
            lexer_read_char(lexer);
            if (lexer->ch == ':') {
                tok.type = TOKEN_DOUBLE_COLON;
                tok.value = NULL;
                lexer_read_char(lexer);
            } else {
                tok.type = TOKEN_COLON;
                tok.value = NULL;
            }
            break;
        case '.':
            tok.type = TOKEN_DOT;
            tok.value = NULL;
            lexer_read_char(lexer);
            break;
        case '+':
            lexer_read_char(lexer);
            if (lexer->ch == '+') {
                tok.type = TOKEN_PLUS_PLUS;
                tok.value = NULL;
                lexer_read_char(lexer);
            } else {
                tok.type = TOKEN_PLUS;
                tok.value = NULL;
            }
            break;
        case '-':
            lexer_read_char(lexer);
            if (lexer->ch == '-') {
                tok.type = TOKEN_MINUS_MINUS;
                tok.value = NULL;
                lexer_read_char(lexer);
            } else {
                tok.type = TOKEN_MINUS;
                tok.value = NULL;
            }
            break;
        case '/':
            tok.type = TOKEN_SLASH;
            tok.value = NULL;
            lexer_read_char(lexer);
            break;
        case '"':
            tok.type = TOKEN_STRING;
            tok.value = lexer_read_quoted_string(lexer);
            break;
        case '\'':
            tok.type = TOKEN_CHAR;
            tok.value = lexer_read_char_literal(lexer);
            break;
        case 0:
            tok.type = TOKEN_EOF;
            tok.value = NULL;
            break;
        default:
            if (isalpha(lexer->ch) || lexer->ch == '_') {
                char* ident = lexer_read_identifier(lexer);
                tok.type = lexer_lookup_keyword(ident);
                if (tok.type == TOKEN_IDENTIFIER || tok.type == TOKEN_STRING_TYPE) {
                    tok.value = ident;
                } else {
                    free(ident);
                    tok.value = NULL;
                }
                return tok;
            } else if (isdigit(lexer->ch)) {
                int is_float;
                tok.value = lexer_read_number(lexer, &is_float);
                tok.type = is_float ? TOKEN_FLOAT : TOKEN_INTEGER;
                return tok;
            } else {
                tok.type = TOKEN_ILLEGAL;
                tok.value = (char*)malloc(2);
                tok.value[0] = lexer->ch;
                tok.value[1] = '\0';
                lexer_read_char(lexer);
            }
    }
    return tok;
}
