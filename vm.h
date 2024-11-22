//
// Created by Yue Xue  on 11/16/24.
//

#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "table.h"

#define STACK_MAX 256

typedef struct {
    Chunk *chunk;
    uint8_t *ip;
    Value stack[STACK_MAX];
    Value *stack_top;
    Object *objects;
    Table string_table;
    Table globals;
} VM ;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void init_VM();
void free_VM();
InterpretResult interpret(const char *src);
void runtime_error(const char *format, ...);
void push_stack(Value value);
Value pop_stack();

#endif //CLOX_VM_H
