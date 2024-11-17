#include "chunk.h"
#include "vm.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"


static void repl() {
    char line[1024];
    while (true) {
        printf("> ");
        if (fgets(line, sizeof(line), stdin)) {
            interpret(line);
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

int main(int argc, const char **argv) {
    init_VM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        run_file(argv[1]);
    } else {
        fprintf(stderr, "no arg to start REPL, one arg to run a file!\n");
    }

//    Chunk chunk;
//    init_chunk(&chunk);
//
//    write_chunk(&chunk, OP_CONSTANT, 999);
//    int c1 = add_constant(&chunk, 1);
//    write_chunk(&chunk, c1, 999);
//
//    write_chunk(&chunk, OP_CONSTANT, 999);
//    int c2 = add_constant(&chunk, 2);
//    write_chunk(&chunk, c2, 999);
//
//    write_chunk(&chunk, OP_CONSTANT, 999);
//    int c3 = add_constant(&chunk, 3);
//    write_chunk(&chunk, c3, 999);
//
//    write_chunk(&chunk, OP_MULTIPLY, 999);
//
//    write_chunk(&chunk, OP_ADD, 999);
//
//    write_chunk(&chunk, OP_CONSTANT, 999);
//    int c4 = add_constant(&chunk, 4);
//    write_chunk(&chunk, c4, 999);
//
//    write_chunk(&chunk, OP_CONSTANT, 999);
//    int c5 = add_constant(&chunk, 5);
//    write_chunk(&chunk, c5, 999);
//
//    write_chunk(&chunk, OP_NEGATE, 999);
//
//    write_chunk(&chunk, OP_DIVIDE, 999);
//
//    write_chunk(&chunk, OP_SUBTRACT, 999);
//
//    write_chunk(&chunk, OP_RETURN, 999);
//
//    interpret(&chunk);
//
//    free_chunk(&chunk);
//    printf("\n");

    free_VM();
}

