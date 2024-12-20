//
// Created by Yue Xue  on 11/16/24.
//

#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "table.h"
#include "object.h"

typedef struct CallFrame {
    Closure *closure;
    uint8_t *PC; // program counter
    Value *FP; // frame pointer. The start of the frame
} CallFrame;

typedef struct VM{
    CallFrame frames[FRAME_MAX];
    int frame_count;
    Value stack[STACK_MAX];
    Value *stack_top;
    UpValue *open_upvalues;
    Object *objects; // 所有object的值
    Table string_table; // 同名的String只会创建一次。
    Table globals; // 储存所有全局变量
    Table builtin;
    Table const_table;  // 储存所有const的全局变量。(const的全局变量会同时存在与globals和const_table中）
    int gray_count;
    int gray_capacity;
    Object **gray_stack;
    size_t allocated_size;
    size_t next_gc;
//    String *init_string;
//    String *length_string;
} VM ;

extern String *INIT;
extern String *LENGTH;
extern String *ARRAY_ITERATOR;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_PRODUCE_ERROR,
    INTERPRET_READ_ERROR,
} InterpretResult;

extern VM vm;

void init_VM();
void free_VM();
InterpretResult interpret(const char *src);
InterpretResult produce(const char *src, const char *path);
InterpretResult read_run_bytecode(const char *path);
void additional_repl_init();

void stack_push(Value value);
Value stack_pop();

#endif //CLOX_VM_H
