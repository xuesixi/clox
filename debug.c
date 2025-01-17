//
// Created by Yue Xue  on 11/14/24.
//

#include "debug.h"
#include "object.h"


/**
 *
 * @return -1 if error, 0 if ok
 */
int disassemble_chunk(Chunk *chunk, const char *name) {
    assert(chunk != NULL);
    start_color(YELLOW);
    printf("== %s ==\n", name);
    end_color();
    for (int offset = 0; offset < chunk->count; ) {
        offset = disassemble_instruction(chunk, offset, true);
        if (offset == -1) {
            return - 1;
        }
    }
    NEW_LINE();
    return 0;
}

static int simple_instruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int byte_instruction(const char *name, const Chunk *chunk, int offset, const char *meaning) {
    uint8_t index = chunk->code[offset + 1];
    printf("%-23s %s: %d\n", name, meaning, index);
    return offset + 2;
}

static int constant16_instruction(const char *name, const Chunk *chunk, int offset) {
    uint8_t i0 = chunk->code[offset + 1];
    uint8_t i1 = chunk->code[offset + 2];
    uint16_t index = u8_to_u16(i0, i1);
    printf("%-23s ", name);
    start_color(GRAY);
    printf("%d: ", index);
    end_color();
    Value value = chunk->constants.values[index];
    print_value_with_color(value);
    NEW_LINE();
    return offset + 3;
}

static int invoke_instruction(const char *name, const Chunk *chunk, int offset) {
    uint8_t name_index0 = chunk->code[offset + 1];
    uint8_t name_index1 = chunk->code[offset + 2];
    int name_index = u8_to_u16(name_index0, name_index1);
    uint8_t arg_count = chunk->code[offset + 3];
    Value method_name = chunk->constants.values[name_index];
    printf("%-23s ", name);
    print_value_with_color(method_name);
    printf("(%d)", arg_count);
    NEW_LINE();
    return offset + 4;
}

static int jump_instruction(const char *name, const Chunk *chunk, int offset, bool forward) {
    uint8_t i0 = chunk->code[offset + 1];
    uint8_t i1 = chunk->code[offset + 2];
    int index = u8_to_u16(i0, i1);
    int target = forward ? offset + 3 + index : offset + 3 - index;

    printf("%-23s ", name);
    start_color(BOLD_RED);
    printf("-> %d\n", target);
    end_color();
    return offset + 3;
}

/**
 * 给定一个 offset，将对应的 instruction 的信息打印
 * @return 下一个 instruction 的 offset
 */
int disassemble_instruction(Chunk *chunk, int offset, bool line_break) {

    // print line number
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset-1]) {
        printf("       ");
    }else {
        if (line_break) {
            printf("\n");
        }
        printf("%4d   ", chunk->lines[offset]);
    }

    printf("     %4d   ", offset);

    int instruction = chunk->code[offset];
    switch (instruction) {
        case OP_RETURN:
            return simple_instruction("RETURN", offset);
        case OP_LOAD_CONSTANT:
            return constant16_instruction("LOAD_CONSTANT", chunk, offset);
        case OP_NEGATE:
            return simple_instruction("NEGATE", offset);
        case OP_ADD:
            return simple_instruction("ADD", offset);
        case OP_SUBTRACT:
            return simple_instruction("SUBTRACT", offset);
        case OP_MULTIPLY:
            return simple_instruction("MULTIPLY", offset);
        case OP_DIVIDE:
            return simple_instruction("DIVIDE", offset);
        case OP_MOD:
            return simple_instruction("MOD", offset);
        case OP_POWER:
            return simple_instruction("POWER", offset);
        case OP_LOAD_NIL:
            return simple_instruction("LOAD_NIL", offset);
        case OP_LOAD_TRUE:
            return simple_instruction("LOAD_TRUE", offset);
        case OP_LOAD_FALSE:
            return simple_instruction("LOAD_FALSE", offset);
        case OP_NOT:
            return simple_instruction("NOT", offset);
        case OP_TEST_EQUAL:
            return simple_instruction("TEST_EQUAL", offset);
        case OP_TEST_LESS:
            return simple_instruction("TEST_LESS", offset);
        case OP_TEST_GREATER:
            return simple_instruction("TEST_GREATER", offset);
        case OP_PRINT:
            return simple_instruction("PRINT", offset);
        case OP_REPL_AUTO_PRINT:
            return simple_instruction("REPL_AUTO_PRINT", offset);
        case OP_POP:
            return simple_instruction("POP", offset);
        case OP_DEF_GLOBAL:
            return constant16_instruction("DEF_GLOBAL", chunk, offset);
        case OP_DEF_GLOBAL_CONST:
            return constant16_instruction("DEF_GLOBAL_CONST", chunk, offset);
        case OP_GET_GLOBAL:
            return constant16_instruction("GET_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constant16_instruction("SET_GLOBAL", chunk, offset);
        case OP_GET_LOCAL:
            return byte_instruction("GET_LOCAL", chunk, offset, "index");
        case OP_SET_LOCAL:
            return byte_instruction("SET_LOCAL", chunk, offset, "index");
        case OP_JUMP:
            return jump_instruction("JUMP", chunk, offset, true);
        case OP_JUMP_IF_FALSE:
            return jump_instruction("JUMP_IF_FALSE", chunk, offset, true);
        case OP_JUMP_IF_TRUE:
            return jump_instruction("JUMP_IF_TRUE", chunk, offset, true);
        case OP_JUMP_BACK:
            return jump_instruction("JUMP_BACK", chunk, offset, false);
        case OP_JUMP_IF_NOT_EQUAL:
            return jump_instruction("JUMP_IF_NOT_EQUAL", chunk, offset, true);
        case OP_POP_JUMP_IF_FALSE:
            return jump_instruction("POP_JUMP_IF_FALSE", chunk, offset, true);
        case OP_POP_JUMP_IF_TRUE:
            return jump_instruction("POP_JUMP_IF_TRUE", chunk, offset, true);
        case OP_CALL:
            return byte_instruction("CALL", chunk, offset, "arg #");
        case OP_GET_UPVALUE:
            return byte_instruction("GET_UPVALUE", chunk, offset, "index");
        case OP_SET_UPVALUE:
            return byte_instruction("SET_UPVALUE", chunk, offset, "index");
        case OP_MAKE_CLOSURE: {
            offset++;
            uint8_t u0 = chunk->code[offset ++];
            uint8_t u1 = chunk->code[offset ++];
            int index = u8_to_u16(u0, u1);
            printf("%-23s ", "MAKE_CLOSURE");
            start_color(GRAY);
            printf("%d : ", index);
            end_color();
            Value value = chunk->constants.values[index];
            print_value_with_color(value);
            NEW_LINE();
            LoxFunction *function = as_function(value);
            offset += function->upvalue_count * 2;
            return offset;
        }
        case OP_CLOSE_UPVALUE:
            return simple_instruction("CLOSE_UPVALUE", offset);
        case OP_MAKE_CLASS:
            return constant16_instruction("MAKE_CLASS", chunk, offset);
        case OP_GET_PROPERTY:
            return constant16_instruction("GET_PROPERTY", chunk, offset);
        case OP_COPY:
            return simple_instruction("COPY", offset);
        case OP_COPY2:
            return simple_instruction("COPY2", offset);
        case OP_COPY_N:
            return byte_instruction("COPY_N", chunk, offset, "position");
        case OP_SET_PROPERTY:
            return constant16_instruction("SET_PROPERTY", chunk, offset);
        case OP_MAKE_METHOD:
            return simple_instruction("MAKE_METHOD", offset);
        case OP_PROPERTY_INVOKE:
            return invoke_instruction("PROPERTY_INVOKE", chunk, offset);
        case OP_INHERIT:
            return simple_instruction("INHERIT", offset);
        case OP_SUPER_ACCESS:
            return constant16_instruction("SUPER_ACCESS", chunk, offset);
        case OP_SUPER_INVOKE:
            return invoke_instruction("SUPER_INVOKE", chunk, offset);
        case OP_DIMENSION_ARRAY:
            return byte_instruction("DIMENSION_ARRAY", chunk, offset, "dimension");
        case OP_MAKE_ARRAY:
            return byte_instruction("MAKE_ARRAY", chunk, offset, "length");
        case OP_UNPACK_ARRAY:
            return byte_instruction("UNPACK_ARRAY", chunk, offset, "length");
        case OP_INDEXING_GET:
            return simple_instruction("INDEXING_GET", offset);
        case OP_INDEXING_SET:
            return simple_instruction("INDEXING_SET", offset);
        case OP_MAKE_STATIC_FIELD:
            return constant16_instruction("MAKE_STATIC_FIELD", chunk, offset);
        case OP_IMPORT:
            return simple_instruction("IMPORT", offset);
        case OP_RESTORE_MODULE:
            return simple_instruction("RESTORE_MODULE", offset);
        case NOP:
            return simple_instruction("NOP", offset);
        case OP_SWAP:
            return byte_instruction("SWAP", chunk, offset, "position");
        case OP_DEF_PUB_GLOBAL:
            return constant16_instruction("DEF_PUB_GLOBAL", chunk, offset);
        case OP_DEF_PUB_GLOBAL_CONST:
            return constant16_instruction("DEF_PUB_GLOBAL_CONST", chunk, offset);
        case OP_EXPORT:
            return constant16_instruction("EXPORT", chunk, offset);
        case OP_LOAD_ABSENCE:
            return simple_instruction("LOAD_ABSENCE", offset);
        case OP_JUMP_IF_NOT_ABSENCE:
            return jump_instruction("JUMP_IF_NOT_ABSENCE", chunk, offset, true);
        case OP_ARR_AS_VAR_ARG:
            return simple_instruction("ARR_AS_VAR_ARG", offset);
        case OP_JUMP_FOR_ITER:
            return jump_instruction("JUMP_FOR_ITER", chunk, offset, true);
        case OP_GET_ITERATOR:
            return simple_instruction("GET_ITERATOR", offset);
        case OP_MAP_ADD_PAIR:
            return simple_instruction("MAP_ADD_PAIR", offset);
        case OP_NEW_MAP:
            return simple_instruction("NEW_MAP", offset);
        case OP_SET_TRY:
            return jump_instruction("SET_TRY", chunk, offset, true);
        case OP_SKIP_CATCH:
            return jump_instruction("SKIP_CATCH", chunk, offset, true);
        case OP_THROW:
            return simple_instruction("THROW", offset);
        case OP_TEST_VALUE_OF:
            return byte_instruction("TEST_VALUE_OF", chunk, offset, "amount");
        case OP_JUMP_IF_EQUAL:
            return jump_instruction("JUMP_IF_EQUAL", chunk, offset, true);
        default:
            printf("Unknown instruction: %d\n", instruction);
            return -1;
    }
}