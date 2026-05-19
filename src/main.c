#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define getpid() GetCurrentProcessId()
#else
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif


static double get_time_ms() {
#ifdef _WIN32
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)freq.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
#endif
}
static int has_warning = 0;

static int has_fq_extension(const char* filename) {
    if (strcmp(filename, "/dev/stdin") == 0) {
        return 1;
    }
    size_t len = strlen(filename);
    if (len < 3) return 0;
    return strcmp(filename + len - 3, ".fq") == 0;
}

static char* read_file(const char* filename) {
    FILE* fp = NULL;

    if (strcmp(filename, "/dev/stdin") == 0) {
        fp = stdin;
    } else {
        fp = fopen(filename, "r");
        if (!fp) {
            fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
            return NULL;
        }
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* content = (char*)malloc(size + 1);
    if (!content) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        if (fp != stdin) fclose(fp);
        return NULL;
    }

    fread(content, 1, size, fp);
    content[size] = '\0';

    if (fp != stdin) fclose(fp);
    return content;
}

static char* resolve_import(const char* module_name, const char* current_dir) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s.fq", current_dir, module_name);
    FILE* fp = fopen(filepath, "r");
    if (fp) {
        fclose(fp);
        return strdup(filepath);
    }
    snprintf(filepath, sizeof(filepath), "%s/%s/hh.fq", current_dir, module_name);
    fp = fopen(filepath, "r");
    if (fp) {
        fclose(fp);
        return strdup(filepath);
    }
    snprintf(filepath, sizeof(filepath), "%s/%s/hh.pls", current_dir, module_name);
    fp = fopen(filepath, "r");
    if (fp) {
        fclose(fp);
        return strdup(filepath);
    }
    return NULL;
}

static int matches_exported_var(const char* line, const char* var_name) {
    const char* p = line;
    while (*p == ' ' || *p == '\t') p++;
    
    if (strncmp(p, "out ", 4) == 0) {
        p += 4;
        while (*p == ' ' || *p == '\t') p++;
        while (*p && *p != ' ' && *p != '\t' && *p != '=' && *p != ':') p++;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '=') return 0;
        char name[256];
        int i = 0;
        while (*p && *p != ' ' && *p != '\t' && *p != '=' && *p != ':' && *p != '\n' && *p != '\r' && i < 255) {
            name[i++] = *p++;
        }
        name[i] = '\0';
        return strcmp(name, var_name) == 0;
    }
    
    if (strncmp(p, "func ", 5) == 0) {
        p += 5;
        while (*p == ' ' || *p == '\t') p++;
        char name[256];
        int i = 0;
        while (*p && *p != ' ' && *p != '\t' && *p != '(' && *p != '\n' && *p != '\r' && i < 255) {
            name[i++] = *p++;
        }
        name[i] = '\0';
        char func_name_with_paren[256];
        snprintf(func_name_with_paren, sizeof(func_name_with_paren), "%s()", name);
        return strcmp(func_name_with_paren, var_name) == 0 || strcmp(name, var_name) == 0;
    }
    
    return 0;
}

static char* extract_exported_var(const char* module_name, const char* var_name, const char* current_dir) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s.fq", current_dir, module_name);
    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        snprintf(filepath, sizeof(filepath), "%s/%s/hh.fq", current_dir, module_name);
        fp = fopen(filepath, "r");
    }
    if (!fp) {
        snprintf(filepath, sizeof(filepath), "%s/%s/hh.pls", current_dir, module_name);
        fp = fopen(filepath, "r");
    }
    if (!fp) return NULL;

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (matches_exported_var(line, var_name)) {
            char* result = malloc(8192);
            result[0] = '\0';
            strcat(result, line);
            
            if (strstr(line, "func ") != NULL) {
                int brace_count = 0;
                const char* p = line;
                while (*p) {
                    if (*p == '{') brace_count++;
                    if (*p == '}') brace_count--;
                    p++;
                }
                
                while (brace_count > 0 && fgets(line, sizeof(line), fp)) {
                    strcat(result, line);
                    const char* q = line;
                    while (*q) {
                        if (*q == '{') brace_count++;
                        if (*q == '}') brace_count--;
                        q++;
                    }
                }
            }
            
            fclose(fp);
            return result;
        }
    }
    fclose(fp);
    return NULL;
}

static char* get_dir_from_path(const char* filepath) {
    char* last_slash = strrchr(filepath, '/');
    if (last_slash) {
        size_t len = last_slash - filepath;
        char* dir = (char*)malloc(len + 1);
        strncpy(dir, filepath, len);
        dir[len] = '\0';
        return dir;
    }
    return strdup(".");
}

static char* preprocess_imports(const char* source, const char* filepath) {
    char* dir = get_dir_from_path(filepath);

    size_t max_bufsize = strlen(source) * 2 + 1024;
    char* result = (char*)malloc(max_bufsize);
    result[0] = '\0';

    const char* current = source;
    char line[1024];

    while (*current) {
        const char* line_start = current;
        const char* line_end = strchr(current, '\n');
        if (!line_end) {
            line_end = current + strlen(current);
        }

        size_t line_len = line_end - line_start;
        if (line_len >= sizeof(line)) line_len = sizeof(line) - 1;

        strncpy(line, line_start, line_len);
        line[line_len] = '\0';

        char* comment = strstr(line, "//");
        if (comment) {
            *comment = '\0';
        }

        if (strncmp(line, "import ", 7) == 0) {
            char* module_name = line + 7;
            while (*module_name == ' ' || *module_name == '\t') module_name++;
            char* end = module_name + strlen(module_name) - 1;
            while (end > module_name && (*end == '\n' || *end == '\r' || *end == ' ' || *end == '\t')) *end-- = '\0';

            if (strlen(module_name) > 0) {
                char* import_path = resolve_import(module_name, dir);
                if (import_path) {
                    char* import_content = read_file(import_path);
                    if (import_content) {
                        strcat(result, "// Begin import ");
                        strcat(result, module_name);
                        strcat(result, "\n");
                        strcat(result, import_content);
                        strcat(result, "\n// End import ");
                        strcat(result, module_name);
                        strcat(result, "\n");
                        free(import_content);
                    }
                    free(import_path);
                } else {
                    fprintf(stderr, "Warning: Could not resolve import '%s'\n", module_name);
                    has_warning = 1;
                }
            }
        } else if (strncmp(line, "use ", 4) == 0) {
            char* use_expr = line + 4;
            while (*use_expr == ' ' || *use_expr == '\t') use_expr++;
            char* double_colon = strstr(use_expr, "::");
            if (double_colon) {
                *double_colon = '\0';
                char* module_name = use_expr;
                char* var_name = double_colon + 2;
                while (*var_name == ' ' || *var_name == '\t') var_name++;
                char* end = var_name + strlen(var_name) - 1;
                while (end > var_name && (*end == '\n' || *end == '\r' || *end == ' ' || *end == '\t')) *end-- = '\0';
                
                if (strlen(module_name) > 0 && strlen(var_name) > 0) {
                    char* var_decl = extract_exported_var(module_name, var_name, dir);
                    if (var_decl) {
                        strcat(result, "// Begin use ");
                        strcat(result, module_name);
                        strcat(result, "::");
                        strcat(result, var_name);
                        strcat(result, "\n");
                        strcat(result, var_decl);
                        strcat(result, "\n// End use ");
                        strcat(result, module_name);
                        strcat(result, "::");
                        strcat(result, var_name);
                        strcat(result, "\n");
                        free(var_decl);
                    } else {
                        fprintf(stderr, "Warning: Could not find exported variable '%s' in module '%s'\n", var_name, module_name);
                        has_warning = 1;
                    }
                }
            } else {
                strcat(result, line);
                strcat(result, "\n");
            }
        } else {
            strcat(result, line);
            strcat(result, "\n");
        }

        current = line_end;
        if (*current == '\n') current++;
    }

    free(dir);
    return result;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s <file.fq> [-o output]     - Compile to executable using LLVM\n", argv[0]);
        fprintf(stderr, "  %s -r <file.fq>             - Run directly with JIT\n", argv[0]);
        return 1;
    }
    
    char* filename = NULL;
    char* output_filename = NULL;
    int run_directly = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_filename = argv[i + 1];
                i++;
            } else {
                fprintf(stderr, "Error: Missing output filename after -o\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-r") == 0) {
            run_directly = 1;
        } else if (filename == NULL) {
            filename = argv[i];
        } else {
            fprintf(stderr, "Error: Unknown argument '%s'\n", argv[i]);
            return 1;
        }
    }
    
    if (filename == NULL) {
        fprintf(stderr, "Error: Missing input file\n");
        return 1;
    }
    
    if (!has_fq_extension(filename)) {
        fprintf(stderr, "Error: Only .fq files are supported\n");
        return 1;
    }
    
    if (!output_filename) {
        output_filename = "a.out";
    }
    
    double start_time = get_time_ms();

    char* source = read_file(filename);
    if (!source) {
        return 1;
    }

    char* preprocessed = preprocess_imports(source, filename);
    free(source);
    source = preprocessed;

    Lexer* lexer = lexer_new(source);
    Parser* parser = parser_new(lexer);
    AST* ast = parser_parse(parser);
    
    if (parser->has_error) {
        ast_free(ast);
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        return 1;
    }
    
    int success;
    double parse_time = get_time_ms() - start_time;
    if (run_directly) {
        fprintf(stderr, "Running %s with JIT...\n", filename);
        double jit_start = get_time_ms();
        double run_time = codegen_jit_run(ast, &success);
        double compile_time = get_time_ms() - jit_start - run_time;
        if (success) {
            if (!has_warning) {
                fprintf(stderr, "\n");
                fprintf(stderr, "  Run: %.4f ms\n", run_time);
                fprintf(stderr, "  Parse: %.4f ms\n", parse_time);
                fprintf(stderr, "  Compile: %.4f ms\n", compile_time);
                fprintf(stderr, "  Total: %.4f ms\n", parse_time + compile_time + run_time);
            }
        } else {
            fprintf(stderr, "Error: Execution failed\n");
        }
    } else {
        fprintf(stderr, "Compiling %s to %s...\n", filename, output_filename);
        int ret = codegen_compile(ast, output_filename);
        if (ret == 0) {
            fprintf(stderr, "Successfully generated executable '%s'\n", output_filename);
            fprintf(stderr, "Compilation completed in %.4f ms\n", get_time_ms() - start_time);
        } else {
            fprintf(stderr, "Error: Compilation failed\n");
        }
        ast_free(ast);
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        return ret == 0 ? 0 : 1;
    }
    
    ast_free(ast);
    parser_free(parser);
    lexer_free(lexer);
    free(source);
    
    return success ? 0 : 1;
}