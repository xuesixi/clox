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
    Module *current_module;
    Table builtin;
    int gray_count;
    int gray_capacity;
    Object **gray_stack;
    size_t allocated_size;
    size_t next_gc;
} VM ;

extern String *INIT;
extern String *LENGTH;
extern String *ARRAY_ITERATOR;
extern String *SCRIPT;
extern String *ANONYMOUS_MODULE;

extern Module *repl_module;
extern jmp_buf error_buf;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_PRODUCE_ERROR,
    INTERPRET_READ_ERROR,
    INTERPRET_REPL_EXIT,
} InterpretResult;

extern VM vm;

void init_VM();
void free_VM();
InterpretResult interpret(const char *src, const char *path);
InterpretResult produce(const char *src, const char *path);
InterpretResult read_run_bytecode(const char *path);
InterpretResult load_lib(unsigned char *bytes, size_t len, const char *path);

void stack_push(Value value);
Value stack_pop();

void runtime_error(const char *format, ...);
void runtime_error_catch_1(const char *format, Value value);
void runtime_error_catch_2(const char *format, Value v1, Value v2);
void catch();
void runtime_error_and_catch(const char *format, ...);

#endif //CLOX_VM_H
