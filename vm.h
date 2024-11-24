//
// Created by Yue Xue  on 11/16/24.
//

#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "table.h"

typedef struct VM{
    Chunk *chunk;
    uint8_t *ip;
    Value stack[STACK_MAX];
    Value *stack_top;
    Object *objects; // 所有object的值
    Table string_table; // 同名的String只会创建一次。
    Table globals; // 储存所有全局变量
    Table const_table;  // 储存所有const的全局变量。(const的全局变量会同时存在与globals和const_table中）
} VM ;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_PRODUCE_ERROR,
} InterpretResult;

extern VM vm;
extern bool REPL;

void init_VM();
void free_VM();
InterpretResult interpret(const char *src);
InterpretResult produce(const char *src, const char *path);
InterpretResult run_chunk(Chunk *chunk);
void runtime_error(const char *format, ...);
void push_stack(Value value);
Value pop_stack();

#endif //CLOX_VM_H
