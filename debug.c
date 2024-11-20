//
// Created by Yue Xue  on 11/14/24.
//

#include "debug.h"
#include "stdio.h"
//#include "assert.h"

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
    uint8_t index = chunk->code[offset + 1];
    printf("%-16s %4d -> ", name, index);
    Value value = chunk->constants.values[index];
    print_value_with_type(value);
    printf("\n");
    return offset + 2;
}

/**
 * 给定一个 offset，将对应的 instruction 的信息打印
 * @return 下一个 instruction 的 offset
 */
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
        case OP_NEGATE:
            return simple_instruction("OP_NEGATE", offset);
        case OP_ADD:
            return simple_instruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simple_instruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simple_instruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simple_instruction("OP_DIVIDE", offset);
        case OP_MOD:
            return simple_instruction("OP_MOD", offset);
        case OP_NIL:
            return simple_instruction("OP_NIL", offset);
        case OP_TRUE:
            return simple_instruction("OP_TRUE", offset);
        case OP_FALSE:
            return simple_instruction("OP_FALSE", offset);
        case OP_NOT:
            return simple_instruction("OP_NOT", offset);
        default:
            printf("Unknown instruction: %d\n", instruction);
            return offset + 1;
    }
}