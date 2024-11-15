//
// Created by Yue Xue  on 11/14/24.
//

#include "debug.h"
#include "stdio.h"
#include "assert.h"

/**
 *
 * @param chunk
 * @param name
 */
void disassemble_chunk(Chunk *chunk, const char *name) {
    assert(chunk != NULL);
    printf("== %s ==\n", name);
    for (int offset = 0; offset < chunk->count; ) {
        offset = disassemble_instruction(chunk, offset);
    }
}

static int simple_instruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int constant_instruction(const char *name, const Chunk *chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    print_value(chunk->constants.values[constant]);
    return offset + 2;
}

int disassemble_instruction(Chunk *chunk, int offset) {
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset-1]) {
        printf("   | ");
    }else {
        printf("%4d ", chunk->lines[offset]);
    }
    int instruction = chunk->code[offset];
    switch (instruction) {
        case OP_RETURN:
            return simple_instruction("OP_RETURN", offset);
        case OP_CONSTANT:
            return constant_instruction("OP_CONSTANT", chunk, offset);
        default:
            printf("Unknown instruction: %d\n", instruction);
            return offset + 1;
    }
}