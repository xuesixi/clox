//
// Created by Yue Xue  on 11/14/24.
//

#include "chunk.h"
#include "memory.h"

void init_chunk(Chunk *c) {
    c->capacity = 0;
    c->count = 0;
    c->code = NULL;
    c->lines = NULL;
    init_ValueArray(& c->constants);
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
        c->lines = GROW_ARRAY(int , c->lines, c->capacity, new_capacity);
        c->capacity = new_capacity;
    }
    c->code[c->count] = data;
    c->lines[c->count] = line;
    c->count ++;
}

void free_chunk(Chunk *c) {
    FREE_ARRAY(uint8_t, c->code, c->count);
    FREE_ARRAY(int , c->lines, c->count);
    init_chunk(c);
}

/**
 * 往指定 chunk 中写入一个常量
 * @param c 指定的 chunk
 * @param constant 常量
 * @return 该常量在 chunk 中的 constants 中的索引。
 */
int add_constant(Chunk *c, Value constant) {
    write_ValueArray(& c->constants, constant);
    return c->constants.count - 1;
}


