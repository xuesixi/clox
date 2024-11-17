
#include "chunk.h"
#include "vm.h"
#include "stdio.h"

int main(int argc, const char **argv) {
    /**
     *
     * 1 2 3 * + 4 5 ~ / -
     * 4 3 0 2 - * -
     * 4 3 2 ~ * ~ +
     * 4 3 2 ~ * -
     */
    init_VM();

    Chunk chunk;
    init_chunk(&chunk);

    write_chunk(&chunk, OP_CONSTANT, 999);
    int c1 = add_constant(&chunk, 1);
    write_chunk(&chunk, c1, 999);

    write_chunk(&chunk, OP_CONSTANT, 999);
    int c2 = add_constant(&chunk, 2);
    write_chunk(&chunk, c2, 999);

    write_chunk(&chunk, OP_CONSTANT, 999);
    int c3 = add_constant(&chunk, 3);
    write_chunk(&chunk, c3, 999);

    write_chunk(&chunk, OP_MULTIPLY, 999);

    write_chunk(&chunk, OP_ADD, 999);

    write_chunk(&chunk, OP_CONSTANT, 999);
    int c4 = add_constant(&chunk, 4);
    write_chunk(&chunk, c4, 999);

    write_chunk(&chunk, OP_CONSTANT, 999);
    int c5 = add_constant(&chunk, 5);
    write_chunk(&chunk, c5, 999);

    write_chunk(&chunk, OP_NEGATE, 999);

    write_chunk(&chunk, OP_DIVIDE, 999);

    write_chunk(&chunk, OP_SUBTRACT, 999);

    write_chunk(&chunk, OP_RETURN, 999);

    interpret(&chunk);

    free_VM();
    free_chunk(&chunk);
    printf("\n");
}

