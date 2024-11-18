//
// Created by Yue Xue  on 11/16/24.
//

#include "vm.h"
#include "compiler.h"
#include "stdio.h"
#include "debug.h"

#define NEW_LINE() printf("\n")

VM vm;

static uint8_t read_byte();
static void reset_stack();
static Value read_constant();
static InterpretResult run();
static void show_stack();
static void binary_op(char operator);

/* ------------------上面是静态函数申明-----------------------
   ------------------下面是静态函数定义----------------------- */

static void binary_op(char operator) {
    Value b = pop_stack();
    Value a = pop_stack();
    double result;
    switch (operator) {
        case '+':
            result = a + b;
            break;
        case '-':
            result = a - b;
            break;
        case '*':
            result = a * b;
            break;
        case '/':
            result = a / b;
            break;
        default:
            printf("%c is not a valid binary operator\n", operator);
            return;
    }
    push_stack(result);
}

static void show_stack() {
    for (Value *i = vm.stack; i < vm.stack_top; i++) {
        printf("[ ");
        print_value(*i);
        printf(" ]");
    }
    NEW_LINE();
}

static void reset_stack() {
    vm.stack_top = vm.stack;
}

/**
 * 读取下一个字节，然后移动 ip
 * @return 下一个字节
 */
static inline uint8_t read_byte() {
    return (*vm.ip++);
}

static inline Value read_constant() {
    uint8_t index = read_byte();
    return vm.chunk->constants.values[index];
}

/**
 * 遍历虚拟机的 chunk 中的每一个执行，执行之
 * @return 执行的结果
 */
static InterpretResult run() {
    while (true) {

#ifdef DEBUG_TRACE_EXECUTION
        show_stack();
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction = read_byte();

        switch (instruction) {
            case OP_RETURN: {
                print_value(pop_stack());
                NEW_LINE();
                return INTERPRET_OK;
            }
            case OP_CONSTANT: {
                Value value = read_constant();
                push_stack(value);
                break;
            }
            case OP_NEGATE:
                push_stack(-pop_stack());
                break;
            case OP_ADD:
                binary_op('+');
                break;
            case OP_SUBTRACT:
                binary_op('-');
                break;
            case OP_MULTIPLY:
                binary_op('*');
                break;
            case OP_DIVIDE:
                binary_op('/');
                break;
        }
    }
}

/* ------------------上面是静态函数定义-----------------------
   ------------------下面是申明在头文件中的函数定义----------------- */

void init_VM() {
    reset_stack();
}

void free_VM() {}

void push_stack(Value value) {
    * vm.stack_top = value;
    vm.stack_top++;
}

Value pop_stack() {
    vm.stack_top--;
    return *(vm.stack_top);
}

/**
 * 将虚拟机要执行的代码块设定为目标代码块，然后执行之
 * @param chunk 要执行的代码快
 * @return 执行结果（是否出错等）
 */
InterpretResult interpret(const char *src) {
    Chunk chunk;
    init_chunk(&chunk);

    if (!compile(src, &chunk)) {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = chunk.code;

    InterpretResult result = run();
    free_chunk(&chunk);
    return result;
}

