#include "readline/readline.h"
#include "stdlib.h"
#include <unistd.h>
#include "string.h"
#include "vm.h"

bool COMPILE_ONLY = false;
bool RUN_BYTECODE = false;
char OUTPUT_PATH[100];
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

static void produce_bytecode(const char *code_path, const char *result_path) {
    char *src = read_file(code_path);
    InterpretResult result = produce(src, result_path);
    free(src);
    if (result == INTERPRET_COMPILE_ERROR) {
        printf("== compile error ==\n");
    } else if (result == INTERPRET_RUNTIME_ERROR) {
        printf("== runtime error ==\n");
    } else {
        printf("== produce finished ==\n");
    }
}

static void main_run_bytecode(const char *code_path) {

    InterpretResult result = read_run_bytecode(code_path);
    if (result == INTERPRET_COMPILE_ERROR) {
        printf("== compile error ==\n");
    } else if (result == INTERPRET_RUNTIME_ERROR) {
        printf("== runtime error ==\n");
    } else if (result == INTERPRET_READ_ERROR){
        printf("== io error ==\n");
    } else {
        printf("== execution finished ==\n");
    }
}

/**
 * clox
 * clox file
 * */
int main(int argc, char *const argv[]) {

    init_VM();
    char *options = "dlsc:bh";
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
            case 'c':
                COMPILE_ONLY = true;
                strcpy(OUTPUT_PATH, optarg);
                break;
            case 'b':
                RUN_BYTECODE = true;
                break;
            case 'h':
            default:
                printf("Options: \n");
                printf("-s: show the compile result\n");
                printf("-d: trace the execution\n");
                printf("-c path/to/output: compile and write and result to the specified path\n");
                printf("-b: treat the given file as bytecode\n");
                exit(1);
        }
    }
    if (optind < argc) {
        if (COMPILE_ONLY) {
            produce_bytecode(argv[optind], OUTPUT_PATH);
        } else if (RUN_BYTECODE) {
            main_run_bytecode(argv[optind]);
        } else {
            run_file(argv[optind]);
        }
    } else {
        if (COMPILE_ONLY) {
            printf("The output path is not specified\n");
            printf("The typical format of compiling is `clox -c output/path script/path`\n");
            exit(1);
        } else {
            repl();
        }
    }

    free_VM();
}
