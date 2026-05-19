#ifndef FREQ_COMMON_H
#define FREQ_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef _WIN32
#define FREQ_WINDOWS 1
#define FREQ_PATH_SEPARATOR '\\'
#elif defined(__APPLE__)
#define FREQ_MACOS 1
#define FREQ_PATH_SEPARATOR '/'
#elif defined(__linux__)
#define FREQ_LINUX 1
#define FREQ_PATH_SEPARATOR '/'
#else
#define FREQ_UNKNOWN 1
#define FREQ_PATH_SEPARATOR '/'
#endif

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_CHAR,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_VAR,
    TOKEN_LET,
    TOKEN_CONST,
    TOKEN_INT,
    TOKEN_INT64,
    TOKEN_LONG,
    TOKEN_FLOAT_TYPE,
    TOKEN_DOUBLE,
    TOKEN_BOOL,
    TOKEN_STRING_TYPE,
    TOKEN_CHAR_TYPE,
    TOKEN_PRINT,
    TOKEN_PRINTN,
    TOKEN_PRINTF,
    TOKEN_PRINTFN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_ELIF,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_SWITCH,
    TOKEN_DEFAULT,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_FO,
    TOKEN_FUNC,
    TOKEN_RETURN,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_QUIT,
    TOKEN_EXIT,
    TOKEN_COMMA,
    TOKEN_EQUALS,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_SEMICOLON,
    TOKEN_NEWLINE,
    TOKEN_COLON,
    TOKEN_DOT,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PLUS_PLUS,
    TOKEN_MINUS_MINUS,
    TOKEN_AMPERSAND,
    TOKEN_STRUCT,
    TOKEN_ENUM,
    TOKEN_MOD,
    TOKEN_IMPORT,
    TOKEN_USE,
    TOKEN_OUT,
    TOKEN_DOUBLE_COLON,
    TOKEN_ILLEGAL
} TokenType;

typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

typedef enum {
    AST_VAR_DECL,
    AST_LET_DECL,
    AST_CONST_DECL,
    AST_ASSIGN_STMT,
    AST_IF_STMT,
    AST_SWITCH_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_FO_STMT,
    AST_FUNC_DECL,
    AST_FUNC_CALL,
    AST_RETURN_STMT,
    AST_PRINT_STMT,
    AST_PRINTN_STMT,
    AST_PRINTF_STMT,
    AST_PRINTFN_STMT,
    AST_IMPLICIT_PRINT_STMT,
    AST_LITERAL,
    AST_IDENTIFIER,
    AST_TYPE_CAST,
    AST_BINARY_EXPR,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_QUIT_STMT,
    AST_EXIT_STMT,
    AST_ARRAY_LITERAL,
    AST_ARRAY_ACCESS,
    AST_ARRAY_PROPERTY,
    AST_POINTER_DECL,
    AST_DEREF,
    AST_ADDRESS_OF,
    AST_STRUCT_DECL,
    AST_STRUCT_LITERAL,
    AST_STRUCT_ACCESS,
    AST_STRUCT_ASSIGN,
    AST_ENUM_DECL,
    AST_MODULE_DECL,
    AST_IMPORT_STMT,
    AST_USE_STMT,
    AST_MODULE_ACCESS
} ASTNodeType;

typedef enum {
    VAL_INT,
    VAL_INT64,
    VAL_LONG,
    VAL_FLOAT,
    VAL_DOUBLE,
    VAL_BOOL,
    VAL_STRING,
    VAL_CHAR,
    VAL_ARRAY,
    VAL_POINTER,
    VAL_STRUCT
} ValueType;

struct Value;

typedef struct ArrayValue {
    ValueType element_type;
    int dimensions;
    int* sizes;
    struct Value* elements;
    int num_elements;
} ArrayValue;

typedef struct StructValue StructValue;

typedef struct {
    char* field_name;
    ValueType field_type;
    int offset;
} StructField;

typedef struct {
    char* name;
    StructField* fields;
    int field_count;
    int size;
} StructType;

#define MAX_ENUM_MEMBERS 64

typedef struct {
    char* name;
    int64_t value;
} EnumMember;

typedef struct {
    char* name;
    EnumMember* members;
    int member_count;
} EnumType;

typedef struct Value {
    ValueType type;
    union {
        int32_t int_val;
        int64_t long_val;
        float float_val;
        double double_val;
        bool bool_val;
        char* string_val;
        char char_val;
        ArrayValue* array_val;
        uintptr_t pointer_val;
        StructValue* struct_val;
    } data;
} Value;

struct StructValue {
    StructType* type;
    Value* fields;
};

struct ASTNode;

typedef struct {
    char* name;
    ValueType var_type;
    struct ASTNode* initializer;
    int is_exported;
} VarDeclNode;

typedef struct {
    struct ASTNode** args;
    int arg_count;
} PrintStmtNode;

typedef struct {
    Value value;
} LiteralNode;

typedef struct {
    char* name;
    int cached_idx;
} IdentifierNode;

typedef struct {
    ValueType target_type;
    struct ASTNode* expr;
    int is_method_style;
} TypeCastNode;

typedef enum {
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE
} BinaryOp;

typedef struct {
    BinaryOp op;
    struct ASTNode* left;
    struct ASTNode* right;
} BinaryExprNode;

typedef struct ASTNode ASTNode;

typedef struct {
    ASTNode* condition;
    ASTNode* body;
    ASTNode* else_body;
} IfStmtNode;

typedef enum {
    CASE_SINGLE,
    CASE_MULTIPLE,
    CASE_RANGE
} CaseType;

typedef struct {
    CaseType type;
    ASTNode* values;
    ASTNode* range_start;
    ASTNode* range_end;
} CaseLabel;

typedef struct SwitchCase {
    CaseLabel label;
    ASTNode* body;
    int is_default;
    struct SwitchCase* next;
} SwitchCase;

typedef struct {
    ASTNode* expr;
    SwitchCase* cases;
    int is_expression;
} SwitchStmtNode;

typedef struct {
    ASTNode* condition;
    ASTNode* body;
    ASTNode* quit_condition;
    int has_quit;
} WhileStmtNode;

typedef struct {
    ASTNode* init;
    ASTNode* condition;
    ASTNode* increment;
    ASTNode* body;
} ForStmtNode;

typedef struct {
    char* var_name;
    ASTNode* range_start;
    ASTNode* range_end;
    ASTNode* array_expr;
    ASTNode* body;
} FoStmtNode;

typedef struct {
    char* name;
    ValueType type;
} FuncParam;

typedef struct {
    char* name;
    FuncParam* params;
    int param_count;
    ASTNode* body;
} FuncDeclNode;

typedef struct {
    ASTNode* expr;
    ValueType return_type;
} ReturnStmtNode;

typedef struct {
    char* name;
    ASTNode** args;
    int arg_count;
} FuncCallNode;

typedef struct ArrayLiteralNode {
    ValueType element_type;
    int dimensions;
    int* sizes;
    ASTNode** elements;
    int num_elements;
} ArrayLiteralNode;

typedef struct ArrayAccessNode {
    char* name;
    ASTNode** indices;
    int num_indices;
} ArrayAccessNode;

typedef enum {
    PROPERTY_LEN
} ArrayPropertyType;

typedef struct ArrayPropertyNode {
    char* name;
    ArrayPropertyType property;
} ArrayPropertyNode;

typedef struct {
    char* name;
    ValueType base_type;
    struct ASTNode* initializer;
} PointerDeclNode;

typedef struct {
    struct ASTNode* expr;
} DerefNode;

typedef struct {
    char* name;
} AddressOfNode;

typedef struct {
    char* name;
    char** field_names;
    ValueType* field_types;
    int field_count;
} StructDeclNode;

typedef struct {
    char* struct_name;
    char** field_names;
    struct ASTNode** field_values;
    int field_count;
} StructLiteralNode;

typedef struct {
    char* struct_name;
    char* field_name;
} StructAccessNode;

typedef struct {
    char* struct_name;
    char* field_name;
    struct ASTNode* value;
} StructAssignNode;

typedef struct {
    char* name;
    char** member_names;
    int64_t* member_values;
    int member_count;
} EnumDeclNode;

typedef struct {
    char* name;
    ASTNode* body;
} ModuleDeclNode;

typedef struct {
    char* module_name;
} ImportStmtNode;

typedef struct {
    char* module_name;
    char* func_name;
} UseStmtNode;

typedef struct {
    char* module_name;
    char* member_name;
} ModuleAccessNode;

typedef struct ASTNode {
    ASTNodeType type;
    union {
        VarDeclNode var_decl;
        PrintStmtNode print_stmt;
        LiteralNode literal;
        IdentifierNode identifier;
        TypeCastNode type_cast;
        BinaryExprNode binary_expr;
        IfStmtNode if_stmt;
        SwitchStmtNode switch_stmt;
        WhileStmtNode while_stmt;
        ForStmtNode for_stmt;
        FoStmtNode fo_stmt;
        FuncDeclNode func_decl;
        FuncCallNode func_call;
        ReturnStmtNode return_stmt;
        ArrayLiteralNode array_literal;
        ArrayAccessNode array_access;
        ArrayPropertyNode array_property;
        PointerDeclNode pointer_decl;
        DerefNode deref;
        AddressOfNode address_of;
        StructDeclNode struct_decl;
        StructLiteralNode struct_literal;
        StructAccessNode struct_access;
        StructAssignNode struct_assign;
        EnumDeclNode enum_decl;
        ModuleDeclNode module_decl;
        ImportStmtNode import_stmt;
        UseStmtNode use_stmt;
        ModuleAccessNode module_access;
    } data;
    struct ASTNode* next;
} ASTNode;

typedef struct {
    ASTNode* head;
    ASTNode* tail;
    int node_count;
} AST;

#endif
