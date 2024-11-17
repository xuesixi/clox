//
// Created by Yue Xue  on 11/14/24.
//

#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

typedef enum {
    OP_RETURN,
    OP_CONSTANT,
    OP_NEGATE,
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t *code;
    int *lines;
    ValueArray constants;
} Chunk;

void init_chunk(Chunk *c);
void write_chunk(Chunk *c, uint8_t data, int line);
void free_chunk(Chunk *c);
int add_constant(Chunk *c, Value constant);

#endif //CLOX_CHUNK_H
