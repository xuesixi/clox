//
// Created by Yue Xue  on 11/16/24.
//

#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "table.h"
#include "object.h"

typedef enum GOTO_MARK {
    NO_MARK,
    MARK_EQUAL,
    MARK_KEY_HASHING_GET,
    MARK_KEY_HASHING_SET_0,
    MARK_KEY_HASHING_SET_1,
    MARK_MAP_GET,
} GOTO_MARK;

typedef struct CallFrame {
    Closure *closure;
    uint8_t *PC; // program counter
    Value *FP; // frame pointer. The start of the frame
    GOTO_MARK mark;
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
extern String *HAS_NEXT;
extern String *NEXT;

extern String *ARRAY_CLASS;
extern String *STRING_CLASS;
extern String *INT_CLASS;
extern String *FLOAT_CLASS;
extern String *BOOL_CLASS;
extern String *NATIVE_CLASS;
extern String *FUNCTION_CLASS;
extern String *MAP_CLASS;
extern String *CLOSURE_CLASS;
extern String *METHOD_CLASS;
extern String *MODULE_CLASS;
extern String *CLASS_CLASS;
extern String *NIL_CLASS;

extern String *ITERATOR;
extern String *EQUAL;
extern String *HASH;

extern Module *repl_module;
extern jmp_buf error_buf;

typedef enum InterpretResult{
    INTERPRET_EXECUTE_OK,
    INTERPRET_PRODUCE_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_BYTECODE_WRITE_ERROR,
    INTERPRET_BYTECODE_READ_ERROR,
    INTERPRET_BYTECODE_DISASSEMBLE_ERROR,
    INTERPRET_BYTECODE_DISASSEMBLE_OK,
    INTERPRET_REPL_EXIT,
} InterpretResult;

extern VM vm;

void init_VM();
void free_VM();
InterpretResult interpret(const char *src, const char *path);
InterpretResult produce(const char *src, const char *path);
InterpretResult read_run_bytecode(const char *path);
InterpretResult load_bytes_into_builtin(unsigned char *bytes, size_t len, const char *path);
InterpretResult disassemble_byte_code(const char *path);


void assert_ref_type(Value value, ObjectType type, const char *message);
void assert_value_type(Value value, ValueType type, const char *message);
void runtime_error(const char *format, ...);
void runtime_error_catch_1(const char *format, Value value);
void runtime_error_catch_str_v(const char *format, const char *message, Value value);
void runtime_error_catch_2(const char *format, Value v1, Value v2);
void catch(InterpretResult result);
void runtime_error_and_catch(const char *format, ...);

void stack_push(Value value);
Value stack_pop();

#endif //CLOX_VM_H
