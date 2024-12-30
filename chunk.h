//
// Created by Yue Xue  on 11/14/24.
//

#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "value.h"

typedef enum OpCode{
    OP_RETURN,
    OP_LOAD_CONSTANT, // OP, index16：将const[index16]置于栈顶
    OP_NEGATE, // [value] -> [-value]
    OP_ADD, // [a, b] -> [ a + b ]
    OP_SUBTRACT, // [a, b] -> [ a - b]
    OP_MULTIPLY, // [a, b] -> [ a * b ]
    OP_DIVIDE, // [a, b] -> [ a / b ]
    OP_MOD, // [a, b] -> [a % b]
    OP_POWER, // [ a, b] -> [a ** b]
    OP_LOAD_NIL, // [ ] -> [nil]
    OP_LOAD_TRUE, // [ ] -> [true]
    OP_LOAD_FALSE, // [ ] -> [false]
    OP_NOT, // [a] -> [-a]
    OP_TEST_LESS, // [a, b ] -> [a < b]
    OP_TEST_GREATER, // [a, b] -> [a > b]
    OP_TEST_EQUAL, // [a, b] -> [a == b]
    OP_PRINT, // [a] -> [ ], print a
    OP_REPL_AUTO_PRINT,
    OP_POP, // [a] -> [ ]
    OP_DEF_GLOBAL, // OP, index16: 定义一个全局变量，变量名是为const[index]之字符串。以栈顶的值为初始化值，消耗之。
    OP_DEF_GLOBAL_CONST, // OP, index16: 定义一个const全局变量，变量名是为const[index]之字符串。以栈顶的值为初始化值，消耗之。
    OP_GET_GLOBAL, // OP, index16: 向栈中添加一个全局变量的值，该变量名为const[index]之字符串
    OP_SET_GLOBAL, // OP, index16: 为一个全局变量赋值为栈顶的值。变量名为const[index]。不消耗栈顶的值。
    OP_GET_LOCAL, // OP, index: 将stack[index]的值置入栈顶
    OP_SET_LOCAL, // op, index: 将栈顶的值赋值给stack[index]。不消耗栈顶的值。
    OP_GET_UPVALUE, // op, index: 将当前closure的upvalues[index]置入栈顶
    OP_SET_UPVALUE, // op, index: 将栈顶的值赋值给当前closure的upvalues[index]。不消耗栈顶的值。
    OP_JUMP_IF_FALSE, // op, offset16: 如果栈顶的值是false，那么ip += offset16。不消耗栈顶的值
    OP_JUMP_IF_TRUE, // op, offset16: 如果栈顶的值是true, 那么ip += offset16。不消耗栈顶的值
    OP_JUMP_BACK, // op, offset16: ip -= offset16. 不消耗栈顶的值
    OP_JUMP, // op, offset16: 无条件跳转：ip += offset16
    OP_JUMP_IF_NOT_EQUAL, // op, offset16: 如果栈顶的两个值不相等，则跳转
    OP_POP_JUMP_IF_FALSE,
    OP_POP_JUMP_IF_TRUE,
    OP_CALL, // op, arg_count: 将栈顶的值视为一个可调用对象，调用之。
    OP_MAKE_CLOSURE, // op, index16: const[index]是一个函数，使用那个函数创建一个closure
    OP_CLOSE_UPVALUE, // op：将栈顶的值视为一个upvalue，将其close，然后从栈中移除之
    OP_MAKE_CLASS, // op，index16：创建一个class对象，它的名字是const[index]。将这个class置入栈顶
    OP_GET_PROPERTY, // op, index16：将栈顶的值视为一个instance，移除之，然后获取其名为const[index]的字段或者方法，置入栈顶
    OP_SET_PROPERTY, // op, index16: [instance, value], 将value赋值给这个instance的名为const[index]的字段。该指令移除instance，但保留栈顶的value
    OP_MAKE_METHOD, // op：[class, closure], 将closure储存为class的一个method。移除closure
    OP_PROPERTY_INVOKE, // op, index16, arg_count: [receiver, args...]：从receiver中寻找名为const[index]的属性，然后调用之
    OP_INHERIT, // op: [superclass, subclass]: 让subclass继承superclass。移除subclass
    OP_SUPER_ACCESS, // op, index16: [receiver, superclass]: 从superclass中寻找名为const[index]的方法，然后绑定给receiver。receiver和superclass都被移除，将帮绑定后的method置入栈顶。
    OP_SUPER_INVOKE, // op, index16, arg_count: [receiver, args..., superclass]: 从superclass中寻找名为const[index]的方法，立刻调用之。
    OP_COPY, // op: 复制栈顶的值，将其置入栈顶. [a] -> [a, a]
    OP_COPY2, // op: 赋值栈顶的两个值，将他们置入栈顶。[a, b] -> [a, b, a, b]
    OP_COPY_N, // op, n: push(stack_peek(n))
    OP_INDEXING_GET, // op: [array, index]
    OP_INDEXING_SET, // op: [array, index, value]
    OP_DIMENSION_ARRAY, // op, dimension,
    OP_MAKE_ARRAY, // op, length
    OP_UNPACK_ARRAY, // op, length
    OP_MAKE_STATIC_FIELD, // op, name_index16: [class, field] -> [class]
    OP_IMPORT, // op : [path_string]
    OP_RESTORE_MODULE, // op:  [old_module, nil] -> [new_module]
    OP_SWAP, // op, index: swap(top, top - index)
    NOP,
    OP_DEF_PUB_GLOBAL, // op, name_index16
    OP_DEF_PUB_GLOBAL_CONST, // op, name_index16
    OP_EXPORT, // op, name_index16
    OP_LOAD_ABSENCE,
    OP_JUMP_IF_NOT_ABSENCE, // op, offset16
    OP_ARR_AS_VAR_ARG, // op
    OP_JUMP_FOR_ITER, // op, offset16: [iter] -> [iter, item], 如果还有下一个元素，则将之置于栈顶，否则跳转
    OP_GET_ITERATOR, // op: [iterable] -> [iterator]
    OP_MAP_ADD_PAIR, // op, [map, k0, v0] -> [map]
    OP_NEW_MAP, // op: [] -> [map]
    OP_SET_TRY,
    OP_SKIP_CATCH,
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
uint16_t add_constant(Chunk *c, Value constant);
//uint16_t u8_to_u16(uint8_t i0, uint8_t i1);
void u16_to_u8(uint16_t value, uint8_t *i0, uint8_t *i1);

#define u8_to_u16(i0, i1) ( ( ( (uint16_t) (i1) ) << 8) | (i0) )

#endif  // CLOX_CHUNK_H
