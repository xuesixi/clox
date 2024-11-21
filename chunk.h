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
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MOD,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_NOT,
    OP_LESS,
    OP_GREATER,
    OP_EQUAL,
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

#endif  // CLOX_CHUNK_H
