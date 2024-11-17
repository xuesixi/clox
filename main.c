
#include "chunk.h"
#include "vm.h"
#include "stdio.h"

int main(int argc, const char **argv) {
    init_VM();

    Chunk chunk;
    init_chunk(&chunk);

    int constant_index = add_constant(&chunk, 1.2);
    write_chunk(&chunk, OP_CONSTANT, 999);
    write_chunk(&chunk, constant_index, 999);
    write_chunk(&chunk, OP_NEGATE, 999);

    write_chunk(&chunk, OP_RETURN, 999);

    interpret(&chunk);

    free_VM();
    free_chunk(&chunk);
    printf("\n");
}

