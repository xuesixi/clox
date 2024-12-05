#include "chunk.h"
#include "readline/readline.h"
#include "stdlib.h"
#include <unistd.h>
#include "string.h"
#include "vm.h"

bool REPL;
bool SHOW_COMPILE_RESULT = false;
bool TRACE_EXECUTION = false;
int TRACE_SKIP = -1;
bool SHOW_LABEL = false;
jmp_buf consume_buf;

static void repl() {
    REPL = true;
    additional_repl_init();
    printf("You are in the clox REPL mode. Type help() for more information\n\n");
    while (true) {
        char *line;
        if (setjmp(consume_buf) == 0 ) {
            line = readline("> ");
            execute:
            if (line != NULL) {
                interpret(line);
                add_history(line);
                free(line);
            } else {
                NEW_LINE();
                break;
            }

        } else {
            // if consume() views the input as not finished
            char *extra_line = readline("... ");
            char *temp;
            asprintf(&temp, "%s%s", line, extra_line);
            free(line);
            free(extra_line);
            line = temp;
            goto execute;
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
int main(int argc, char *const argv[]) {

    init_VM();
    char *options = "dls";
    char op;
    while ((op = getopt(argc, argv, options)) != -1) {
        switch (op) {
            case 'd':
                TRACE_EXECUTION = true;
                break;
            case 's':
                SHOW_COMPILE_RESULT = true;
                break;
            case 'l':
                SHOW_LABEL = true;
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
