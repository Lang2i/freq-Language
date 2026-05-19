#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

static void parser_next_token(Parser* parser) {
    parser->current_token = parser->peek_token;
    parser->peek_token = lexer_next_token(parser->lexer);
}

static int parser_current_token_is(Parser* parser, TokenType type) {
    return parser->current_token.type == type;
}

static int parser_peek_token_is(Parser* parser, TokenType type) {
    return parser->peek_token.type == type;
}

static void parser_skip_newlines(Parser* parser) {
    while (parser_current_token_is(parser, TOKEN_NEWLINE) || 
           parser_current_token_is(parser, TOKEN_SEMICOLON)) {
        parser_next_token(parser);
    }
}

static ASTNode* parse_statement(Parser* parser);
static ASTNode* parse_condition(Parser* parser);
static ASTNode* parse_expression(Parser* parser);

static ASTNode* ast_node_new(ASTNodeType type) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    node->next = NULL;
    return node;
}

static ValueType parse_type_keyword(Parser* parser) {
    switch (parser->current_token.type) {
        case TOKEN_INT: return VAL_INT;
        case TOKEN_INT64: return VAL_INT64;
        case TOKEN_LONG: return VAL_LONG;
        case TOKEN_FLOAT_TYPE: return VAL_FLOAT;
        case TOKEN_DOUBLE: return VAL_DOUBLE;
        case TOKEN_BOOL: return VAL_BOOL;
        case TOKEN_STRING_TYPE: return VAL_STRING;
        case TOKEN_CHAR_TYPE: return VAL_CHAR;
        default: return VAL_INT;
    }
}

static char* parser_read_identifier(Parser* parser) {
    if (!parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
        return NULL;
    }
    char* name = strdup(parser->current_token.value);
    free(parser->current_token.value);
    parser_next_token(parser);
    return name;
}

static ASTNode* parse_literal_or_identifier(Parser* parser) {
    ASTNode* node;
    
    if (parser_current_token_is(parser, TOKEN_INT) ||
        parser_current_token_is(parser, TOKEN_LONG) ||
        parser_current_token_is(parser, TOKEN_FLOAT_TYPE) ||
        parser_current_token_is(parser, TOKEN_DOUBLE) ||
        parser_current_token_is(parser, TOKEN_BOOL) ||
        parser_current_token_is(parser, TOKEN_STRING_TYPE) ||
        parser_current_token_is(parser, TOKEN_CHAR_TYPE)) {
        
        ValueType target_type = parse_type_keyword(parser);
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        if (parser_current_token_is(parser, TOKEN_LPAREN)) {
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            ASTNode* expr = parse_literal_or_identifier(parser);
            
            parser_skip_newlines(parser);
            if (parser_current_token_is(parser, TOKEN_RPAREN)) {
                parser_next_token(parser);
            } else {
                fprintf(stderr, "Error: Missing closing parenthesis\n");
                parser->has_error = 1;
                return NULL;
            }
            
            node = ast_node_new(AST_TYPE_CAST);
            node->data.type_cast.target_type = target_type;
            node->data.type_cast.expr = expr;
            node->data.type_cast.is_method_style = 0;
            return node;
        }
    }
    
    switch (parser->current_token.type) {
        case TOKEN_STAR: {
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            ASTNode* inner = parse_literal_or_identifier(parser);
            if (!inner) {
                fprintf(stderr, "Error: Expected expression after '*'\n");
                parser->has_error = 1;
                return NULL;
            }
            
            node = ast_node_new(AST_DEREF);
            node->data.deref.expr = inner;
            break;
        }
        case TOKEN_AMPERSAND: {
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            if (!parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
                fprintf(stderr, "Error: Expected identifier after '&'\n");
                parser->has_error = 1;
                return NULL;
            }
            
            node = ast_node_new(AST_ADDRESS_OF);
            node->data.address_of.name = parser->current_token.value;
            parser_next_token(parser);
            break;
        }
        case TOKEN_INTEGER:
            node = ast_node_new(AST_LITERAL);
            node->data.literal.value.type = VAL_INT64;
            node->data.literal.value.data.long_val = atoll(parser->current_token.value);
            free(parser->current_token.value);
            parser_next_token(parser);
            break;
            
        case TOKEN_FLOAT:
            node = ast_node_new(AST_LITERAL);
            node->data.literal.value.type = VAL_FLOAT;
            node->data.literal.value.data.float_val = atof(parser->current_token.value);
            free(parser->current_token.value);
            parser_next_token(parser);
            break;
            
        case TOKEN_STRING:
            node = ast_node_new(AST_LITERAL);
            node->data.literal.value.type = VAL_STRING;
            node->data.literal.value.data.string_val = strdup(parser->current_token.value);
            free(parser->current_token.value);
            parser_next_token(parser);
            break;
            
        case TOKEN_CHAR:
            node = ast_node_new(AST_LITERAL);
            node->data.literal.value.type = VAL_CHAR;
            node->data.literal.value.data.char_val = parser->current_token.value[0];
            free(parser->current_token.value);
            parser_next_token(parser);
            break;
            
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            node = ast_node_new(AST_LITERAL);
            node->data.literal.value.type = VAL_BOOL;
            node->data.literal.value.data.bool_val = (parser->current_token.type == TOKEN_TRUE);
            if (parser->current_token.value != NULL) {
                free(parser->current_token.value);
            }
            parser_next_token(parser);
            break;
            
        case TOKEN_LBRACKET: {
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            ASTNode* arr_node = ast_node_new(AST_ARRAY_LITERAL);
            arr_node->data.array_literal.element_type = VAL_INT;
            arr_node->data.array_literal.dimensions = 1;
            arr_node->data.array_literal.sizes = NULL;
            arr_node->data.array_literal.elements = NULL;
            arr_node->data.array_literal.num_elements = 0;
            
            int capacity = 16;
            arr_node->data.array_literal.elements = malloc(capacity * sizeof(ASTNode*));
            
            while (!parser_current_token_is(parser, TOKEN_RBRACKET) && 
                   !parser_current_token_is(parser, TOKEN_EOF)) {
                if (parser_current_token_is(parser, TOKEN_LBRACKET)) {
                    ASTNode* nested = parse_literal_or_identifier(parser);
                    if (nested && nested->type == AST_ARRAY_LITERAL) {
                        if (arr_node->data.array_literal.num_elements == 0) {
                            arr_node->data.array_literal.dimensions = nested->data.array_literal.dimensions + 1;
                            arr_node->data.array_literal.sizes = malloc(arr_node->data.array_literal.dimensions * sizeof(int));
                            arr_node->data.array_literal.sizes[0] = 0;
                            for (int i = 0; i < nested->data.array_literal.dimensions; i++) {
                                arr_node->data.array_literal.sizes[i + 1] = nested->data.array_literal.sizes[i];
                            }
                        }
                        arr_node->data.array_literal.sizes[0]++;
                        arr_node->data.array_literal.elements[arr_node->data.array_literal.num_elements++] = nested;
                    }
                } else {
                    ASTNode* elem = parse_literal_or_identifier(parser);
                    if (elem) {
                        if (arr_node->data.array_literal.num_elements == 0 && !arr_node->data.array_literal.sizes) {
                            arr_node->data.array_literal.sizes = malloc(sizeof(int));
                        }
                        arr_node->data.array_literal.sizes[0]++;
                        arr_node->data.array_literal.elements[arr_node->data.array_literal.num_elements++] = elem;
                        if (elem->type == AST_LITERAL) {
                            if (arr_node->data.array_literal.num_elements == 1) {
                                arr_node->data.array_literal.element_type = elem->data.literal.value.type;
                            }
                        }
                    }
                }
                
                if (arr_node->data.array_literal.num_elements >= capacity) {
                    capacity *= 2;
                    arr_node->data.array_literal.elements = realloc(arr_node->data.array_literal.elements, capacity * sizeof(ASTNode*));
                }
                
                parser_skip_newlines(parser);
                if (parser_current_token_is(parser, TOKEN_COMMA)) {
                    parser_next_token(parser);
                    parser_skip_newlines(parser);
                } else if (!parser_current_token_is(parser, TOKEN_RBRACKET)) {
                    break;
                }
            }
            
            if (parser_current_token_is(parser, TOKEN_RBRACKET)) {
                parser_next_token(parser);
            }
            
            node = arr_node;
            break;
        }
            
        case TOKEN_IDENTIFIER: {
            char* name = parser_read_identifier(parser);
            if (!name) {
                return NULL;
            }
            parser_skip_newlines(parser);
            
            if (parser_current_token_is(parser, TOKEN_LBRACKET)) {
                ASTNode* access_node = ast_node_new(AST_ARRAY_ACCESS);
                access_node->data.array_access.name = name;
                access_node->data.array_access.indices = NULL;
                access_node->data.array_access.num_indices = 0;
                
                int capacity = 4;
                access_node->data.array_access.indices = malloc(capacity * sizeof(ASTNode*));
                
                while (parser_current_token_is(parser, TOKEN_LBRACKET)) {
                    parser_next_token(parser);
                    parser_skip_newlines(parser);
                    
                    ASTNode* index = parse_condition(parser);
                    if (index) {
                        access_node->data.array_access.indices[access_node->data.array_access.num_indices++] = index;
                    }
                    
                    if (access_node->data.array_access.num_indices >= capacity) {
                        capacity *= 2;
                        access_node->data.array_access.indices = realloc(access_node->data.array_access.indices, capacity * sizeof(ASTNode*));
                    }
                    
                    parser_skip_newlines(parser);
                    if (parser_current_token_is(parser, TOKEN_RBRACKET)) {
                        parser_next_token(parser);
                        parser_skip_newlines(parser);
                    }
                }
                
                node = access_node;
                break;
            }
            
            if (parser_current_token_is(parser, TOKEN_DOUBLE_COLON)) {
                parser_next_token(parser);
                parser_skip_newlines(parser);
                
                if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
                    char* member_name = strdup(parser->current_token.value);
                    free(parser->current_token.value);
                    parser_next_token(parser);
                    parser_skip_newlines(parser);
                    
                    if (parser_current_token_is(parser, TOKEN_LPAREN)) {
                        ASTNode* call_node = ast_node_new(AST_FUNC_CALL);
                        call_node->data.func_call.name = malloc(strlen(name) + strlen(member_name) + 3);
                        sprintf(call_node->data.func_call.name, "%s::%s", name, member_name);
                        call_node->data.func_call.args = NULL;
                        call_node->data.func_call.arg_count = 0;
                        
                        int capacity = 4;
                        call_node->data.func_call.args = malloc(capacity * sizeof(ASTNode*));
                        
                        parser_next_token(parser);
                        parser_skip_newlines(parser);
                        
                        while (!parser_current_token_is(parser, TOKEN_RPAREN) && 
                               !parser_current_token_is(parser, TOKEN_EOF)) {
                            ASTNode* arg = parse_expression(parser);
                            if (arg) {
                                if (call_node->data.func_call.arg_count >= capacity) {
                                    capacity *= 2;
                                    call_node->data.func_call.args = realloc(call_node->data.func_call.args, capacity * sizeof(ASTNode*));
                                }
                                call_node->data.func_call.args[call_node->data.func_call.arg_count++] = arg;
                            }
                            
                            parser_skip_newlines(parser);
                            if (parser_current_token_is(parser, TOKEN_COMMA)) {
                                parser_next_token(parser);
                                parser_skip_newlines(parser);
                            } else if (!parser_current_token_is(parser, TOKEN_RPAREN)) {
                                break;
                            }
                        }
                        
                        if (parser_current_token_is(parser, TOKEN_RPAREN)) {
                            parser_next_token(parser);
                        }
                        
                        free(member_name);
                        node = call_node;
                    } else {
                        ASTNode* access_node = ast_node_new(AST_MODULE_ACCESS);
                        access_node->data.module_access.module_name = name;
                        access_node->data.module_access.member_name = member_name;
                        node = access_node;
                    }
                    break;
                }
            }
            
            if (parser_current_token_is(parser, TOKEN_LPAREN)) {
                parser_next_token(parser);
                parser_skip_newlines(parser);
                
                ASTNode* call_node = ast_node_new(AST_FUNC_CALL);
                call_node->data.func_call.name = name;
                call_node->data.func_call.args = NULL;
                call_node->data.func_call.arg_count = 0;
                
                int capacity = 4;
                call_node->data.func_call.args = malloc(capacity * sizeof(ASTNode*));
                
                while (!parser_current_token_is(parser, TOKEN_RPAREN) && 
                       !parser_current_token_is(parser, TOKEN_EOF)) {
                    ASTNode* arg = parse_expression(parser);
                    if (arg) {
                        if (call_node->data.func_call.arg_count >= capacity) {
                            capacity *= 2;
                            call_node->data.func_call.args = realloc(call_node->data.func_call.args, capacity * sizeof(ASTNode*));
                        }
                        call_node->data.func_call.args[call_node->data.func_call.arg_count++] = arg;
                    }
                    
                    parser_skip_newlines(parser);
                    if (parser_current_token_is(parser, TOKEN_COMMA)) {
                        parser_next_token(parser);
                        parser_skip_newlines(parser);
                    } else if (!parser_current_token_is(parser, TOKEN_RPAREN)) {
                        break;
                    }
                }
                
                if (parser_current_token_is(parser, TOKEN_RPAREN)) {
                    parser_next_token(parser);
                }
                
                node = call_node;
                break;
            }
            
            node = ast_node_new(AST_IDENTIFIER);
            node->data.identifier.name = name;
            node->data.identifier.cached_idx = -1;
            break;
        }
            
        default:
            node = ast_node_new(AST_LITERAL);
            node->data.literal.value.type = VAL_STRING;
            node->data.literal.value.data.string_val = strdup(parser->current_token.value);
            if (parser->current_token.value) {
                free(parser->current_token.value);
            }
            parser_next_token(parser);
            break;
    }
    
    return node;
}

static ASTNode* parse_print_statement(Parser* parser, ASTNodeType type) {
    ASTNode* node = ast_node_new(type);
    node->data.print_stmt.arg_count = 0;
    node->data.print_stmt.args = NULL;
    
    int start_line = parser->current_token.line;
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    int has_paren = parser_current_token_is(parser, TOKEN_LPAREN);
    if (has_paren) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
    }
    
    int capacity = 4;
    node->data.print_stmt.args = (ASTNode**)malloc(capacity * sizeof(ASTNode*));
    
    ASTNode* arg = parse_literal_or_identifier(parser);
    if (arg) {
        node->data.print_stmt.args[node->data.print_stmt.arg_count++] = arg;
    }
    
    while (parser_current_token_is(parser, TOKEN_COMMA)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        arg = parse_literal_or_identifier(parser);
        if (arg) {
            if (node->data.print_stmt.arg_count >= capacity) {
                capacity *= 2;
                node->data.print_stmt.args = (ASTNode**)realloc(node->data.print_stmt.args, capacity * sizeof(ASTNode*));
            }
            node->data.print_stmt.args[node->data.print_stmt.arg_count++] = arg;
        }
    }
    
    if (has_paren) {
        parser_skip_newlines(parser);
        if (parser_current_token_is(parser, TOKEN_RPAREN)) {
            parser_next_token(parser);
        } else {
            fprintf(stderr, "Error at line %d: Missing closing parenthesis\n", start_line);
            node->data.print_stmt.arg_count = -1;
            parser->has_error = 1;
        }
    }
    
    return node;
}

static ValueType parse_type(Parser* parser) {
    ValueType type = VAL_INT;
    switch (parser->current_token.type) {
        case TOKEN_INT:
            type = VAL_INT;
            break;
        case TOKEN_INT64:
            type = VAL_INT64;
            break;
        case TOKEN_LONG:
            type = VAL_LONG;
            break;
        case TOKEN_FLOAT_TYPE:
            type = VAL_FLOAT;
            break;
        case TOKEN_DOUBLE:
            type = VAL_DOUBLE;
            break;
        case TOKEN_BOOL:
            type = VAL_BOOL;
            break;
        case TOKEN_STRING_TYPE:
            type = VAL_STRING;
            break;
        case TOKEN_CHAR_TYPE:
            type = VAL_CHAR;
            break;
        default:
            break;
    }
    parser_next_token(parser);
    return type;
}

static ASTNode* parse_pointer_declaration(Parser* parser) {
    ASTNode* node = ast_node_new(AST_POINTER_DECL);
    
    if (!parser_current_token_is(parser, TOKEN_STAR)) {
        fprintf(stderr, "Error: Expected '*' in pointer declaration\n");
        parser->has_error = 1;
        return NULL;
    }
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    if (!parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
        fprintf(stderr, "Error: Expected identifier after '*'\n");
        parser->has_error = 1;
        return NULL;
    }
    
    node->data.pointer_decl.name = parser->current_token.value;
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    node->data.pointer_decl.base_type = VAL_INT;
    
    if (parser_current_token_is(parser, TOKEN_COLON)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        if (parser_current_token_is(parser, TOKEN_INT) ||
            parser_current_token_is(parser, TOKEN_LONG) ||
            parser_current_token_is(parser, TOKEN_FLOAT_TYPE) ||
            parser_current_token_is(parser, TOKEN_DOUBLE) ||
            parser_current_token_is(parser, TOKEN_BOOL) ||
            parser_current_token_is(parser, TOKEN_STRING_TYPE) ||
            parser_current_token_is(parser, TOKEN_CHAR_TYPE)) {
            node->data.pointer_decl.base_type = parse_type(parser);
            parser_skip_newlines(parser);
        }
    }
    
    node->data.pointer_decl.initializer = NULL;
    
    if (parser_current_token_is(parser, TOKEN_EQUALS)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        node->data.pointer_decl.initializer = parse_literal_or_identifier(parser);
        
        parser_skip_newlines(parser);
        
        if (parser_current_token_is(parser, TOKEN_SEMICOLON)) {
            parser_next_token(parser);
        }
    }
    
    return node;
}

static ASTNode* parse_var_declaration(Parser* parser, ASTNodeType type) {
    ASTNode* node = ast_node_new(type);
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_STAR)) {
        return parse_pointer_declaration(parser);
    }
    
    node->data.var_decl.var_type = VAL_INT64;
    
    if (parser_current_token_is(parser, TOKEN_INT) ||
        parser_current_token_is(parser, TOKEN_INT64) ||
        parser_current_token_is(parser, TOKEN_LONG) ||
        parser_current_token_is(parser, TOKEN_FLOAT_TYPE) ||
        parser_current_token_is(parser, TOKEN_DOUBLE) ||
        parser_current_token_is(parser, TOKEN_BOOL) ||
        parser_current_token_is(parser, TOKEN_STRING_TYPE) ||
        parser_current_token_is(parser, TOKEN_CHAR_TYPE)) {
        node->data.var_decl.var_type = parse_type(parser);
        parser_skip_newlines(parser);
    }
    
    if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
        node->data.var_decl.name = parser_read_identifier(parser);
        parser_skip_newlines(parser);
        
        if (parser_current_token_is(parser, TOKEN_COLON)) {
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            if (parser_current_token_is(parser, TOKEN_INT) ||
                parser_current_token_is(parser, TOKEN_LONG) ||
                parser_current_token_is(parser, TOKEN_FLOAT_TYPE) ||
                parser_current_token_is(parser, TOKEN_DOUBLE) ||
                parser_current_token_is(parser, TOKEN_BOOL) ||
                parser_current_token_is(parser, TOKEN_STRING_TYPE) ||
                parser_current_token_is(parser, TOKEN_CHAR_TYPE)) {
                node->data.var_decl.var_type = parse_type(parser);
                parser_skip_newlines(parser);
            }
        } else if (parser_current_token_is(parser, TOKEN_INT) ||
                   parser_current_token_is(parser, TOKEN_LONG) ||
                   parser_current_token_is(parser, TOKEN_FLOAT_TYPE) ||
                   parser_current_token_is(parser, TOKEN_DOUBLE) ||
                   parser_current_token_is(parser, TOKEN_BOOL) ||
                   parser_current_token_is(parser, TOKEN_STRING_TYPE) ||
                   parser_current_token_is(parser, TOKEN_CHAR_TYPE)) {
            node->data.var_decl.var_type = parse_type(parser);
            parser_skip_newlines(parser);
        }
        
        parser_skip_newlines(parser);
        
        node->data.var_decl.initializer = NULL;
        
        if (parser_current_token_is(parser, TOKEN_EQUALS)) {
            parser_next_token(parser);
            parser_skip_newlines(parser);
            node->data.var_decl.initializer = parse_expression(parser);
            
            parser_skip_newlines(parser);
            
            if (parser_current_token_is(parser, TOKEN_SEMICOLON)) {
                parser_next_token(parser);
            }
        }
    } else {
        node->data.var_decl.name = NULL;
        node->data.var_decl.initializer = NULL;
    }
    
    return node;
}

static ASTNode* parse_assignment(Parser* parser) {
    ASTNode* node = ast_node_new(AST_ASSIGN_STMT);
    node->data.var_decl.name = parser->current_token.value;
    node->data.var_decl.initializer = NULL;
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_EQUALS)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        node->data.var_decl.initializer = parse_condition(parser);
    }
    
    return node;
}

static ASTNode* parse_expression(Parser* parser) {
    ASTNode* left = parse_literal_or_identifier(parser);
    if (!left) return NULL;
    
    while (1) {
        BinaryOp op = OP_ADD;
        int has_op = 0;
        
        switch (parser->current_token.type) {
            case TOKEN_PLUS:
                op = OP_ADD;
                has_op = 1;
                break;
            case TOKEN_MINUS:
                op = OP_SUBTRACT;
                has_op = 1;
                break;
            case TOKEN_STAR:
                op = OP_MULTIPLY;
                has_op = 1;
                break;
            case TOKEN_SLASH:
                op = OP_DIVIDE;
                has_op = 1;
                break;
            default:
                return left;
        }
        
        if (has_op) {
            parser_next_token(parser);
            ASTNode* right = parse_literal_or_identifier(parser);
            if (!right) return left;
            
            ASTNode* node = ast_node_new(AST_BINARY_EXPR);
            node->data.binary_expr.op = op;
            node->data.binary_expr.left = left;
            node->data.binary_expr.right = right;
            left = node;
        }
    }
}

static ASTNode* parse_condition(Parser* parser) {
    ASTNode* left = parse_expression(parser);
    if (!left) return NULL;
    
    BinaryOp op = OP_EQUAL;
    int has_op = 0;
    
    switch (parser->current_token.type) {
        case TOKEN_EQUAL_EQUAL:
            op = OP_EQUAL;
            has_op = 1;
            break;
        case TOKEN_NOT_EQUAL:
            op = OP_NOT_EQUAL;
            has_op = 1;
            break;
        case TOKEN_GREATER:
            op = OP_GREATER;
            has_op = 1;
            break;
        case TOKEN_GREATER_EQUAL:
            op = OP_GREATER_EQUAL;
            has_op = 1;
            break;
        case TOKEN_LESS:
            op = OP_LESS;
            has_op = 1;
            break;
        case TOKEN_LESS_EQUAL:
            op = OP_LESS_EQUAL;
            has_op = 1;
            break;
        default:
            return left;
    }
    
    if (has_op) {
        parser_next_token(parser);
        ASTNode* right = parse_expression(parser);
        if (!right) return left;
        
        ASTNode* node = ast_node_new(AST_BINARY_EXPR);
        node->data.binary_expr.op = op;
        node->data.binary_expr.left = left;
        node->data.binary_expr.right = right;
        return node;
    }
    
    return left;
}

static ASTNode* parse_block(Parser* parser) {
    ASTNode* head = NULL;
    ASTNode* tail = NULL;
    
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_SEMICOLON)) {
        parser_next_token(parser);
        return NULL;
    }
    
    if (parser_current_token_is(parser, TOKEN_LBRACE)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        while (!parser_current_token_is(parser, TOKEN_RBRACE) && 
               !parser_current_token_is(parser, TOKEN_EOF)) {
            if (parser_current_token_is(parser, TOKEN_ELSE) || 
                parser_current_token_is(parser, TOKEN_ELIF)) {
                break;
            }
            
            ASTNode* stmt = parse_statement(parser);
            if (stmt) {
                if (!head) {
                    head = stmt;
                    tail = stmt;
                } else {
                    tail->next = stmt;
                    tail = stmt;
                }
            }
            parser_skip_newlines(parser);
        }
        
        if (parser_current_token_is(parser, TOKEN_RBRACE)) {
            parser_next_token(parser);
        }
    } else {
        ASTNode* stmt = parse_statement(parser);
        head = stmt;
        tail = stmt;
    }
    
    return head;
}

static ASTNode* parse_if_statement(Parser* parser) {
    ASTNode* node = ast_node_new(AST_IF_STMT);
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_LPAREN)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        node->data.if_stmt.condition = parse_condition(parser);
        
        parser_skip_newlines(parser);
        if (parser_current_token_is(parser, TOKEN_RPAREN)) {
            parser_next_token(parser);
        } else {
            fprintf(stderr, "Error: Missing closing parenthesis in if statement\n");
            parser->has_error = 1;
            return node;
        }
    }
    
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_SEMICOLON)) {
        parser_next_token(parser);
        node->data.if_stmt.body = NULL;
    } else {
        node->data.if_stmt.body = parse_block(parser);
    }
    node->data.if_stmt.else_body = NULL;
    
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_ELSE)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        if (parser_current_token_is(parser, TOKEN_IF)) {
            node->data.if_stmt.else_body = parse_if_statement(parser);
        } else if (parser_current_token_is(parser, TOKEN_ELIF)) {
            node->data.if_stmt.else_body = parse_if_statement(parser);
        } else {
            if (parser_current_token_is(parser, TOKEN_SEMICOLON)) {
                parser_next_token(parser);
                node->data.if_stmt.else_body = NULL;
            } else {
                node->data.if_stmt.else_body = parse_block(parser);
            }
        }
    } else if (parser_current_token_is(parser, TOKEN_ELIF)) {
        node->data.if_stmt.else_body = parse_if_statement(parser);
    }
    
    return node;
}

static ASTNode* parse_switch_statement(Parser* parser) {
    ASTNode* node = ast_node_new(AST_SWITCH_STMT);
    node->data.switch_stmt.expr = NULL;
    node->data.switch_stmt.cases = NULL;
    node->data.switch_stmt.is_expression = 0;
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_LPAREN)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        node->data.switch_stmt.expr = parse_literal_or_identifier(parser);
        
        parser_skip_newlines(parser);
        if (parser_current_token_is(parser, TOKEN_RPAREN)) {
            parser_next_token(parser);
        } else {
            fprintf(stderr, "Error: Missing closing parenthesis in switch statement\n");
            parser->has_error = 1;
            return node;
        }
    }
    
    parser_skip_newlines(parser);
    
    if (!parser_current_token_is(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Error: Expected '{' after switch\n");
        parser->has_error = 1;
        return node;
    }
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    SwitchCase* case_head = NULL;
    SwitchCase* case_tail = NULL;
    
    while (!parser_current_token_is(parser, TOKEN_RBRACE) && 
           !parser_current_token_is(parser, TOKEN_EOF)) {
        
        if (parser_current_token_is(parser, TOKEN_DEFAULT)) {
            SwitchCase* sc = (SwitchCase*)malloc(sizeof(SwitchCase));
            sc->label.type = CASE_SINGLE;
            sc->label.values = NULL;
            sc->label.range_start = NULL;
            sc->label.range_end = NULL;
            sc->body = NULL;
            sc->is_default = 1;
            sc->next = NULL;
            
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            if (!parser_current_token_is(parser, TOKEN_COLON)) {
                fprintf(stderr, "Error: Expected ':' after default\n");
                parser->has_error = 1;
                return node;
            }
            
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            sc->body = parse_statement(parser);
            
            if (!case_head) {
                case_head = sc;
                case_tail = sc;
            } else {
                case_tail->next = sc;
                case_tail = sc;
            }
            
            parser_skip_newlines(parser);
            continue;
        }
        
        ASTNode* first_value = parse_literal_or_identifier(parser);
        if (!first_value) {
            break;
        }
        
        parser_skip_newlines(parser);
        
        SwitchCase* sc = (SwitchCase*)malloc(sizeof(SwitchCase));
        sc->body = NULL;
        sc->is_default = 0;
        sc->next = NULL;
        
        if (parser_current_token_is(parser, TOKEN_COMMA)) {
            sc->label.type = CASE_MULTIPLE;
            sc->label.values = first_value;
            first_value->next = NULL;
            ASTNode* values_tail = first_value;
            
            while (parser_current_token_is(parser, TOKEN_COMMA)) {
                parser_next_token(parser);
                parser_skip_newlines(parser);
                
                ASTNode* value = parse_literal_or_identifier(parser);
                if (value) {
                    value->next = NULL;
                    values_tail->next = value;
                    values_tail = value;
                }
                
                parser_skip_newlines(parser);
            }
        } else if (parser_current_token_is(parser, TOKEN_DOT) && 
                   parser_peek_token_is(parser, TOKEN_DOT)) {
            sc->label.type = CASE_RANGE;
            sc->label.range_start = first_value;
            sc->label.range_end = NULL;
            
            parser_next_token(parser);
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            sc->label.range_end = parse_literal_or_identifier(parser);
        } else {
            sc->label.type = CASE_SINGLE;
            sc->label.values = first_value;
            first_value->next = NULL;
        }
        
        parser_skip_newlines(parser);
        
        if (!parser_current_token_is(parser, TOKEN_COLON)) {
            fprintf(stderr, "Error: Expected ':' in switch case\n");
            parser->has_error = 1;
            return node;
        }
        
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        sc->body = parse_statement(parser);
        
        if (!case_head) {
            case_head = sc;
            case_tail = sc;
        } else {
            case_tail->next = sc;
            case_tail = sc;
        }
        
        parser_skip_newlines(parser);
    }
    
    if (parser_current_token_is(parser, TOKEN_RBRACE)) {
        parser_next_token(parser);
    }
    
    node->data.switch_stmt.cases = case_head;
    
    return node;
}

static ASTNode* parse_for_statement(Parser* parser) {
    ASTNode* node = ast_node_new(AST_FOR_STMT);
    node->data.for_stmt.init = NULL;
    node->data.for_stmt.condition = NULL;
    node->data.for_stmt.increment = NULL;
    node->data.for_stmt.body = NULL;
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    int has_parens = parser_current_token_is(parser, TOKEN_LPAREN);
    if (has_parens) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        parser_skip_newlines(parser);
        if (!parser_current_token_is(parser, TOKEN_SEMICOLON) && 
            !parser_current_token_is(parser, TOKEN_RPAREN)) {
            if (parser_current_token_is(parser, TOKEN_VAR) || 
                parser_current_token_is(parser, TOKEN_LET)) {
                ASTNodeType decl_type = parser_current_token_is(parser, TOKEN_VAR) ? AST_VAR_DECL : AST_LET_DECL;
                ASTNode* decl_node = ast_node_new(decl_type);
                decl_node->data.var_decl.var_type = VAL_INT64;
                decl_node->data.var_decl.name = NULL;
                decl_node->data.var_decl.initializer = NULL;
                
                parser_next_token(parser);
                parser_skip_newlines(parser);
                
                if (parser_current_token_is(parser, TOKEN_INT) ||
                    parser_current_token_is(parser, TOKEN_LONG) ||
                    parser_current_token_is(parser, TOKEN_FLOAT_TYPE) ||
                    parser_current_token_is(parser, TOKEN_DOUBLE) ||
                    parser_current_token_is(parser, TOKEN_BOOL) ||
                    parser_current_token_is(parser, TOKEN_STRING_TYPE) ||
                    parser_current_token_is(parser, TOKEN_CHAR_TYPE)) {
                    decl_node->data.var_decl.var_type = parse_type(parser);
                    parser_skip_newlines(parser);
                }
                
                if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
                    decl_node->data.var_decl.name = parser->current_token.value;
                    parser_next_token(parser);
                    parser_skip_newlines(parser);
                    
                    if (parser_current_token_is(parser, TOKEN_EQUALS)) {
                        parser_next_token(parser);
                        parser_skip_newlines(parser);
                        decl_node->data.var_decl.initializer = parse_literal_or_identifier(parser);
                    }
                }
                node->data.for_stmt.init = decl_node;
            } else if (parser_current_token_is(parser, TOKEN_INT) ||
                       parser_current_token_is(parser, TOKEN_LONG) ||
                       parser_current_token_is(parser, TOKEN_FLOAT_TYPE) ||
                       parser_current_token_is(parser, TOKEN_DOUBLE) ||
                       parser_current_token_is(parser, TOKEN_BOOL) ||
                       parser_current_token_is(parser, TOKEN_STRING_TYPE) ||
                       parser_current_token_is(parser, TOKEN_CHAR_TYPE)) {
                ASTNode* decl_node = ast_node_new(AST_VAR_DECL);
                decl_node->data.var_decl.var_type = parse_type(parser);
                parser_skip_newlines(parser);
                
                if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
                    decl_node->data.var_decl.name = parser->current_token.value;
                    parser_next_token(parser);
                    parser_skip_newlines(parser);
                    
                    if (parser_current_token_is(parser, TOKEN_EQUALS)) {
                        parser_next_token(parser);
                        parser_skip_newlines(parser);
                        decl_node->data.var_decl.initializer = parse_literal_or_identifier(parser);
                    }
                }
                node->data.for_stmt.init = decl_node;
            } else if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
                if (parser_peek_token_is(parser, TOKEN_EQUALS)) {
                    node->data.for_stmt.init = parse_assignment(parser);
                } else if (parser_peek_token_is(parser, TOKEN_PLUS_PLUS) || 
                          parser_peek_token_is(parser, TOKEN_MINUS_MINUS)) {
                    ASTNode* assign_node = ast_node_new(AST_ASSIGN_STMT);
                    assign_node->data.var_decl.name = parser->current_token.value;
                    parser_next_token(parser);
                    
                    int is_plus = parser_current_token_is(parser, TOKEN_PLUS_PLUS);
                    parser_next_token(parser);
                    
                    ASTNode* expr_node = ast_node_new(AST_BINARY_EXPR);
                    expr_node->data.binary_expr.op = is_plus ? OP_ADD : OP_SUBTRACT;
                    
                    ASTNode* left_node = ast_node_new(AST_IDENTIFIER);
                    left_node->data.identifier.name = assign_node->data.var_decl.name;
                    left_node->data.identifier.cached_idx = -1;
                    expr_node->data.binary_expr.left = left_node;
                    
                    ASTNode* right_node = ast_node_new(AST_LITERAL);
                    right_node->data.literal.value.type = VAL_INT64;
                    right_node->data.literal.value.data.long_val = 1;
                    expr_node->data.binary_expr.right = right_node;
                    
                    assign_node->data.var_decl.initializer = expr_node;
                    node->data.for_stmt.init = assign_node;
                }
            }
        }
        
        parser_skip_newlines(parser);
        if (parser_current_token_is(parser, TOKEN_SEMICOLON)) {
            parser_next_token(parser);
        }
        
        parser_skip_newlines(parser);
        if (!parser_current_token_is(parser, TOKEN_SEMICOLON) && 
            !parser_current_token_is(parser, TOKEN_RPAREN)) {
            node->data.for_stmt.condition = parse_condition(parser);
        }
        
        parser_skip_newlines(parser);
        if (parser_current_token_is(parser, TOKEN_SEMICOLON)) {
            parser_next_token(parser);
        }
        
        parser_skip_newlines(parser);
        if (!parser_current_token_is(parser, TOKEN_RPAREN)) {
            if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
                if (parser_peek_token_is(parser, TOKEN_PLUS_PLUS) || 
                    parser_peek_token_is(parser, TOKEN_MINUS_MINUS)) {
                    ASTNode* assign_node = ast_node_new(AST_ASSIGN_STMT);
                    assign_node->data.var_decl.name = parser->current_token.value;
                    parser_next_token(parser);
                    
                    int is_plus = parser_current_token_is(parser, TOKEN_PLUS_PLUS);
                    parser_next_token(parser);
                    
                    ASTNode* expr_node = ast_node_new(AST_BINARY_EXPR);
                    expr_node->data.binary_expr.op = is_plus ? OP_ADD : OP_SUBTRACT;
                    
                    ASTNode* left_node = ast_node_new(AST_IDENTIFIER);
                    left_node->data.identifier.name = assign_node->data.var_decl.name;
                    left_node->data.identifier.cached_idx = -1;
                    expr_node->data.binary_expr.left = left_node;
                    
                    ASTNode* right_node = ast_node_new(AST_LITERAL);
                    right_node->data.literal.value.type = VAL_INT64;
                    right_node->data.literal.value.data.long_val = 1;
                    expr_node->data.binary_expr.right = right_node;
                    
                    assign_node->data.var_decl.initializer = expr_node;
                    node->data.for_stmt.increment = assign_node;
                } else {
                    node->data.for_stmt.increment = parse_condition(parser);
                }
            } else {
                node->data.for_stmt.increment = parse_condition(parser);
            }
        }
        
        parser_skip_newlines(parser);
        if (has_parens && parser_current_token_is(parser, TOKEN_RPAREN)) {
            parser_next_token(parser);
        } else if (has_parens) {
            fprintf(stderr, "Error: Missing closing parenthesis in for statement\n");
            parser->has_error = 1;
            return node;
        }
    } else {
        parser_skip_newlines(parser);
        if (parser_current_token_is(parser, TOKEN_VAR) || 
            parser_current_token_is(parser, TOKEN_LET)) {
            ASTNodeType decl_type = parser_current_token_is(parser, TOKEN_VAR) ? AST_VAR_DECL : AST_LET_DECL;
            ASTNode* decl_node = ast_node_new(decl_type);
            decl_node->data.var_decl.var_type = VAL_INT64;
            decl_node->data.var_decl.name = NULL;
            decl_node->data.var_decl.initializer = NULL;
            
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
                decl_node->data.var_decl.name = parser->current_token.value;
                parser_next_token(parser);
                parser_skip_newlines(parser);
                
                if (parser_current_token_is(parser, TOKEN_EQUALS)) {
                    parser_next_token(parser);
                    parser_skip_newlines(parser);
                    decl_node->data.var_decl.initializer = parse_literal_or_identifier(parser);
                }
            }
            node->data.for_stmt.init = decl_node;
        }
        
        parser_skip_newlines(parser);
        if (parser_current_token_is(parser, TOKEN_SEMICOLON)) {
            parser_next_token(parser);
        }
        
        parser_skip_newlines(parser);
        if (!parser_current_token_is(parser, TOKEN_SEMICOLON)) {
            node->data.for_stmt.condition = parse_condition(parser);
        }
        
        parser_skip_newlines(parser);
        if (parser_current_token_is(parser, TOKEN_SEMICOLON)) {
            parser_next_token(parser);
        }
        
        parser_skip_newlines(parser);
        if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
            if (parser_peek_token_is(parser, TOKEN_PLUS_PLUS) || 
                parser_peek_token_is(parser, TOKEN_MINUS_MINUS)) {
                ASTNode* assign_node = ast_node_new(AST_ASSIGN_STMT);
                assign_node->data.var_decl.name = parser->current_token.value;
                parser_next_token(parser);
                
                int is_plus = parser_current_token_is(parser, TOKEN_PLUS_PLUS);
                parser_next_token(parser);
                
                ASTNode* expr_node = ast_node_new(AST_BINARY_EXPR);
                expr_node->data.binary_expr.op = is_plus ? OP_ADD : OP_SUBTRACT;
                
                ASTNode* left_node = ast_node_new(AST_IDENTIFIER);
                left_node->data.identifier.name = assign_node->data.var_decl.name;
                left_node->data.identifier.cached_idx = -1;
                expr_node->data.binary_expr.left = left_node;
                
                ASTNode* right_node = ast_node_new(AST_LITERAL);
                right_node->data.literal.value.type = VAL_INT64;
                right_node->data.literal.value.data.long_val = 1;
                expr_node->data.binary_expr.right = right_node;
                
                assign_node->data.var_decl.initializer = expr_node;
                node->data.for_stmt.increment = assign_node;
            }
        }
    }
    
    parser_skip_newlines(parser);
    
    if (!parser_current_token_is(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Error: Expected '{' after for\n");
        parser->has_error = 1;
        return node;
    }
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    ASTNode* head = NULL;
    ASTNode* tail = NULL;
    
    while (!parser_current_token_is(parser, TOKEN_RBRACE) && 
           !parser_current_token_is(parser, TOKEN_EOF)) {
        
        ASTNode* stmt = parse_statement(parser);
        if (stmt) {
            if (!head) {
                head = stmt;
                tail = stmt;
            } else {
                tail->next = stmt;
                tail = stmt;
            }
        }
        parser_skip_newlines(parser);
    }
    
    if (parser_current_token_is(parser, TOKEN_RBRACE)) {
        parser_next_token(parser);
    }
    
    node->data.for_stmt.body = head;
    
    return node;
}

static ASTNode* parse_fo_statement(Parser* parser) {
    ASTNode* node = ast_node_new(AST_FO_STMT);
    node->data.fo_stmt.var_name = NULL;
    node->data.fo_stmt.range_start = NULL;
    node->data.fo_stmt.range_end = NULL;
    node->data.fo_stmt.array_expr = NULL;
    node->data.fo_stmt.body = NULL;
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_LPAREN)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        ASTNode* start_expr = parse_literal_or_identifier(parser);
        parser_skip_newlines(parser);
        
        if (parser_current_token_is(parser, TOKEN_DOT) && 
            parser_peek_token_is(parser, TOKEN_DOT)) {
            parser_next_token(parser);
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            ASTNode* end_expr = parse_literal_or_identifier(parser);
            
            node->data.fo_stmt.range_start = start_expr;
            node->data.fo_stmt.range_end = end_expr;
        } else {
            node->data.fo_stmt.array_expr = start_expr;
        }
        
        parser_skip_newlines(parser);
        if (parser_current_token_is(parser, TOKEN_RPAREN)) {
            parser_next_token(parser);
        } else {
            fprintf(stderr, "Error: Missing closing parenthesis in fo statement\n");
            parser->has_error = 1;
            return node;
        }
    }
    
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_LBRACKET)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
            node->data.fo_stmt.var_name = strdup(parser->current_token.value);
            free(parser->current_token.value);
            parser_next_token(parser);
        }
        
        parser_skip_newlines(parser);
        if (parser_current_token_is(parser, TOKEN_RBRACKET)) {
            parser_next_token(parser);
        }
    }
    
    parser_skip_newlines(parser);
    
    if (!parser_current_token_is(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Error: Expected '{' after fo\n");
        parser->has_error = 1;
        return node;
    }
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    ASTNode* head = NULL;
    ASTNode* tail = NULL;
    
    while (!parser_current_token_is(parser, TOKEN_RBRACE) && 
           !parser_current_token_is(parser, TOKEN_EOF)) {
        
        ASTNode* stmt = parse_statement(parser);
        if (stmt) {
            if (!head) {
                head = stmt;
                tail = stmt;
            } else {
                tail->next = stmt;
                tail = stmt;
            }
        }
        parser_skip_newlines(parser);
    }
    
    if (parser_current_token_is(parser, TOKEN_RBRACE)) {
        parser_next_token(parser);
    }
    
    node->data.fo_stmt.body = head;
    
    return node;
}

static ASTNode* parse_struct_declaration(Parser* parser) {
    ASTNode* node = ast_node_new(AST_STRUCT_DECL);
    node->data.struct_decl.name = NULL;
    node->data.struct_decl.field_names = NULL;
    node->data.struct_decl.field_types = NULL;
    node->data.struct_decl.field_count = 0;
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
        node->data.struct_decl.name = strdup(parser->current_token.value);
        free(parser->current_token.value);
        parser_next_token(parser);
        parser_skip_newlines(parser);
    }
    
    if (!parser_current_token_is(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Error at line %d: Expected '{' after struct name\n", parser->current_token.line);
        parser->has_error = 1;
        return node;
    }
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    int capacity = 4;
    node->data.struct_decl.field_names = malloc(capacity * sizeof(char*));
    node->data.struct_decl.field_types = malloc(capacity * sizeof(ValueType));
    
    while (!parser_current_token_is(parser, TOKEN_RBRACE) && 
           !parser_current_token_is(parser, TOKEN_EOF)) {
        
        if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
            char* field_name = parser->current_token.value;
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            ValueType field_type = VAL_INT;
            
            if (parser_current_token_is(parser, TOKEN_COLON)) {
                parser_next_token(parser);
                parser_skip_newlines(parser);
                
                if (parser_current_token_is(parser, TOKEN_INT)) {
                    field_type = VAL_INT;
                    parser_next_token(parser);
                } else if (parser_current_token_is(parser, TOKEN_LONG)) {
                    field_type = VAL_LONG;
                    parser_next_token(parser);
                } else if (parser_current_token_is(parser, TOKEN_FLOAT_TYPE)) {
                    field_type = VAL_FLOAT;
                    parser_next_token(parser);
                } else if (parser_current_token_is(parser, TOKEN_DOUBLE)) {
                    field_type = VAL_DOUBLE;
                    parser_next_token(parser);
                } else if (parser_current_token_is(parser, TOKEN_BOOL)) {
                    field_type = VAL_BOOL;
                    parser_next_token(parser);
                } else if (parser_current_token_is(parser, TOKEN_STRING_TYPE)) {
                    field_type = VAL_STRING;
                    parser_next_token(parser);
                } else if (parser_current_token_is(parser, TOKEN_CHAR_TYPE)) {
                    field_type = VAL_CHAR;
                    parser_next_token(parser);
                } else {
                    field_type = VAL_INT;
                }
            } else {
                field_type = VAL_INT;
            }
            
            if (node->data.struct_decl.field_count >= capacity) {
                capacity *= 2;
                node->data.struct_decl.field_names = realloc(node->data.struct_decl.field_names, capacity * sizeof(char*));
                node->data.struct_decl.field_types = realloc(node->data.struct_decl.field_types, capacity * sizeof(ValueType));
            }
            
            node->data.struct_decl.field_names[node->data.struct_decl.field_count] = field_name;
            node->data.struct_decl.field_types[node->data.struct_decl.field_count] = field_type;
            node->data.struct_decl.field_count++;
        }
        
        parser_skip_newlines(parser);
        
        if (parser_current_token_is(parser, TOKEN_SEMICOLON)) {
            parser_next_token(parser);
            parser_skip_newlines(parser);
        }
    }
    
    if (parser_current_token_is(parser, TOKEN_RBRACE)) {
        parser_next_token(parser);
    } else {
        fprintf(stderr, "Error at line %d: Missing closing brace in struct declaration\n", parser->current_token.line);
        parser->has_error = 1;
    }
    
    return node;
}

static ASTNode* parse_enum_declaration(Parser* parser) {
    ASTNode* node = ast_node_new(AST_ENUM_DECL);
    node->data.enum_decl.name = NULL;
    node->data.enum_decl.member_names = NULL;
    node->data.enum_decl.member_values = NULL;
    node->data.enum_decl.member_count = 0;
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
        node->data.enum_decl.name = strdup(parser->current_token.value);
        free(parser->current_token.value);
        parser_next_token(parser);
        parser_skip_newlines(parser);
    }
    
    if (!parser_current_token_is(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Error at line %d: Expected '{' after enum name\n", parser->current_token.line);
        parser->has_error = 1;
        return node;
    }
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    int capacity = 4;
    node->data.enum_decl.member_names = malloc(capacity * sizeof(char*));
    node->data.enum_decl.member_values = malloc(capacity * sizeof(int64_t));
    
    int64_t next_value = 0;
    
    while (!parser_current_token_is(parser, TOKEN_RBRACE) && 
           !parser_current_token_is(parser, TOKEN_EOF)) {
        
        if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
            char* member_name = strdup(parser->current_token.value);
            free(parser->current_token.value);
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            int64_t member_value = next_value++;
            
            if (parser_current_token_is(parser, TOKEN_EQUALS)) {
                parser_next_token(parser);
                parser_skip_newlines(parser);
                
                if (parser_current_token_is(parser, TOKEN_INTEGER)) {
                    member_value = atoll(parser->current_token.value);
                    next_value = member_value + 1;
                    free(parser->current_token.value);
                    parser_next_token(parser);
                } else {
                    fprintf(stderr, "Error at line %d: Expected integer value after '=' in enum\n", parser->current_token.line);
                    parser->has_error = 1;
                    free(member_name);
                    return node;
                }
            }
            
            if (node->data.enum_decl.member_count >= capacity) {
                capacity *= 2;
                node->data.enum_decl.member_names = realloc(node->data.enum_decl.member_names, capacity * sizeof(char*));
                node->data.enum_decl.member_values = realloc(node->data.enum_decl.member_values, capacity * sizeof(int64_t));
            }
            
            node->data.enum_decl.member_names[node->data.enum_decl.member_count] = member_name;
            node->data.enum_decl.member_values[node->data.enum_decl.member_count] = member_value;
            node->data.enum_decl.member_count++;
        }
        
        parser_skip_newlines(parser);
        
        if (parser_current_token_is(parser, TOKEN_COMMA)) {
            parser_next_token(parser);
            parser_skip_newlines(parser);
        }
    }
    
    if (parser_current_token_is(parser, TOKEN_RBRACE)) {
        parser_next_token(parser);
    } else {
        fprintf(stderr, "Error at line %d: Missing closing brace in enum declaration\n", parser->current_token.line);
        parser->has_error = 1;
    }
    
    return node;
}

static ASTNode* parse_module_declaration(Parser* parser) {
    ASTNode* node = ast_node_new(AST_MODULE_DECL);
    node->data.module_decl.name = NULL;
    node->data.module_decl.body = NULL;
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
        node->data.module_decl.name = strdup(parser->current_token.value);
        free(parser->current_token.value);
        parser_next_token(parser);
    }
    
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_LBRACE)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        ASTNode* head = NULL;
        ASTNode* tail = NULL;
        
        while (!parser_current_token_is(parser, TOKEN_RBRACE) && 
               !parser_current_token_is(parser, TOKEN_EOF)) {
            ASTNode* stmt = parse_statement(parser);
            if (stmt) {
                if (!head) {
                    head = stmt;
                    tail = stmt;
                } else {
                    tail->next = stmt;
                    tail = stmt;
                }
            }
            parser_skip_newlines(parser);
        }
        
        if (parser_current_token_is(parser, TOKEN_RBRACE)) {
            parser_next_token(parser);
        }
        
        node->data.module_decl.body = head;
    }
    
    return node;
}

static ASTNode* parse_import_statement(Parser* parser) {
    ASTNode* node = ast_node_new(AST_IMPORT_STMT);
    node->data.import_stmt.module_name = NULL;
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
        node->data.import_stmt.module_name = strdup(parser->current_token.value);
        free(parser->current_token.value);
        parser_next_token(parser);
    } else {
        fprintf(stderr, "Error at line %d: Expected module name after import\n", parser->current_token.line);
        parser->has_error = 1;
    }
    
    return node;
}

static ASTNode* parse_use_statement(Parser* parser) {
    ASTNode* node = ast_node_new(AST_USE_STMT);
    node->data.use_stmt.module_name = NULL;
    node->data.use_stmt.func_name = NULL;
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
        node->data.use_stmt.module_name = strdup(parser->current_token.value);
        free(parser->current_token.value);
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        if (parser_current_token_is(parser, TOKEN_DOUBLE_COLON)) {
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
                node->data.use_stmt.func_name = strdup(parser->current_token.value);
                free(parser->current_token.value);
                parser_next_token(parser);
                parser_skip_newlines(parser);
                
                if (parser_current_token_is(parser, TOKEN_LPAREN)) {
                    parser_next_token(parser);
                    if (parser_current_token_is(parser, TOKEN_RPAREN)) {
                        parser_next_token(parser);
                    }
                }
            } else {
                fprintf(stderr, "Error at line %d: Expected function name after ::\n", parser->current_token.line);
                parser->has_error = 1;
            }
        } else {
            fprintf(stderr, "Error at line %d: Expected :: after module name\n", parser->current_token.line);
            parser->has_error = 1;
        }
    } else {
        fprintf(stderr, "Error at line %d: Expected module name after use\n", parser->current_token.line);
        parser->has_error = 1;
    }
    
    return node;
}

static ASTNode* parse_func_declaration(Parser* parser) {
    ASTNode* node = ast_node_new(AST_FUNC_DECL);
    node->data.func_decl.name = NULL;
    node->data.func_decl.params = NULL;
    node->data.func_decl.param_count = 0;
    node->data.func_decl.body = NULL;
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
        node->data.func_decl.name = strdup(parser->current_token.value);
        free(parser->current_token.value);
        parser_next_token(parser);
    }
    
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_LPAREN)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        if (!parser_current_token_is(parser, TOKEN_RPAREN)) {
            int capacity = 4;
            node->data.func_decl.params = malloc(capacity * sizeof(FuncParam));
            
            while (!parser_current_token_is(parser, TOKEN_RPAREN) && 
                   !parser_current_token_is(parser, TOKEN_EOF)) {
            
            FuncParam param;
            param.name = NULL;
            param.type = VAL_INT;
            
            if (parser_current_token_is(parser, TOKEN_INT) ||
                parser_current_token_is(parser, TOKEN_LONG) ||
                parser_current_token_is(parser, TOKEN_FLOAT_TYPE) ||
                parser_current_token_is(parser, TOKEN_DOUBLE) ||
                parser_current_token_is(parser, TOKEN_BOOL) ||
                parser_current_token_is(parser, TOKEN_STRING_TYPE) ||
                parser_current_token_is(parser, TOKEN_CHAR_TYPE)) {
                param.type = parse_type_keyword(parser);
                parser_next_token(parser);
                parser_skip_newlines(parser);
                
                if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
                    param.name = strdup(parser->current_token.value);
                    free(parser->current_token.value);
                    parser_next_token(parser);
                }
            } else if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
                param.name = strdup(parser->current_token.value);
                free(parser->current_token.value);
                parser_next_token(parser);
                parser_skip_newlines(parser);
                
                if (parser_current_token_is(parser, TOKEN_COLON)) {
                    parser_next_token(parser);
                    parser_skip_newlines(parser);
                    
                    if (parser_current_token_is(parser, TOKEN_INT) ||
                        parser_current_token_is(parser, TOKEN_LONG) ||
                        parser_current_token_is(parser, TOKEN_FLOAT_TYPE) ||
                        parser_current_token_is(parser, TOKEN_DOUBLE) ||
                        parser_current_token_is(parser, TOKEN_BOOL) ||
                        parser_current_token_is(parser, TOKEN_STRING_TYPE) ||
                        parser_current_token_is(parser, TOKEN_CHAR_TYPE)) {
                        param.type = parse_type_keyword(parser);
                        parser_next_token(parser);
                    }
                }
            }
            
            if (param.name) {
                if (node->data.func_decl.param_count >= capacity) {
                    capacity *= 2;
                    node->data.func_decl.params = realloc(node->data.func_decl.params, capacity * sizeof(FuncParam));
                }
                node->data.func_decl.params[node->data.func_decl.param_count++] = param;
            }
            
            parser_skip_newlines(parser);
            if (parser_current_token_is(parser, TOKEN_COMMA)) {
                parser_next_token(parser);
                parser_skip_newlines(parser);
            } else if (!parser_current_token_is(parser, TOKEN_RPAREN)) {
                break;
            }
        }
        }
        
        if (parser_current_token_is(parser, TOKEN_RPAREN)) {
            parser_next_token(parser);
        }
    }
    
    parser_skip_newlines(parser);
    
    if (!parser_current_token_is(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Error: Expected '{' after function declaration\n");
        parser->has_error = 1;
        return node;
    }
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    ASTNode* head = NULL;
    ASTNode* tail = NULL;
    
    while (!parser_current_token_is(parser, TOKEN_RBRACE) && 
           !parser_current_token_is(parser, TOKEN_EOF)) {
        
        ASTNode* stmt = parse_statement(parser);
        if (stmt) {
            if (!head) {
                head = stmt;
                tail = stmt;
            } else {
                tail->next = stmt;
                tail = stmt;
            }
        }
        parser_skip_newlines(parser);
    }
    
    if (parser_current_token_is(parser, TOKEN_RBRACE)) {
        parser_next_token(parser);
    }
    
    node->data.func_decl.body = head;
    
    return node;
}

static ASTNode* parse_return_statement(Parser* parser) {
    ASTNode* node = ast_node_new(AST_RETURN_STMT);
    node->data.return_stmt.expr = NULL;
    node->data.return_stmt.return_type = VAL_INT;
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    node->data.return_stmt.expr = parse_expression(parser);
    
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_COLON)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        if (parser_current_token_is(parser, TOKEN_INT) ||
            parser_current_token_is(parser, TOKEN_LONG) ||
            parser_current_token_is(parser, TOKEN_FLOAT_TYPE) ||
            parser_current_token_is(parser, TOKEN_DOUBLE) ||
            parser_current_token_is(parser, TOKEN_BOOL) ||
            parser_current_token_is(parser, TOKEN_STRING_TYPE) ||
            parser_current_token_is(parser, TOKEN_CHAR_TYPE)) {
            node->data.return_stmt.return_type = parse_type_keyword(parser);
            parser_next_token(parser);
        }
    }
    
    return node;
}

static ASTNode* parse_while_statement(Parser* parser) {
    ASTNode* node = ast_node_new(AST_WHILE_STMT);
    node->data.while_stmt.condition = NULL;
    node->data.while_stmt.body = NULL;
    node->data.while_stmt.quit_condition = NULL;
    node->data.while_stmt.has_quit = 0;
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_LPAREN)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        node->data.while_stmt.condition = parse_condition(parser);
        
        parser_skip_newlines(parser);
        if (parser_current_token_is(parser, TOKEN_RPAREN)) {
            parser_next_token(parser);
        } else {
            fprintf(stderr, "Error: Missing closing parenthesis in while statement\n");
            parser->has_error = 1;
            return node;
        }
    }
    
    parser_skip_newlines(parser);
    
    if (!parser_current_token_is(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Error: Expected '{' after while\n");
        parser->has_error = 1;
        return node;
    }
    
    parser_next_token(parser);
    parser_skip_newlines(parser);
    
    ASTNode* head = NULL;
    ASTNode* tail = NULL;
    
    while (!parser_current_token_is(parser, TOKEN_RBRACE) && 
           !parser_current_token_is(parser, TOKEN_EOF)) {
        
        ASTNode* stmt = parse_statement(parser);
        if (stmt) {
            if (!head) {
                head = stmt;
                tail = stmt;
            } else {
                tail->next = stmt;
                tail = stmt;
            }
        }
        parser_skip_newlines(parser);
    }
    
    if (parser_current_token_is(parser, TOKEN_RBRACE)) {
        parser_next_token(parser);
    }
    
    node->data.while_stmt.body = head;
    
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_EXIT)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        if (parser_current_token_is(parser, TOKEN_LPAREN)) {
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            node->data.while_stmt.quit_condition = parse_condition(parser);
            
            parser_skip_newlines(parser);
            if (parser_current_token_is(parser, TOKEN_RPAREN)) {
                parser_next_token(parser);
            } else {
                fprintf(stderr, "Error: Missing closing parenthesis in exit\n");
                parser->has_error = 1;
                return node;
            }
        }
    }
    
    return node;
}

static ASTNode* parse_statement(Parser* parser) {
    ASTNode* stmt = NULL;
    
    parser_skip_newlines(parser);
    
    if (parser_current_token_is(parser, TOKEN_STAR)) {
        parser_next_token(parser);
        parser_skip_newlines(parser);
        
        if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
            char* name = parser->current_token.value;
            parser_next_token(parser);
            parser_skip_newlines(parser);
            
            if (parser_current_token_is(parser, TOKEN_EQUALS)) {
                parser_next_token(parser);
                parser_skip_newlines(parser);
                
                ASTNode* init = parse_expression(parser);
                
                ASTNode* assign_node = ast_node_new(AST_ASSIGN_STMT);
                assign_node->data.var_decl.name = name;
                
                ASTNode* deref_node = ast_node_new(AST_DEREF);
                ASTNode* ident_node = ast_node_new(AST_IDENTIFIER);
                ident_node->data.identifier.name = strdup(name);
                ident_node->data.identifier.cached_idx = -1;
                deref_node->data.deref.expr = ident_node;
                
                ASTNode* binary_node = ast_node_new(AST_BINARY_EXPR);
                binary_node->data.binary_expr.op = OP_EQUAL;
                binary_node->data.binary_expr.left = deref_node;
                binary_node->data.binary_expr.right = init;
                
                assign_node->data.var_decl.initializer = binary_node;
                return assign_node;
            } else {
                ASTNode* deref_node = ast_node_new(AST_DEREF);
                ASTNode* ident_node = ast_node_new(AST_IDENTIFIER);
                ident_node->data.identifier.name = name;
                ident_node->data.identifier.cached_idx = -1;
                deref_node->data.deref.expr = ident_node;
                
                stmt = ast_node_new(AST_IMPLICIT_PRINT_STMT);
                stmt->data.print_stmt.args = (ASTNode**)malloc(sizeof(ASTNode*));
                stmt->data.print_stmt.args[0] = deref_node;
                stmt->data.print_stmt.arg_count = 1;
                return stmt;
            }
        }
    }
    
    switch (parser->current_token.type) {
        case TOKEN_IDENTIFIER: {
            char* first_name = parser_read_identifier(parser);
            if (!first_name) {
                return NULL;
            }
            parser_skip_newlines(parser);
            
            if (parser_current_token_is(parser, TOKEN_DOUBLE_COLON)) {
                parser_next_token(parser);
                parser_skip_newlines(parser);
                
                if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
                    char* member_name = strdup(parser->current_token.value);
                    free(parser->current_token.value);
                    parser_next_token(parser);
                    parser_skip_newlines(parser);
                    
                    if (parser_current_token_is(parser, TOKEN_LPAREN)) {
                        ASTNode* call_node = ast_node_new(AST_FUNC_CALL);
                        call_node->data.func_call.name = malloc(strlen(first_name) + strlen(member_name) + 3);
                        sprintf(call_node->data.func_call.name, "%s::%s", first_name, member_name);
                        call_node->data.func_call.args = NULL;
                        call_node->data.func_call.arg_count = 0;
                        
                        int capacity = 4;
                        call_node->data.func_call.args = malloc(capacity * sizeof(ASTNode*));
                        
                        parser_next_token(parser);
                        parser_skip_newlines(parser);
                        
                        while (!parser_current_token_is(parser, TOKEN_RPAREN) && 
                               !parser_current_token_is(parser, TOKEN_EOF)) {
                            ASTNode* arg = parse_expression(parser);
                            if (arg) {
                                if (call_node->data.func_call.arg_count >= capacity) {
                                    capacity *= 2;
                                    call_node->data.func_call.args = realloc(call_node->data.func_call.args, capacity * sizeof(ASTNode*));
                                }
                                call_node->data.func_call.args[call_node->data.func_call.arg_count++] = arg;
                            }
                            
                            parser_skip_newlines(parser);
                            if (parser_current_token_is(parser, TOKEN_COMMA)) {
                                parser_next_token(parser);
                                parser_skip_newlines(parser);
                            } else if (!parser_current_token_is(parser, TOKEN_RPAREN)) {
                                break;
                            }
                        }
                        
                        if (parser_current_token_is(parser, TOKEN_RPAREN)) {
                            parser_next_token(parser);
                        }
                        
                        free(member_name);
                        free(first_name);
                        return call_node;
                    } else {
                        ASTNode* access_node = ast_node_new(AST_MODULE_ACCESS);
                        access_node->data.module_access.module_name = first_name;
                        access_node->data.module_access.member_name = member_name;
                        return access_node;
                    }
                }
                free(first_name);
            }
            
            if (parser_current_token_is(parser, TOKEN_DOT)) {
                parser_next_token(parser);
                parser_skip_newlines(parser);
                
                if (parser_current_token_is(parser, TOKEN_IDENTIFIER)) {
                    char* field_name = parser->current_token.value;
                    parser_next_token(parser);
                    parser_skip_newlines(parser);
                    
                    if (parser_current_token_is(parser, TOKEN_EQUALS)) {
                        parser_next_token(parser);
                        parser_skip_newlines(parser);
                        
                        ASTNode* value = parse_literal_or_identifier(parser);
                        
                        ASTNode* assign_node = ast_node_new(AST_STRUCT_ASSIGN);
                        assign_node->data.struct_assign.struct_name = first_name;
                        assign_node->data.struct_assign.field_name = field_name;
                        assign_node->data.struct_assign.value = value;
                        
                        parser_skip_newlines(parser);
                        if (parser_current_token_is(parser, TOKEN_SEMICOLON)) {
                            parser_next_token(parser);
                        }
                        
                        return assign_node;
                    } else {
                        ASTNode* access_node = ast_node_new(AST_STRUCT_ACCESS);
                        access_node->data.struct_access.struct_name = first_name;
                        access_node->data.struct_access.field_name = field_name;
                        
                        stmt = ast_node_new(AST_IMPLICIT_PRINT_STMT);
                        stmt->data.print_stmt.args = (ASTNode**)malloc(sizeof(ASTNode*));
                        stmt->data.print_stmt.args[0] = access_node;
                        stmt->data.print_stmt.arg_count = 1;
                        return stmt;
                    }
                }
            }
            
            parser_skip_newlines(parser);
            
            if (parser_current_token_is(parser, TOKEN_EQUALS)) {
                parser_next_token(parser);
                parser_skip_newlines(parser);
                
                ASTNode* init = parse_expression(parser);
                
                ASTNode* assign_node = ast_node_new(AST_ASSIGN_STMT);
                assign_node->data.var_decl.name = first_name;
                assign_node->data.var_decl.initializer = init;
                
                parser_skip_newlines(parser);
                if (parser_current_token_is(parser, TOKEN_SEMICOLON)) {
                    parser_next_token(parser);
                }
                
                return assign_node;
            } else if (parser_current_token_is(parser, TOKEN_PLUS_PLUS)) {
                parser_next_token(parser);
                
                ASTNode* assign_node = ast_node_new(AST_ASSIGN_STMT);
                assign_node->data.var_decl.name = first_name;
                
                ASTNode* binary_node = ast_node_new(AST_BINARY_EXPR);
                binary_node->data.binary_expr.op = OP_ADD;
                
                ASTNode* left_node = ast_node_new(AST_IDENTIFIER);
                left_node->data.identifier.name = first_name;
                left_node->data.identifier.cached_idx = -1;
                binary_node->data.binary_expr.left = left_node;
                
                ASTNode* right_node = ast_node_new(AST_LITERAL);
                right_node->data.literal.value.type = VAL_INT64;
                right_node->data.literal.value.data.long_val = 1;
                binary_node->data.binary_expr.right = right_node;
                
                assign_node->data.var_decl.initializer = binary_node;
                return assign_node;
            } else if (parser_current_token_is(parser, TOKEN_MINUS_MINUS)) {
                parser_next_token(parser);
                
                ASTNode* assign_node = ast_node_new(AST_ASSIGN_STMT);
                assign_node->data.var_decl.name = first_name;
                
                ASTNode* binary_node = ast_node_new(AST_BINARY_EXPR);
                binary_node->data.binary_expr.op = OP_SUBTRACT;
                
                ASTNode* left_node = ast_node_new(AST_IDENTIFIER);
                left_node->data.identifier.name = first_name;
                left_node->data.identifier.cached_idx = -1;
                binary_node->data.binary_expr.left = left_node;
                
                ASTNode* right_node = ast_node_new(AST_LITERAL);
                right_node->data.literal.value.type = VAL_INT64;
                right_node->data.literal.value.data.long_val = 1;
                binary_node->data.binary_expr.right = right_node;
                
                assign_node->data.var_decl.initializer = binary_node;
                return assign_node;
            } else if (parser_current_token_is(parser, TOKEN_LPAREN)) {
                parser_next_token(parser);
                parser_skip_newlines(parser);

                ASTNode* call_node = ast_node_new(AST_FUNC_CALL);
                call_node->data.func_call.name = first_name;
                call_node->data.func_call.args = NULL;
                call_node->data.func_call.arg_count = 0;

                int capacity = 4;
                call_node->data.func_call.args = malloc(capacity * sizeof(ASTNode*));

                while (!parser_current_token_is(parser, TOKEN_RPAREN) &&
                       !parser_current_token_is(parser, TOKEN_EOF)) {
                    ASTNode* arg = parse_expression(parser);
                    if (arg) {
                        if (call_node->data.func_call.arg_count >= capacity) {
                            capacity *= 2;
                            call_node->data.func_call.args = realloc(call_node->data.func_call.args, capacity * sizeof(ASTNode*));
                        }
                        call_node->data.func_call.args[call_node->data.func_call.arg_count++] = arg;
                    }

                    parser_skip_newlines(parser);
                    if (parser_current_token_is(parser, TOKEN_COMMA)) {
                        parser_next_token(parser);
                        parser_skip_newlines(parser);
                    } else if (!parser_current_token_is(parser, TOKEN_RPAREN)) {
                        break;
                    }
                }

                if (parser_current_token_is(parser, TOKEN_RPAREN)) {
                    parser_next_token(parser);
                }

                parser_skip_newlines(parser);
                if (parser_current_token_is(parser, TOKEN_SEMICOLON)) {
                    parser_next_token(parser);
                }

                return call_node;
            } else {
                ASTNode* ident_node = ast_node_new(AST_IDENTIFIER);
                ident_node->data.identifier.name = first_name;
                ident_node->data.identifier.cached_idx = -1;
                
                stmt = ast_node_new(AST_IMPLICIT_PRINT_STMT);
                stmt->data.print_stmt.args = (ASTNode**)malloc(sizeof(ASTNode*));
                stmt->data.print_stmt.args[0] = ident_node;
                stmt->data.print_stmt.arg_count = 1;
                return stmt;
            }
            break;
        }
        case TOKEN_IF:
        case TOKEN_ELIF:
            stmt = parse_if_statement(parser);
            break;
        case TOKEN_SWITCH:
            stmt = parse_switch_statement(parser);
            break;
        case TOKEN_FOR:
            stmt = parse_for_statement(parser);
            break;
        case TOKEN_FO:
            stmt = parse_fo_statement(parser);
            break;
        case TOKEN_WHILE:
            stmt = parse_while_statement(parser);
            break;
        case TOKEN_BREAK:
            stmt = ast_node_new(AST_BREAK_STMT);
            parser_next_token(parser);
            break;
        case TOKEN_CONTINUE:
            stmt = ast_node_new(AST_CONTINUE_STMT);
            parser_next_token(parser);
            break;
        case TOKEN_QUIT:
            stmt = ast_node_new(AST_QUIT_STMT);
            parser_next_token(parser);
            break;
        case TOKEN_FUNC:
            stmt = parse_func_declaration(parser);
            break;
        case TOKEN_STRUCT:
            stmt = parse_struct_declaration(parser);
            break;
        case TOKEN_ENUM:
            stmt = parse_enum_declaration(parser);
            break;
        case TOKEN_MOD:
            stmt = parse_module_declaration(parser);
            break;
        case TOKEN_IMPORT:
            stmt = parse_import_statement(parser);
            break;
        case TOKEN_USE:
            stmt = parse_use_statement(parser);
            break;
        case TOKEN_OUT: {
            parser_next_token(parser);
            parser_skip_newlines(parser);
            if (parser_current_token_is(parser, TOKEN_VAR)) {
                ASTNode* var_node = parse_var_declaration(parser, AST_VAR_DECL);
                if (var_node) {
                    var_node->data.var_decl.is_exported = 1;
                }
                stmt = var_node;
            } else if (parser_current_token_is(parser, TOKEN_LET)) {
                ASTNode* let_node = parse_var_declaration(parser, AST_LET_DECL);
                if (let_node) {
                    let_node->data.var_decl.is_exported = 1;
                }
                stmt = let_node;
            } else if (parser_current_token_is(parser, TOKEN_CONST)) {
                ASTNode* const_node = parse_var_declaration(parser, AST_CONST_DECL);
                if (const_node) {
                    const_node->data.var_decl.is_exported = 1;
                }
                stmt = const_node;
            } else {
                fprintf(stderr, "Error at line %d: Expected 'var', 'let', or 'const' after 'out'\n", parser->current_token.line);
                parser->has_error = 1;
                stmt = NULL;
            }
            break;
        }
        case TOKEN_RETURN:
            stmt = parse_return_statement(parser);
            break;
        case TOKEN_VAR:
            stmt = parse_var_declaration(parser, AST_VAR_DECL);
            break;
        case TOKEN_LET:
            stmt = parse_var_declaration(parser, AST_LET_DECL);
            break;
        case TOKEN_CONST:
            stmt = parse_var_declaration(parser, AST_CONST_DECL);
            break;
        case TOKEN_PRINT:
            stmt = parse_print_statement(parser, AST_PRINT_STMT);
            break;
        case TOKEN_PRINTN:
            stmt = parse_print_statement(parser, AST_PRINTN_STMT);
            break;
        case TOKEN_PRINTF:
            stmt = parse_print_statement(parser, AST_PRINTF_STMT);
            break;
        case TOKEN_PRINTFN:
            stmt = parse_print_statement(parser, AST_PRINTFN_STMT);
            break;
        case TOKEN_STRING:
            stmt = ast_node_new(AST_IMPLICIT_PRINT_STMT);
            stmt->data.print_stmt.args = (ASTNode**)malloc(sizeof(ASTNode*));
            stmt->data.print_stmt.args[0] = parse_literal_or_identifier(parser);
            stmt->data.print_stmt.arg_count = 1;
            break;
        case TOKEN_INTEGER:
        case TOKEN_FLOAT:
        case TOKEN_CHAR:
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            stmt = ast_node_new(AST_IMPLICIT_PRINT_STMT);
            stmt->data.print_stmt.args = (ASTNode**)malloc(sizeof(ASTNode*));
            stmt->data.print_stmt.args[0] = parse_literal_or_identifier(parser);
            stmt->data.print_stmt.arg_count = 1;
            break;
        case TOKEN_RBRACE:
        case TOKEN_LBRACE:
            return NULL;
        default:
            parser_next_token(parser);
            return NULL;
    }
    
    return stmt;
}

Parser* parser_new(Lexer* lexer) {
    Parser* parser = (Parser*)malloc(sizeof(Parser));
    parser->lexer = lexer;
    parser->has_error = 0;
    parser_next_token(parser);
    parser_next_token(parser);
    return parser;
}

void parser_free(Parser* parser) {
    free(parser);
}

AST* parser_parse(Parser* parser) {
    AST* ast = (AST*)malloc(sizeof(AST));
    ast->head = NULL;
    ast->tail = NULL;
    ast->node_count = 0;
    
    while (!parser_current_token_is(parser, TOKEN_EOF) && !parser->has_error) {
        ASTNode* stmt = parse_statement(parser);
        if (stmt) {
            if (ast->tail) {
                ast->tail->next = stmt;
            } else {
                ast->head = stmt;
            }
            ast->tail = stmt;
            ast->node_count++;
        } else {
            parser_next_token(parser);
        }
    }
    
    return ast;
}

static void ast_node_free(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_VAR_DECL:
        case AST_LET_DECL:
        case AST_CONST_DECL:
            if (node->data.var_decl.name) {
                free(node->data.var_decl.name);
            }
            if (node->data.var_decl.initializer) {
                ast_node_free(node->data.var_decl.initializer);
            }
            break;
            
        case AST_PRINT_STMT:
        case AST_PRINTN_STMT:
        case AST_PRINTF_STMT:
        case AST_PRINTFN_STMT:
        case AST_IMPLICIT_PRINT_STMT:
            if (node->data.print_stmt.args) {
                for (int i = 0; i < node->data.print_stmt.arg_count; i++) {
                    ast_node_free(node->data.print_stmt.args[i]);
                }
                free(node->data.print_stmt.args);
            }
            break;
            
        case AST_LITERAL:
            if (node->data.literal.value.type == VAL_STRING && node->data.literal.value.data.string_val) {
                free(node->data.literal.value.data.string_val);
            }
            break;
            
        case AST_IDENTIFIER:
            if (node->data.identifier.name) {
                free(node->data.identifier.name);
            }
            break;
            
        case AST_IF_STMT:
            if (node->data.if_stmt.condition) {
                ast_node_free(node->data.if_stmt.condition);
            }
            if (node->data.if_stmt.body) {
                ast_node_free(node->data.if_stmt.body);
            }
            if (node->data.if_stmt.else_body) {
                ast_node_free(node->data.if_stmt.else_body);
            }
            break;
            
        case AST_SWITCH_STMT:
            if (node->data.switch_stmt.expr) {
                ast_node_free(node->data.switch_stmt.expr);
            }
            {
                SwitchCase* sc = node->data.switch_stmt.cases;
                while (sc) {
                    SwitchCase* next = sc->next;
                    if (sc->label.values) {
                        ASTNode* v = sc->label.values;
                        while (v) {
                            ASTNode* vn = v->next;
                            ast_node_free(v);
                            v = vn;
                        }
                    }
                    if (sc->label.range_start) {
                        ast_node_free(sc->label.range_start);
                    }
                    if (sc->label.range_end) {
                        ast_node_free(sc->label.range_end);
                    }
                    if (sc->body) {
                        ast_node_free(sc->body);
                    }
                    free(sc);
                    sc = next;
                }
            }
            break;
            
        case AST_FOR_STMT:
            if (node->data.for_stmt.init) {
                ast_node_free(node->data.for_stmt.init);
            }
            if (node->data.for_stmt.condition) {
                ast_node_free(node->data.for_stmt.condition);
            }
            if (node->data.for_stmt.increment) {
                ast_node_free(node->data.for_stmt.increment);
            }
            if (node->data.for_stmt.body) {
                ASTNode* current = node->data.for_stmt.body;
                while (current) {
                    ASTNode* next = current->next;
                    ast_node_free(current);
                    current = next;
                }
            }
            break;
            
        case AST_WHILE_STMT:
            if (node->data.while_stmt.condition) {
                ast_node_free(node->data.while_stmt.condition);
            }
            if (node->data.while_stmt.body) {
                ASTNode* current = node->data.while_stmt.body;
                while (current) {
                    ASTNode* next = current->next;
                    ast_node_free(current);
                    current = next;
                }
            }
            if (node->data.while_stmt.quit_condition) {
                ast_node_free(node->data.while_stmt.quit_condition);
            }
            break;
            
        case AST_BINARY_EXPR:
            if (node->data.binary_expr.left) {
                ast_node_free(node->data.binary_expr.left);
            }
            if (node->data.binary_expr.right) {
                ast_node_free(node->data.binary_expr.right);
            }
            break;
            
        case AST_TYPE_CAST:
            if (node->data.type_cast.expr) {
                ast_node_free(node->data.type_cast.expr);
            }
            break;
            
        default:
            break;
    }
    
    free(node);
}

void ast_free(AST* ast) {
    if (!ast) return;
    
    ASTNode* current = ast->head;
    while (current) {
        ASTNode* next = current->next;
        ast_node_free(current);
        current = next;
    }
    
    free(ast);
}
