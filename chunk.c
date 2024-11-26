//
// Created by Yue Xue  on 11/14/24.
//

#include "chunk.h"

#include "memory.h"

static void init_constant(Chunk *chunk) {
    add_constant(chunk, int_value(0));
    add_constant(chunk, int_value(1));
    add_constant(chunk, int_value(2));
    add_constant(chunk, int_value(3));
    add_constant(chunk, int_value(4));
    add_constant(chunk, int_value(5));
    add_constant(chunk, int_value(6));
    add_constant(chunk, int_value(7));
    add_constant(chunk, int_value(8));
    add_constant(chunk, float_value(1.0));
    add_constant(chunk, float_value(2.0));
    add_constant(chunk, float_value(0.5));
    add_constant(chunk, float_value(0.0));
}

/**
 *
 * @param i0 [7,0]
 * @param i1 [15,8]
 * @return
 */
inline uint16_t u8_to_u16(uint8_t i0, uint8_t i1) {
    uint16_t value = (((uint16_t)i1) << 8) + i0;
    return value;
}

/**
 * @param i0 [7,0]
 * @param i1 [15,8]
 */
inline void u16_to_u8(uint16_t value, uint8_t *i0, uint8_t *i1) {
    *i0 = value & 0xff;
    *i1 = (value >> 8) & 0xff;
}

void init_chunk(Chunk *c) {
    c->capacity = 0;
    c->count = 0;
    c->code = NULL;
    c->lines = NULL;
    init_ValueArray(&c->constants);
    init_constant(c);
}

int compute_new_capacity(Chunk *c) {
    return c->capacity < 8 ? 8 : c->capacity * 2;
}

/**
 * 往指定 chunk 中写入一个字节，以及其所在的行
 * @param c 指定的 chunk
 * @param data 数据，也就是一个字节
 * @param line 所在行
 */
void write_chunk(Chunk *c, uint8_t data, int line) {
    if (c->capacity == c->count) {
        int new_capacity = compute_new_capacity(c);
        c->code = GROW_ARRAY(uint8_t, c->code, c->capacity, new_capacity);
        c->lines = GROW_ARRAY(int, c->lines, c->capacity, new_capacity);
        c->capacity = new_capacity;
    }
    c->code[c->count] = data;
    c->lines[c->count] = line;
    c->count++;
}

void free_chunk(Chunk *c) {
    FREE_ARRAY(uint8_t, c->code, c->count);
    FREE_ARRAY(int, c->lines, c->count);
    init_chunk(c);
}

/**
 * 少部分常数会被预先加载到constants。
 * @param value
 * @return 如果value被预先加载，返回其索引，否则返回-1
 */
int constant_mapping(Value value) {
    if (is_int(value) && as_int(value) <= 8) {
        return as_int(value);
    } else if (is_float(value)) {
        double v = as_float(value);
        if (v == 1.0) {
            return 9;
        } else if (v == 2.0) {
            return 10;
        } else if (v == 0.5) {
            return 11;
        } else if (v == 0.0) {
            return 12;
        }
    }
    return -1;
}

/**
 * 往指定 chunk 中写入一个常量
 * @param c 指定的 chunk
 * @param constant 常量
 * @return 该常量在 chunk 中的 constants 中的索引。
 */
inline int add_constant(Chunk *c, Value constant) {
    append_ValueArray(&c->constants, constant);
    return c->constants.count - 1;
}
