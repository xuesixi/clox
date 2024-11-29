#include "chunk.h"
#include "readline/readline.h"
#include "stdlib.h"
#include <unistd.h>
#include "vm.h"

bool REPL;
bool SHOW_COMPILE_RESULT = false;
bool TRACE_EXECUTION = false;

static void repl() {
    REPL = true;
    while (true) {
        char *line = readline("> ");
        if (line != NULL) {
            interpret(line);
            add_history(line);
            free(line);
        } else {
            printf("\n");
            break;
        }
    }
}

static char *read_file(const char *path) {
    FILE *file = fopen(path, "r");
    assert(file != NULL);

    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char *buffer = malloc(size + 1);
    assert(buffer != NULL);
    size_t read_size = fread(buffer, 1, size, file);
    buffer[read_size] = '\0';

    fclose(file);

    return buffer;
}

static void run_file(const char *path) {
    char *src = read_file(path);
    InterpretResult result = interpret(src);
    free(src);
    if (result == INTERPRET_COMPILE_ERROR) {
        printf("== compile error ==\n");
    } else if (result == INTERPRET_RUNTIME_ERROR) {
        printf("== runtime error ==\n");
    } else {
        printf("== execution finished ==\n");
    }
}

/*
static void produce_bytecode(const char *code_path, const char *result_path) {
    char *src = read_file(code_path);
    InterpretResult result = produce(src, result_path);
    free(src);
    if (result == INTERPRET_COMPILE_ERROR) {
        printf("compile error\n");
    } else if (result == INTERPRET_RUNTIME_ERROR) {
        printf("runtime error\n");
    } else {
        printf("good\n");
    }
}
*/

// static void run_bytecode(const char *bytecode_path) {
//     FILE *file = fopen(bytecode_path, "rb");
// }

/**
 * clox
 * clox file
 * */
int main(int argc, char * const argv[]) {

    init_VM();
    char *options = "ds";
    char op;
    while ( (op = getopt(argc, argv, options)) != -1) {
        switch (op) {
        case 'd':
            TRACE_EXECUTION = true;
            break;
        case 's':
            SHOW_COMPILE_RESULT = true;
            break;
        default:
            printf("Invalid option %c\n", op);
            printf("-s: show the compile result\n");
            printf("-d: trace the execution\n");
        }
     } 
     if (optind < argc) {
        run_file(argv[optind]);
     } else {
        repl();
     }

    free_VM();
}
