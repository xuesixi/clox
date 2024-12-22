//
// Created by Yue Xue  on 11/14/24.
//

#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

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
    OP_REPL_AUTO_PRINT,
    OP_POP,
    OP_DEF_GLOBAL, // OP, index: 定义一个全局变量，变量名是为const[index]之字符串。以栈顶的值为初始化值，消耗之。
    OP_DEF_GLOBAL_CONST, // OP, index: 定义一个const全局变量，变量名是为const[index]之字符串。以栈顶的值为初始化值，消耗之。
    OP_GET_GLOBAL, // OP, index: 向栈中添加一个全局变量的值，该变量名为const[index]之字符串
    OP_SET_GLOBAL, // OP, index: 为一个全局变量赋值为栈顶的值。变量名为const[index]。不消耗栈顶的值。
    OP_GET_LOCAL, // OP, index: 将stack[index]的值置入栈顶
    OP_SET_LOCAL, // op, index: 将栈顶的值赋值给stack[index]。不消耗栈顶的值。
    OP_GET_UPVALUE, // op, index: 将当前closure的upvalues[index]置入栈顶
    OP_SET_UPVALUE, // op, index: 将栈顶的值赋值给当前closure的upvalues[index]。不消耗栈顶的值。
    OP_JUMP_IF_FALSE, // op, offset16: 如果栈顶的值是false，那么ip += offset16。不消耗栈顶的值
    OP_JUMP_IF_TRUE, // op, offset16: 如果栈顶的值是true, 那么ip += offset16。不消耗栈顶的值
    OP_JUMP_BACK, // op, offset16: ip -= offset16. 不消耗栈顶的值
    OP_JUMP, // op, offset16: 无条件跳转：ip += offset16
    OP_JUMP_IF_NOT_EQUAL, // op, offset16: 如果栈顶的两个值不相等，则跳转
    OP_JUMP_IF_FALSE_POP,
    OP_JUMP_IF_TRUE_POP,
    OP_CALL, // op, arg_count: 将栈顶的值视为一个可调用对象，调用之。
    OP_CLOSURE, // op, index: const[index]是一个函数，使用那个函数创建一个closure
    OP_CLOSE_UPVALUE, // op：将栈顶的值视为一个upvalue，将其close，然后从栈中移除之
    OP_CLASS, // op，index：创建一个class对象，它的名字是const[index]。将这个class置入栈顶
    OP_GET_PROPERTY, // op, index：将栈顶的值视为一个instance，移除之，然后获取其名为const[index]的字段或者方法，置入栈顶
    OP_SET_PROPERTY, // op, index: [instance, value, top], 将value赋值给这个instance的名为const[index]的字段。该指令移除instance，但保留栈顶的value
    OP_METHOD, // op：[class, closure, top], 将closure储存为class的一个method。移除closure
    OP_PROPERTY_INVOKE, // op, index, arg_count: [receiver, args..., top]：从receiver中寻找名为const[index]的属性，然后调用之
    OP_INHERIT, // op: [superclass, subclass, top]: 让subclass继承superclass。移除subclass
    OP_SUPER_ACCESS, // op, index: [receiver, superclass, top]: 从superclass中寻找名为const[index]的方法，然后绑定给receiver。receiver和superclass都被移除，将帮绑定后的method置入栈顶。
    OP_SUPER_INVOKE, // op, index, arg_count: [receiver, args..., superclass, top]: 从superclass中寻找名为const[index]的方法，立刻调用之。
    OP_COPY, // op: 复制栈顶的值，将其置入栈顶. [a, top] -> [a, a, top]
    OP_COPY2, // op: 赋值栈顶的两个值，将他们置入栈顶。[a, b, top] -> [a, b, a, b, top]
    OP_COPY_N, // op, n: push(stack_peek(n))
    OP_INDEXING_GET, // op: [array, index, top]
    OP_INDEXING_SET, // op: [array, index, value, top]
    OP_DIMENSION_ARRAY, // op, dimension,
    OP_BUILD_ARRAY, // op, length
    OP_UNPACK_ARRAY, // op, length
    OP_CLASS_STATIC_FIELD, // op, name_index: [class, field, top] -> [class, top]
    OP_IMPORT, // op : [path, top]
    OP_RESTORE_MODULE, // op:  [old_module, nil, top] -> [new_module, top]
    OP_SWAP, // op, index:
    NOP,
    OP_DEF_PUB_GLOBAL, // op, name_index
    OP_DEF_PUB_GLOBAL_CONST, // op, name_index
} OpCode;

typedef struct Chunk{
    int count;
    int capacity;
    uint8_t *code;
    int *lines;
    ValueArray constants;
} Chunk;

void init_chunk(Chunk *c);
void write_to_chunk(Chunk *c, uint8_t data, int line);
void free_chunk(Chunk *c);
int constant_mapping(Value value);
int add_constant(Chunk *c, Value constant);
uint16_t u8_to_u16(uint8_t i0, uint8_t i1);
void u16_to_u8(uint16_t value, uint8_t *i0, uint8_t *i1);

#endif  // CLOX_CHUNK_H
