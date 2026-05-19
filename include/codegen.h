#ifndef FREQ_CODEGEN_H
#define FREQ_CODEGEN_H

#include "common.h"

int codegen_compile(AST* ast, const char* output_filename);
double codegen_jit_run(AST* ast, int* success);

#endif