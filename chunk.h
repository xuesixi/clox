//
// Created by Yue Xue  on 11/14/24.
//

#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

typedef enum OpCode{
    OP_RETURN,
    OP_CONSTANT, // OP, index：将const[index]置于栈顶
    OP_CONSTANT2, // OP, index16: 将const[index16]置于栈顶。
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MOD,
    OP_POWER,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_NOT,
    OP_LESS,
    OP_GREATER,
    OP_EQUAL,
    OP_PRINT,
    OP_EXPRESSION_PRINT,
    OP_POP,
    OP_DEFINE_GLOBAL, // OP, index: 定义一个全局变量，变量名是为const[index]之字符串。以栈顶的值为初始化值，消耗之。
    OP_DEFINE_GLOBAL_CONST, // OP, index: 定义一个const全局变量，变量名是为const[index]之字符串。以栈顶的值为初始化值，消耗之。
    OP_GET_GLOBAL, // OP, index: 向栈中添加一个全局变量的值，该变量名为const[index]之字符串
    OP_SET_GLOBAL, // OP, index: 为一个全局变量赋值为栈顶的值。变量名为const[index]。不消耗栈顶的值。
    OP_GET_LOCAL, // OP, index: 将stack[index]的值置入栈顶
    OP_SET_LOCAL, // op, index: 将栈顶的值赋值给stack[index]。不消耗栈顶的值。
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_JUMP_IF_FALSE, // op, offset16: 如果栈顶的值是false，那么ip += offset16。不消耗栈顶的值
    OP_JUMP_IF_TRUE, // op, offset16: 如果栈顶的值是true, 那么ip += offset16。不消耗栈顶的值
    OP_JUMP_BACK, // op, offset16: ip -= offset16. 不消耗栈顶的值
    OP_JUMP, // op, offset16: 无条件跳转：ip += offset16
    OP_JUMP_IF_NOT_EQUAL, // op, offset16: 如果栈顶的两个值不相等，则跳转
    OP_JUMP_IF_FALSE_POP,
    OP_JUMP_IF_TRUE_POP,
    OP_CALL, // op, arg_count:
    OP_CLOSURE,
} OpCode;

typedef struct Chunk{
    int count;
    int capacity;
    uint8_t *code;
    int *lines;
    ValueArray constants;
} Chunk;

void init_chunk(Chunk *c);
void write_chunk(Chunk *c, uint8_t data, int line);
void free_chunk(Chunk *c);
int constant_mapping(Value value);
int add_constant(Chunk *c, Value constant);
uint16_t u8_to_u16(uint8_t i0, uint8_t i1);
void u16_to_u8(uint16_t value, uint8_t *i0, uint8_t *i1);

#endif  // CLOX_CHUNK_H
