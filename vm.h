//
// Created by Yue Xue  on 11/16/24.
//

#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "table.h"

typedef struct CallFrame {
//    LoxFunction *function;
    Closure *closure;
    uint8_t *PC; // program counter
    Value *FP; // frame pointer. The start of the frame
} CallFrame;

typedef struct VM{
    CallFrame frames[FRAME_MAX];
    int frame_count;
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

void init_VM();
void free_VM();
InterpretResult interpret(const char *src);
InterpretResult produce(const char *src, const char *path);
void additional_repl_init();

#endif //CLOX_VM_H
