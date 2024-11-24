#include "chunk.h"
#include "vm.h"
#include "stdlib.h"
#include "readline/readline.h"

bool REPL;

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
        printf("compile error\n");
    } else if (result == INTERPRET_RUNTIME_ERROR) {
        printf("runtime error\n");
    } else {
        printf("good\n");
    }
}

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

//static void run_bytecode(const char *bytecode_path) {
//    FILE *file = fopen(bytecode_path, "rb");
//}

int main(int argc, const char **argv) {
    init_VM();
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        run_file(argv[1]);
    } else if(argc == 3) {
        produce_bytecode(argv[1], argv[2]);
    } else {
        fprintf(stderr, "no arg to start REPL, one arg to run a file!\n");
    }

    free_VM();
}

