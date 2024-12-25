#include "readline/readline.h"
#include "stdlib.h"
#include <unistd.h>
#include "memory.h"
#include "native.h"
#include "string.h"
#include "limits.h"
#include "vm.h"

#define REPL_FILE_NAME "LOX_REPL"

bool COMPILE_ONLY = false;
bool RUN_BYTECODE = false;
char OUTPUT_PATH[100];
bool REPL = false;
bool preload_finished = false;
bool preload_started = false;
bool SHOW_COMPILE_RESULT = false;
bool TRACE_EXECUTION = false;
int TRACE_SKIP = -1;
bool SHOW_LABEL = false;
jmp_buf consume_buf;

static void print_result_with_color(InterpretResult result) {
    switch (result) {
        case INTERPRET_COMPILE_ERROR:
            start_color(BOLD_MAGENTA);
            printf("== compile error ==\n");
            break;
        case INTERPRET_RUNTIME_ERROR:
            start_color(RED);
            printf("== runtime error ==\n");
            break;
        case INTERPRET_PRODUCE_ERROR:
            start_color(RED);
            printf("== produce error ==\n");
            break;
        case INTERPRET_READ_ERROR:
            start_color(RED);
            printf("== file reading error");
            break;
        case INTERPRET_OK:
        case INTERPRET_REPL_EXIT:
            start_color(GREEN);
            printf("== execution finished ==\n");
    }
    end_color();
}

static void repl() {

    DISABLE_GC;

    REPL = true;

    FILE *file = fopen(REPL_FILE_NAME, "w");

    fprintf(file, "// This file is generated by the clox REPL. It will be overwritten by the next REPL. \n\n");

    char repl_path[PATH_MAX];

    getcwd(repl_path, sizeof(repl_path));
    if (getcwd(repl_path, sizeof(repl_path)) == NULL) {
        perror("getcwd() error");
        exit(1);
    }

    int len = strlen(repl_path) + strlen(REPL_FILE_NAME) + 1; // +1 is for the '/'
    memcpy(repl_path + strlen(repl_path), "/"REPL_FILE_NAME, len + 1); // +1 to copy the '/0'
    String *path_string = string_copy(repl_path,  len);
    repl_module = new_module(path_string);

    additional_repl_init();
    printf("You are in the clox REPL mode. Type help() for more information, exit() to exit. \nYour compiled input will be saved in the file LOX_REPL.\n\n");

    ENABLE_GC;

    while (true) {
        char *line;
        if (setjmp(consume_buf) == 0 ) {
            line = readline("> ");
            execute:
            if (line != NULL) {
                InterpretResult result = interpret(line, NULL);
                if (result == INTERPRET_REPL_EXIT) {
                    fclose(file);
                    exit(0);
                }
                add_history(line);
                if (result != INTERPRET_COMPILE_ERROR) {
                    fprintf(file, "%s\n\n", line);
                    fflush(file);
                }
                free(line);
            } else {
                // EOF, exit
                NEW_LINE();
                break;
            }

        } else {
            // if consume() views the input as not finished
            char *extra_line = readline("... ");
            char *temp;
            asprintf(&temp, "%s\n%s", line, extra_line);
            free(line);
            free(extra_line);
            line = temp;
            goto execute;
        }
    }
    fclose(file);
}

char *resolve_path(const char *path) {
    static char buffer[PATH_MAX];
    return realpath(path, buffer);
}

char *get_filename(char *path) {
    char *separator = strrchr(path, '/');
    if (separator == NULL) {
        return path;
    } else {
        return separator + 1;
    }
}

char *read_file(const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return NULL;
    }
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
    if (src == NULL) {
        printf("error when opening file %s\n", path);
        exit(1);
    }
    InterpretResult result = interpret(src, path);
    free(src);
#ifdef COLOR_RUN_FILE_RESULT
    print_result_with_color(result);
#endif
}

static void produce_bytecode(const char *code_path, const char *result_path) {
    char *src = read_file(code_path);
    if (src == NULL) {
        printf("error when opening file %s\n", code_path);
        exit(1);
    }
    InterpretResult result = produce(src, result_path);
    free(src);
#ifdef COLOR_RUN_FILE_RESULT
    print_result_with_color(result);
#endif
}

static void main_run_bytecode(const char *code_path) {

    InterpretResult result = read_run_bytecode(code_path);
#ifdef COLOR_RUN_FILE_RESULT
    print_result_with_color(result);
#endif
}

int main(int argc, char *const argv[]) {

    init_VM();
#ifdef JUST_SCRIPT
    run_file(argv[1]);
    return 0;
#endif
    char *options = "dlsc:bh";
    int op;
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
