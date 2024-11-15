
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "stdio.h"

int main(int argc, const char **argv) {
    Chunk chunk;
    init_chunk(&chunk);
    write_chunk(&chunk, OP_RETURN, 0);
    int constant_index = add_constant(&chunk, 1.2);
    write_chunk(&chunk, OP_CONSTANT, 1);
    write_chunk(&chunk, constant_index, 1);
    disassemble_chunk(&chunk, "first!");
    free_chunk(&chunk);
    printf("\n");
}
