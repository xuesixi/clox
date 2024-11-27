//
// Created by Yue Xue  on 11/14/24.
//

#include "debug.h"

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

static int byte_instruction(const char *name, const Chunk *chunk, int offset) {
    uint8_t index = chunk->code[offset + 1];
    printf("%-23s %4d\n", name, index);
    return offset + 2;
}

static int constant_instruction(const char *name, const Chunk *chunk, int offset) {
    uint8_t index = chunk->code[offset + 1];
    printf("%-23s %4d : ", name, index);
    Value value = chunk->constants.values[index];
    // print_value_with_type(value);
    print_value(value);
    printf("\n");
    return offset + 2;
}

static int constant2_instruction(const char *name, const Chunk *chunk, int offset) {
    uint8_t i0 = chunk->code[offset + 1];
    uint8_t i1 = chunk->code[offset + 2];
    int index = u8_to_u16(i0, i1);
    printf("%-23s %4d : ", name, index);
    Value value = chunk->constants.values[index];
    print_value(value);
    printf("\n");
    return offset + 3;
}

static int jump_instruction(const char *name, const Chunk *chunk, int offset, bool forward) {
    uint8_t i0 = chunk->code[offset + 1];
    uint8_t i1 = chunk->code[offset + 2];
    int index = u8_to_u16(i0, i1);
    int target = forward ? offset + 3 + index : offset + 3 - index;
    char *label = map_get(&label_map, (void*)(target + 1));
    if (label != NULL) {
        printf("%-23s   -> %04d: %s\n", name, target, label);
    } else {
        printf("%-23s   -> %04d\n", name, target);
    }
    return offset + 3;
}

/**
 * 给定一个 offset，将对应的 instruction 的信息打印
 * @return 下一个 instruction 的 offset
 */
int disassemble_instruction(Chunk *chunk, int offset) {

    char *label = map_get(&label_map, (void*)(offset + 1)); // +1 是为了防止索引0被当成NULL。见label_statement()
    if (label != NULL) {
        printf("%s: \n", label);
    }

    // code index
    printf("%04d ", offset);

    // print line number
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
        case OP_CONSTANT2:
            return constant2_instruction("OP_CONSTANT2", chunk, offset);
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
        case OP_EQUAL:
            return simple_instruction("OP_EQUAL", offset);
        case OP_LESS:
            return simple_instruction("OP_LESS", offset);
        case OP_GREATER:
            return simple_instruction("OP_GREATER", offset);
        case OP_PRINT:
            return simple_instruction("OP_PRINT", offset);
        case OP_POP:
            return simple_instruction("OP_POP", offset);
        case OP_DEFINE_GLOBAL:
            return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL_CONST:
            return constant_instruction("OP_DEFINE_GLOBAL_CONST", chunk, offset);
        case OP_GET_GLOBAL:
            return constant_instruction("OP_GET_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constant_instruction("OP_SET_GLOBAL", chunk, offset);
        case OP_GET_LOCAL:
            return byte_instruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return byte_instruction("OP_SET_LOCAL", chunk, offset);
        case OP_JUMP:
            return jump_instruction("OP_JUMP", chunk, offset, true);
        case OP_JUMP_IF_FALSE:
            return jump_instruction("OP_JUMP_IF_FALSE", chunk, offset, true);
        case OP_JUMP_IF_TRUE:
            return jump_instruction("OP_JUMP_IF_TRUE", chunk, offset, true);
        case OP_JUMP_BACK:
            return jump_instruction("OP_JUMP_BACK", chunk, offset, false);
        case OP_JUMP_IF_NOT_EQUAL:
            return jump_instruction("OP_JUMP_IF_NOT_EQUAL", chunk, offset, true);
        case OP_JUMP_IF_FALSE_POP:
            return jump_instruction("OP_JUMP_IF_FALSE_POP", chunk, offset, true);
        case OP_JUMP_IF_TRUE_POP:
            return jump_instruction("OP_JUMP_IF_TRUE_POP", chunk, offset, true);
        default:
            printf("Unknown instruction: %d\n", instruction);
            return offset + 1;
    }
}