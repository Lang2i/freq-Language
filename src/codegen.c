#include "codegen.h"
#include "parser.h"
#include "lexer.h"
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Support.h>
#include <llvm-c/ExecutionEngine.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_LOCAL_VARS 64
#define MAX_GLOBAL_VARS 64

static LLVMModuleRef module;
static LLVMBuilderRef builder;
static LLVMContextRef context;
static LLVMValueRef current_function;
static int in_function_body;
static struct {
    const char* name;
    LLVMValueRef alloca;
    ValueType type;
} local_vars[MAX_LOCAL_VARS];
static int local_var_count;
static struct {
    const char* name;
    ValueType type;
} global_vars[MAX_GLOBAL_VARS];
static int global_var_count;
static LLVMBasicBlockRef current_loop_cond;
static LLVMBasicBlockRef current_loop_end;
static LLVMBasicBlockRef current_loop_after;

static void local_var_add(const char* name, LLVMValueRef alloca, ValueType type) {
    if (local_var_count < MAX_LOCAL_VARS) {
        local_vars[local_var_count].name = name;
        local_vars[local_var_count].alloca = alloca;
        local_vars[local_var_count].type = type;
        local_var_count++;
    }
}

static LLVMValueRef local_var_get(const char* name) {
    for (int i = 0; i < local_var_count; i++) {
        if (strcmp(local_vars[i].name, name) == 0) {
            return local_vars[i].alloca;
        }
    }
    return NULL;
}

static ValueType local_var_get_type(const char* name) {
    for (int i = 0; i < local_var_count; i++) {
        if (strcmp(local_vars[i].name, name) == 0) {
            return local_vars[i].type;
        }
    }
    return VAL_INT;
}

static void global_var_add(const char* name, ValueType type) {
    if (global_var_count < MAX_GLOBAL_VARS) {
        global_vars[global_var_count].name = name;
        global_vars[global_var_count].type = type;
        global_var_count++;
    }
}

static ValueType global_var_get_type(const char* name) {
    for (int i = 0; i < global_var_count; i++) {
        if (strcmp(global_vars[i].name, name) == 0) {
            return global_vars[i].type;
        }
    }
    return VAL_INT;
}

static LLVMTypeRef get_llvm_type(ValueType type) {
    switch (type) {
        case VAL_INT64:
            return LLVMInt64TypeInContext(context);
        case VAL_LONG:
            return LLVMInt64TypeInContext(context);
        case VAL_FLOAT:
            return LLVMFloatTypeInContext(context);
        case VAL_DOUBLE:
            return LLVMDoubleTypeInContext(context);
        case VAL_INT:
            return LLVMInt32TypeInContext(context);
        default:
            return LLVMInt64TypeInContext(context);
    }
}

static void local_vars_clear(void) {
    local_var_count = 0;
}

static LLVMValueRef generate_expr(ASTNode* expr);
static int generate_body(ASTNode* body);

static LLVMValueRef generate_binary_expr(ASTNode* expr) {
    LLVMValueRef left = generate_expr(expr->data.binary_expr.left);
    LLVMValueRef right = generate_expr(expr->data.binary_expr.right);
    if (!left || !right) return NULL;

    switch (expr->data.binary_expr.op) {
        case OP_ADD:
            return LLVMBuildAdd(builder, left, right, "addtmp");
        case OP_SUBTRACT:
            return LLVMBuildSub(builder, left, right, "subtmp");
        case OP_MULTIPLY:
            return LLVMBuildMul(builder, left, right, "multmp");
        case OP_DIVIDE:
            return LLVMBuildSDiv(builder, left, right, "divtmp");
        case OP_LESS:
            return LLVMBuildICmp(builder, LLVMIntSLT, left, right, "cmptmp");
        case OP_LESS_EQUAL:
            return LLVMBuildICmp(builder, LLVMIntSLE, left, right, "cmptmp");
        case OP_GREATER:
            return LLVMBuildICmp(builder, LLVMIntSGT, left, right, "cmptmp");
        case OP_GREATER_EQUAL:
            return LLVMBuildICmp(builder, LLVMIntSGE, left, right, "cmptmp");
        case OP_EQUAL:
            return LLVMBuildICmp(builder, LLVMIntEQ, left, right, "cmptmp");
        case OP_NOT_EQUAL:
            return LLVMBuildICmp(builder, LLVMIntNE, left, right, "cmptmp");
        default:
            return LLVMBuildAdd(builder, left, right, "addtmp");
    }
}

static LLVMValueRef generate_func_call_expr(ASTNode* stmt) {
    char func_name[256];
    strcpy(func_name, stmt->data.func_call.name);
    char fixed_name[256] = "";
    int j = 0;
    for (int i = 0; func_name[i]; i++) {
        if (func_name[i] == ':' && func_name[i+1] == ':') {
            fixed_name[j++] = '_';
            i++;
        } else {
            fixed_name[j++] = func_name[i];
        }
    }
    fixed_name[j] = '\0';

    LLVMValueRef func = LLVMGetNamedFunction(module, fixed_name);
    if (func) {
        LLVMValueRef* args = NULL;
        if (stmt->data.func_call.arg_count > 0) {
            args = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * stmt->data.func_call.arg_count);
            for (int i = 0; i < stmt->data.func_call.arg_count; i++) {
                args[i] = generate_expr(stmt->data.func_call.args[i]);
            }
        }
        LLVMTypeRef func_type = LLVMFunctionType(LLVMInt32TypeInContext(context), NULL, 0, 0);
        LLVMValueRef result = LLVMBuildCall2(builder, func_type, func, args, stmt->data.func_call.arg_count, "calltmp");
        if (args) free(args);
        return result;
    }
    return NULL;
}

static LLVMValueRef generate_expr(ASTNode* expr) {
    if (!expr) return NULL;

    switch (expr->type) {
        case AST_LITERAL: {
            if (expr->data.literal.value.type == VAL_INT64) {
                return LLVMConstInt(LLVMInt64TypeInContext(context), expr->data.literal.value.data.long_val, 0);
            } else if (expr->data.literal.value.type == VAL_INT) {
                return LLVMConstInt(LLVMInt32TypeInContext(context), expr->data.literal.value.data.int_val, 0);
            } else if (expr->data.literal.value.type == VAL_STRING) {
                return LLVMBuildGlobalStringPtr(builder, expr->data.literal.value.data.string_val, "str");
            }
            return LLVMConstInt(LLVMInt32TypeInContext(context), 0, 0);
        }
        case AST_IDENTIFIER: {
            LLVMValueRef val = local_var_get(expr->data.identifier.name);
            ValueType var_type;
            if (val) {
                var_type = local_var_get_type(expr->data.identifier.name);
            } else {
                val = LLVMGetNamedGlobal(module, expr->data.identifier.name);
                var_type = global_var_get_type(expr->data.identifier.name);
            }
            if (val) {
                LLVMValueRef loaded = LLVMBuildLoad2(builder, get_llvm_type(var_type), val, expr->data.identifier.name);
                return loaded;
            }
            return NULL;
        }
        case AST_BINARY_EXPR: {
            return generate_binary_expr(expr);
        }
        case AST_FUNC_CALL: {
            return generate_func_call_expr(expr);
        }
        default:
            return NULL;
    }
}

static int generate_statement(ASTNode* stmt) {
    if (!stmt) return 0;

    switch (stmt->type) {
        case AST_VAR_DECL: {
            LLVMTypeRef llvm_type = get_llvm_type(stmt->data.var_decl.var_type);
            if (in_function_body) {
                LLVMValueRef alloca = LLVMBuildAlloca(builder, llvm_type, stmt->data.var_decl.name);
                local_var_add(stmt->data.var_decl.name, alloca, stmt->data.var_decl.var_type);
                if (stmt->data.var_decl.initializer) {
                    LLVMValueRef init_val = generate_expr(stmt->data.var_decl.initializer);
                    if (init_val) {
                        LLVMBuildStore(builder, init_val, alloca);
                    }
                } else {
                    LLVMBuildStore(builder, LLVMConstInt(llvm_type, 0, 0), alloca);
                }
            }
            break;
        }
        case AST_ASSIGN_STMT: {
            LLVMValueRef var = local_var_get(stmt->data.var_decl.name);
            if (!var) {
                var = LLVMGetNamedGlobal(module, stmt->data.var_decl.name);
            }
            if (var && stmt->data.var_decl.initializer) {
                LLVMValueRef val = generate_expr(stmt->data.var_decl.initializer);
                if (val) {
                    if (in_function_body) {
                        LLVMBuildStore(builder, val, var);
                    } else {
                        LLVMSetInitializer(var, val);
                    }
                }
            }
            break;
        }
        case AST_PRINT_STMT:
        case AST_PRINTF_STMT: {
            if (stmt->data.print_stmt.args && stmt->data.print_stmt.arg_count > 0) {
                LLVMValueRef arg = generate_expr(stmt->data.print_stmt.args[0]);
                if (arg) {
                    LLVMValueRef printf_func = LLVMGetNamedFunction(module, "printf");
                    if (printf_func) {
                        LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32TypeInContext(context), (LLVMTypeRef[]){LLVMPointerType(LLVMInt8TypeInContext(context), 0)}, 1, 1);
                        ASTNode* first_arg = stmt->data.print_stmt.args[0];
                        if (first_arg->type == AST_LITERAL && first_arg->data.literal.value.type == VAL_STRING) {
                            LLVMValueRef args[1] = {arg};
                            LLVMBuildCall2(builder, printf_type, printf_func, args, 1, "");
                        } else {
                            LLVMValueRef fmt_str = LLVMBuildGlobalStringPtr(builder, "%lld\n", "fmt");
                            LLVMValueRef args[2] = {fmt_str, arg};
                            LLVMBuildCall2(builder, printf_type, printf_func, args, 2, "");
                        }
                    }
                }
            }
            break;
        }
        case AST_PRINTN_STMT:
        case AST_PRINTFN_STMT: {
            if (stmt->data.print_stmt.args && stmt->data.print_stmt.arg_count > 0) {
                LLVMValueRef arg = generate_expr(stmt->data.print_stmt.args[0]);
                if (arg) {
                    LLVMValueRef printf_func = LLVMGetNamedFunction(module, "printf");
                    if (printf_func) {
                        LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32TypeInContext(context), (LLVMTypeRef[]){LLVMPointerType(LLVMInt8TypeInContext(context), 0)}, 1, 1);
                        LLVMValueRef fmt_str = LLVMBuildGlobalStringPtr(builder, "%lld\n", "fmt");
                        LLVMValueRef args[2] = {fmt_str, arg};
                        LLVMBuildCall2(builder, printf_type, printf_func, args, 2, "");
                    }
                }
            }
            break;
        }
        case AST_IMPLICIT_PRINT_STMT: {
            if (stmt->data.print_stmt.args && stmt->data.print_stmt.arg_count > 0) {
                LLVMValueRef arg = generate_expr(stmt->data.print_stmt.args[0]);
                if (arg) {
                    LLVMValueRef printf_func = LLVMGetNamedFunction(module, "printf");
                    if (printf_func) {
                        LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32TypeInContext(context), (LLVMTypeRef[]){LLVMPointerType(LLVMInt8TypeInContext(context), 0)}, 1, 1);
                        LLVMValueRef fmt_str = LLVMBuildGlobalStringPtr(builder, "%lld\n", "fmt");
                        LLVMValueRef args[2] = {fmt_str, arg};
                        LLVMBuildCall2(builder, printf_type, printf_func, args, 2, "");
                    }
                }
            }
            break;
        }
        case AST_RETURN_STMT: {
            LLVMValueRef ret_val = generate_expr(stmt->data.return_stmt.expr);
            if (ret_val) {
                LLVMTypeRef val_type = LLVMTypeOf(ret_val);
                if (LLVMGetTypeKind(val_type) == LLVMIntegerTypeKind) {
                    unsigned val_bits = LLVMGetIntTypeWidth(val_type);
                    if (val_bits != 64) {
                        if (val_bits < 64) {
                            ret_val = LLVMBuildSExt(builder, ret_val, LLVMInt64TypeInContext(context), "sext");
                        } else {
                            ret_val = LLVMBuildTrunc(builder, ret_val, LLVMInt64TypeInContext(context), "trunc");
                        }
                    }
                }
                LLVMBuildRet(builder, ret_val);
            } else {
                LLVMBuildRet(builder, LLVMConstInt(LLVMInt64TypeInContext(context), 0, 0));
            }
            return 1;
        }
        case AST_FUNC_CALL: {
            generate_func_call_expr(stmt);
            break;
        }
        case AST_FOR_STMT: {
            LLVMBasicBlockRef loop_cond = LLVMAppendBasicBlockInContext(context, current_function, "loop_cond");
            LLVMBasicBlockRef loop_body = LLVMAppendBasicBlockInContext(context, current_function, "loop_body");
            LLVMBasicBlockRef loop_end = LLVMAppendBasicBlockInContext(context, current_function, "loop_end");
            LLVMBasicBlockRef after_loop = LLVMAppendBasicBlockInContext(context, current_function, "after_loop");

            LLVMBasicBlockRef prev_loop_cond = current_loop_cond;
            LLVMBasicBlockRef prev_loop_end = current_loop_end;
            LLVMBasicBlockRef prev_loop_after = current_loop_after;
            current_loop_cond = loop_cond;
            current_loop_end = loop_end;
            current_loop_after = after_loop;

            if (stmt->data.for_stmt.init) {
                generate_statement(stmt->data.for_stmt.init);
            }
            LLVMBuildBr(builder, loop_cond);

            LLVMPositionBuilderAtEnd(builder, loop_cond);
            LLVMValueRef cond_val = NULL;
            if (stmt->data.for_stmt.condition) {
                cond_val = generate_expr(stmt->data.for_stmt.condition);
            }
            if (!cond_val) {
                cond_val = LLVMConstInt(LLVMInt1TypeInContext(context), 1, 0);
            }
            LLVMBuildCondBr(builder, cond_val, loop_body, loop_end);

            LLVMPositionBuilderAtEnd(builder, loop_body);
            if (stmt->data.for_stmt.body) {
                generate_body(stmt->data.for_stmt.body);
            }
            if (stmt->data.for_stmt.increment) {
                generate_statement(stmt->data.for_stmt.increment);
            }
            LLVMBuildBr(builder, loop_cond);

            LLVMPositionBuilderAtEnd(builder, loop_end);
            LLVMBuildBr(builder, after_loop);

            LLVMPositionBuilderAtEnd(builder, after_loop);

            current_loop_cond = prev_loop_cond;
            current_loop_end = prev_loop_end;
            current_loop_after = prev_loop_after;
            break;
        }
        case AST_FO_STMT: {
            LLVMBasicBlockRef loop_cond = LLVMAppendBasicBlockInContext(context, current_function, "fo_loop_cond");
            LLVMBasicBlockRef loop_body = LLVMAppendBasicBlockInContext(context, current_function, "fo_loop_body");
            LLVMBasicBlockRef loop_end = LLVMAppendBasicBlockInContext(context, current_function, "fo_loop_end");
            LLVMBasicBlockRef after_loop = LLVMAppendBasicBlockInContext(context, current_function, "fo_after_loop");

            LLVMBasicBlockRef prev_loop_cond = current_loop_cond;
            LLVMBasicBlockRef prev_loop_end = current_loop_end;
            LLVMBasicBlockRef prev_loop_after = current_loop_after;
            current_loop_cond = loop_cond;
            current_loop_end = loop_end;
            current_loop_after = after_loop;

            LLVMValueRef index = LLVMBuildAlloca(builder, LLVMInt64TypeInContext(context), "fo_index");
            LLVMValueRef start_val = LLVMConstInt(LLVMInt64TypeInContext(context), 0, 0);
            LLVMBuildStore(builder, start_val, index);

            if (stmt->data.fo_stmt.range_start) {
                start_val = generate_expr(stmt->data.fo_stmt.range_start);
                LLVMBuildStore(builder, start_val, index);
            }

            LLVMBuildBr(builder, loop_cond);

            LLVMPositionBuilderAtEnd(builder, loop_cond);
            LLVMValueRef current_idx = LLVMBuildLoad2(builder, LLVMInt64TypeInContext(context), index, "");
            
            LLVMValueRef end_val;
            if (stmt->data.fo_stmt.range_end) {
                end_val = generate_expr(stmt->data.fo_stmt.range_end);
            } else {
                end_val = LLVMConstInt(LLVMInt64TypeInContext(context), 100, 0);
            }
            
            LLVMValueRef cond_val = LLVMBuildICmp(builder, LLVMIntSLT, current_idx, end_val, "fo_cond");
            LLVMBuildCondBr(builder, cond_val, loop_body, loop_end);

            LLVMPositionBuilderAtEnd(builder, loop_body);
            if (stmt->data.fo_stmt.var_name) {
                LLVMValueRef var_alloca = LLVMBuildAlloca(builder, LLVMInt64TypeInContext(context), stmt->data.fo_stmt.var_name);
                local_var_add(stmt->data.fo_stmt.var_name, var_alloca, VAL_INT64);
                LLVMBuildStore(builder, current_idx, var_alloca);
            }
            if (stmt->data.fo_stmt.body) {
                generate_body(stmt->data.fo_stmt.body);
            }
            
            LLVMValueRef next_idx = LLVMBuildAdd(builder, current_idx, LLVMConstInt(LLVMInt64TypeInContext(context), 1, 0), "fo_next");
            LLVMBuildStore(builder, next_idx, index);
            LLVMBuildBr(builder, loop_cond);

            LLVMPositionBuilderAtEnd(builder, loop_end);
            LLVMBuildBr(builder, after_loop);

            LLVMPositionBuilderAtEnd(builder, after_loop);

            current_loop_cond = prev_loop_cond;
            current_loop_end = prev_loop_end;
            current_loop_after = prev_loop_after;
            break;
        }
        case AST_WHILE_STMT: {
            LLVMBasicBlockRef loop_cond = LLVMAppendBasicBlockInContext(context, current_function, "while_cond");
            LLVMBasicBlockRef loop_body = LLVMAppendBasicBlockInContext(context, current_function, "while_body");
            LLVMBasicBlockRef loop_end = LLVMAppendBasicBlockInContext(context, current_function, "while_end");
            LLVMBasicBlockRef after_loop = LLVMAppendBasicBlockInContext(context, current_function, "while_after_loop");

            LLVMBasicBlockRef prev_loop_cond = current_loop_cond;
            LLVMBasicBlockRef prev_loop_end = current_loop_end;
            LLVMBasicBlockRef prev_loop_after = current_loop_after;
            current_loop_cond = loop_cond;
            current_loop_end = loop_end;
            current_loop_after = after_loop;

            LLVMBuildBr(builder, loop_cond);

            LLVMPositionBuilderAtEnd(builder, loop_cond);
            LLVMValueRef cond_val = NULL;
            if (stmt->data.while_stmt.condition) {
                cond_val = generate_expr(stmt->data.while_stmt.condition);
            }
            if (!cond_val) {
                cond_val = LLVMConstInt(LLVMInt1TypeInContext(context), 1, 0);
            }
            LLVMBuildCondBr(builder, cond_val, loop_body, loop_end);

            LLVMPositionBuilderAtEnd(builder, loop_body);
            if (stmt->data.while_stmt.body) {
                generate_body(stmt->data.while_stmt.body);
            }
            
            if (stmt->data.while_stmt.has_quit && stmt->data.while_stmt.quit_condition) {
                LLVMValueRef quit_cond = generate_expr(stmt->data.while_stmt.quit_condition);
                LLVMBuildCondBr(builder, quit_cond, loop_end, loop_cond);
            } else {
                LLVMBuildBr(builder, loop_cond);
            }

            LLVMPositionBuilderAtEnd(builder, loop_end);
            LLVMBuildBr(builder, after_loop);

            LLVMPositionBuilderAtEnd(builder, after_loop);

            current_loop_cond = prev_loop_cond;
            current_loop_end = prev_loop_end;
            current_loop_after = prev_loop_after;
            break;
        }
        case AST_BREAK_STMT: {
            if (current_loop_end) {
                LLVMBuildBr(builder, current_loop_end);
            }
            break;
        }
        case AST_CONTINUE_STMT: {
            if (current_loop_cond) {
                LLVMBuildBr(builder, current_loop_cond);
                LLVMBasicBlockRef dead_block = LLVMAppendBasicBlockInContext(context, current_function, "continue_dead");
                LLVMPositionBuilderAtEnd(builder, dead_block);
            }
            break;
        }
        case AST_QUIT_STMT: {
            if (current_loop_after) {
                LLVMBuildBr(builder, current_loop_after);
                LLVMBasicBlockRef dead_block = LLVMAppendBasicBlockInContext(context, current_function, "quit_dead");
                LLVMPositionBuilderAtEnd(builder, dead_block);
            }
            break;
        }
        case AST_USE_STMT: {
            break;
        }
    }
    return 0;
}

static int generate_body(ASTNode* body) {
    int has_return = 0;
    int prev_in_function = in_function_body;
    in_function_body = 1;
    while (body) {
        if (generate_statement(body)) {
            has_return = 1;
        }
        body = body->next;
    }
    in_function_body = prev_in_function;
    return has_return;
}

static void generate_module_functions(AST* ast) {
    LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32TypeInContext(context),
        (LLVMTypeRef[]){LLVMPointerType(LLVMInt8TypeInContext(context), 0)}, 1, 1);
    LLVMAddFunction(module, "printf", printf_type);

    ASTNode* current = ast->head;
    
    while (current) {
        if (current->type == AST_VAR_DECL) {
            LLVMTypeRef llvm_type = get_llvm_type(current->data.var_decl.var_type);
            LLVMValueRef var = LLVMAddGlobal(module, llvm_type, current->data.var_decl.name);
            global_var_add(current->data.var_decl.name, current->data.var_decl.var_type);
            LLVMSetAlignment(var, 8);
            if (current->data.var_decl.initializer && current->data.var_decl.initializer->type == AST_LITERAL) {
                LLVMValueRef init_val = generate_expr(current->data.var_decl.initializer);
                if (init_val) {
                    LLVMSetInitializer(var, init_val);
                } else {
                    LLVMSetInitializer(var, LLVMConstInt(llvm_type, 0, 0));
                }
            } else {
                LLVMSetInitializer(var, LLVMConstInt(llvm_type, 0, 0));
            }
        }
        current = current->next;
    }

    current = ast->head;
    char current_module[64] = "";

    while (current) {
        if (current->type == AST_MODULE_DECL) {
            strcpy(current_module, current->data.module_decl.name);
        } else if (current->type == AST_FUNC_DECL && strcmp(current->data.func_decl.name, "main") != 0) {
            LLVMTypeRef* param_types = NULL;
            if (current->data.func_decl.param_count > 0) {
                param_types = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * current->data.func_decl.param_count);
                for (int i = 0; i < current->data.func_decl.param_count; i++) {
                    param_types[i] = LLVMInt64TypeInContext(context);
                }
            }

            char func_name[128] = "";
            if (strlen(current_module) > 0) {
                snprintf(func_name, sizeof(func_name), "%s_%s", current_module, current->data.func_decl.name);
            } else {
                strcpy(func_name, current->data.func_decl.name);
            }

            LLVMTypeRef func_type = LLVMFunctionType(LLVMInt64TypeInContext(context), param_types, current->data.func_decl.param_count, 0);
            current_function = LLVMAddFunction(module, func_name, func_type);

            LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context, current_function, "entry");
            LLVMPositionBuilderAtEnd(builder, entry);

            local_vars_clear();
            in_function_body = 1;

            if (current->data.func_decl.param_count > 0 && current->data.func_decl.params) {
                for (int i = 0; i < current->data.func_decl.param_count; i++) {
                    const char* param_name = current->data.func_decl.params[i].name;
                    if (param_name) {
                        LLVMValueRef param_val = LLVMGetParam(current_function, i);
                        LLVMValueRef alloca = LLVMBuildAlloca(builder, LLVMInt64TypeInContext(context), param_name);
                        local_var_add(param_name, alloca, VAL_INT64);
                        LLVMBuildStore(builder, param_val, alloca);
                    }
                }
            }

            generate_body(current->data.func_decl.body);
            in_function_body = 0;
            
            if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(builder)) == NULL) {
                LLVMBuildRet(builder, LLVMConstInt(LLVMInt64TypeInContext(context), 0, 0));
            }
            
            if (param_types) free(param_types);
        }
        current = current->next;
    }
}

static void generate_main_function(AST* ast) {
    ASTNode* current = ast->head;
    ASTNode* explicit_main = NULL;

    while (current) {
        if (current->type == AST_FUNC_DECL && strcmp(current->data.func_decl.name, "main") == 0) {
            explicit_main = current;
            break;
        }
        current = current->next;
    }

    LLVMTypeRef main_type = LLVMFunctionType(LLVMInt64TypeInContext(context), NULL, 0, 0);
    current_function = LLVMAddFunction(module, "main", main_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context, current_function, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    if (explicit_main) {
        local_vars_clear();
        in_function_body = 1;
        generate_body(explicit_main->data.func_decl.body);
        in_function_body = 0;
    } else {
        ASTNode* global_init = ast->head;
        while (global_init) {
            if (global_init->type == AST_VAR_DECL && global_init->data.var_decl.initializer && 
                global_init->data.var_decl.initializer->type != AST_LITERAL) {
                LLVMValueRef var = LLVMGetNamedGlobal(module, global_init->data.var_decl.name);
                if (var) {
                    LLVMValueRef init_val = generate_expr(global_init->data.var_decl.initializer);
                    if (init_val) {
                        LLVMBuildStore(builder, init_val, var);
                    }
                }
            }
            global_init = global_init->next;
        }
        current = ast->head;
        while (current) {
            if (current->type != AST_FUNC_DECL) {
                generate_statement(current);
            }
            current = current->next;
        }
    }

    LLVMBasicBlockRef current_bb = LLVMGetInsertBlock(builder);
    if (current_bb && LLVMGetBasicBlockTerminator(current_bb) == NULL) {
        LLVMBuildRet(builder, LLVMConstInt(LLVMInt64TypeInContext(context), 0, 0));
    } else if (!current_bb) {
        LLVMPositionBuilderAtEnd(builder, entry);
        if (LLVMGetBasicBlockTerminator(entry) == NULL) {
            LLVMBuildRet(builder, LLVMConstInt(LLVMInt64TypeInContext(context), 0, 0));
        }
    }
}

static int llvm_initialized = 0;

static void llvm_initialize_once() {
    if (!llvm_initialized) {
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();
        LLVMInitializeNativeAsmParser();
        llvm_initialized = 1;
    }
}

int codegen_compile(AST* ast, const char* output_filename) {
    if (!ast || !ast->head) {
        fprintf(stderr, "Error: Empty AST\n");
        return 1;
    }

    llvm_initialize_once();

    context = LLVMContextCreate();
    module = LLVMModuleCreateWithNameInContext("freq_program", context);
    builder = LLVMCreateBuilderInContext(context);

    generate_module_functions(ast);
    generate_main_function(ast);

    char* error = NULL;
    LLVMTargetRef target = NULL;
    if (LLVMGetTargetFromTriple(LLVMGetDefaultTargetTriple(), &target, &error) != 0) {
        fprintf(stderr, "Error getting target: %s\n", error);
        LLVMDisposeMessage(error);
        LLVMDisposeBuilder(builder);
        LLVMContextDispose(context);
        return 1;
    }

    LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(
        target,
        LLVMGetDefaultTargetTriple(),
        LLVMGetHostCPUName(),
        LLVMGetHostCPUFeatures(),
        LLVMCodeGenLevelAggressive,
        LLVMRelocDefault,
        LLVMCodeModelDefault
    );

    if (!target_machine) {
        fprintf(stderr, "Error creating target machine\n");
        LLVMDisposeBuilder(builder);
        LLVMContextDispose(context);
        return 1;
    }

    LLVMPassManagerRef pass_manager = LLVMCreatePassManager();
    LLVMAddAnalysisPasses(target_machine, pass_manager);
    LLVMRunPassManager(pass_manager, module);
    LLVMDisposePassManager(pass_manager);

    char ir_filename[256];
    snprintf(ir_filename, sizeof(ir_filename), "/tmp/freq_%d.ll", getpid());

    if (LLVMPrintModuleToFile(module, ir_filename, &error) != 0) {
        fprintf(stderr, "Error writing IR file: %s\n", error);
        LLVMDisposeMessage(error);
        LLVMDisposeTargetMachine(target_machine);
        LLVMDisposeBuilder(builder);
        LLVMContextDispose(context);
        return 1;
    }

    LLVMDisposeTargetMachine(target_machine);
    LLVMDisposeBuilder(builder);
    LLVMContextDispose(context);

    char compile_cmd[512];
    snprintf(compile_cmd, sizeof(compile_cmd),
        "clang -O2 -o %s %s", output_filename, ir_filename);

    int ret = system(compile_cmd);
    remove(ir_filename);

    if (ret != 0) {
        fprintf(stderr, "Compilation failed with code %d\n", ret);
        return 1;
    }

    return 0;
}

double codegen_jit_run(AST* ast, int* success) {
    if (!ast || !ast->head) {
        fprintf(stderr, "Error: Empty AST\n");
        *success = 0; return 0.0;
    }

    llvm_initialize_once();

    context = LLVMContextCreate();
    module = LLVMModuleCreateWithNameInContext("freq_program", context);
    builder = LLVMCreateBuilderInContext(context);

    generate_module_functions(ast);
    generate_main_function(ast);

    char* error = NULL;
    LLVMExecutionEngineRef engine;
    if (LLVMCreateExecutionEngineForModule(&engine, module, &error) != 0) {
        fprintf(stderr, "Error creating execution engine: %s\n", error);
        LLVMDisposeMessage(error);
        LLVMDisposeBuilder(builder);
        LLVMContextDispose(context);
        *success = 0; return 0.0;
    }

    LLVMValueRef main_func = LLVMGetNamedFunction(module, "main");
    double exec_ms = 0.0;
    if (main_func) {
        struct timeval tv_start, tv_end;
        gettimeofday(&tv_start, NULL);
        LLVMRunFunctionAsMain(engine, main_func, 0, NULL, NULL);
        gettimeofday(&tv_end, NULL);
        exec_ms = (tv_end.tv_sec - tv_start.tv_sec) * 1000.0 + (tv_end.tv_usec - tv_start.tv_usec) / 1000.0;
        fflush(stdout);
    }

    LLVMDisposeExecutionEngine(engine);
    LLVMDisposeBuilder(builder);
    LLVMContextDispose(context);

    *success = 1;
    return exec_ms;
}