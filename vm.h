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
    Module *module; // the running module
} CallFrame;

typedef struct TrySavePoint {
    int frame_count;
    uint8_t *PC;
    Value *stack_top;
    struct TrySavePoint *next;
} TrySavePoint;

typedef struct VM{
    CallFrame frames[FRAME_MAX];
    int frame_count;
    Value stack[STACK_MAX];
    Value *stack_top;
    UpValue *open_upvalues;
    Object *objects; // 所有object的值
    Table string_table; // 同名的String只会创建一次。
    Table builtin;
    int gray_count;
    int gray_capacity;
    Object **gray_stack;
    size_t allocated_size;
    size_t next_gc;
    TrySavePoint *last_save;
} VM ;

extern Module *repl_module;
extern jmp_buf error_buf;

typedef enum InterpretResult{
    INTERPRET_0,
    INTERPRET_EXECUTE_OK,
    INTERPRET_PRODUCE_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_BYTECODE_WRITE_ERROR,
    INTERPRET_BYTECODE_READ_ERROR,
    INTERPRET_BYTECODE_DISASSEMBLE_ERROR,
    INTERPRET_BYTECODE_DISASSEMBLE_OK,
    INTERPRET_REPL_EXIT,
    INTERPRET_ERROR_CAUGHT,
} InterpretResult;

extern VM vm;

void init_VM();
void free_VM();
InterpretResult interpret(const char *src, const char *path);
InterpretResult produce(const char *src, const char *path);
InterpretResult read_run_bytecode(const char *path);
InterpretResult load_bytes_into_builtin(unsigned char *bytes, size_t len, const char *path);
InterpretResult disassemble_byte_code(const char *path);

void assert_ref_type(Value value, ObjectType type, const char *expected_type);
void assert_value_type(Value value, ValueType type, const char *expected_type);
void runtime_error(const char *format, ...);

void catch(InterpretResult result);
Class *value_class(Value value);

void stack_push(Value value);
Value stack_pop();
void map_delete();

#endif //CLOX_VM_H
