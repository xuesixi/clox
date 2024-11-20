//
// Created by Yue Xue  on 11/16/24.
//

#include "vm.h"
#include "compiler.h"
#include "stdarg.h"
#include "stdio.h"
#include "debug.h"

#define NEW_LINE() printf("\n")

VM vm;

static uint8_t read_byte();
static bool is_falsy(Value value);
static void reset_stack();
static void runtime_error(const char *format, ...);
static Value read_constant();
static InterpretResult run();
static void show_stack();
static Value peek_stack(int distance);
static void binary_op(char operator);
static void binary_number_op(Value a, Value b, char operator);

/* ------------------上面是静态函数申明-----------------------
   ------------------下面是静态函数定义----------------------- */

static void binary_number_op(Value a, Value b, char operator) {
    if (!is_number(a) || !is_number(b)) {
        runtime_error("the operand is not a number");
        return;
    }
    if (is_int(a) && is_int(b)) {
        int a_v = as_int(a);
        int b_v = as_int(b);
        switch (operator) {
            case '+':
                push_stack(int_value(a_v + b_v));
                break;
            case '-':
                push_stack(int_value(a_v - b_v));
                break;
            case '*':
                push_stack(int_value(a_v * b_v));
                break;
            case '/':
                push_stack(int_value(a_v / b_v));
                break;
            case '%':
                push_stack(int_value(a_v % b_v));
                break;
            default:
                runtime_error("invalid binary operator");
                return;
        }
    } else if (is_int(a) && is_float(b)) {
        int a_v = as_int(a);
        double b_v = as_float(b);
        switch (operator) {
            case '+':
                push_stack(float_value(a_v + b_v));
                break;
            case '-':
                push_stack(float_value(a_v - b_v));
                break;
            case '*':
                push_stack(float_value(a_v * b_v));
                break;
            case '/':
                push_stack(float_value(a_v / b_v));
                break;
            default:
                runtime_error("invalid binary operator");
                return;
        }
    } else if (is_float(a) && is_int(b)) {
        double a_v = as_float(a);
        int b_v = as_int(b);
        switch (operator) {
            case '+':
                push_stack(float_value(a_v + b_v));
                break;
            case '-':
                push_stack(float_value(a_v - b_v));
                break;
            case '*':
                push_stack(float_value(a_v * b_v));
                break;
            case '/':
                push_stack(float_value(a_v / b_v));
                break;
            default:
                runtime_error("invalid binary operator");
                return;
        }
    } else {
        double a_v = as_float(a);
        double b_v = as_float(b);
        switch (operator) {
            case '+':
                push_stack(float_value(a_v + b_v));
                break;
            case '-':
                push_stack(float_value(a_v - b_v));
                break;
            case '*':
                push_stack(float_value(a_v * b_v));
                break;
            case '/':
                push_stack(float_value(a_v / b_v));
                break;
            default:
                runtime_error("invalid binary operator");
                return;
        }
    }
}

static void binary_op(char operator) {
    Value b = pop_stack();
    Value a = pop_stack();
    switch (operator) {
        case '+':
        case '-':
        case '*':
        case '%':
        case '/':
            binary_number_op(a, b, operator);
            break;
        default:
            printf("%c is not a valid binary operator\n", operator);
            return;
    }
}

static void show_stack() {
    for (Value *i = vm.stack; i < vm.stack_top; i++) {
        printf("[ ");
        print_value(*i);
        printf(" ]");
    }
    NEW_LINE();
}

static bool is_falsy(Value value) {
    if (is_bool(value)) {
        return as_bool(value) == false;
    }
    if (is_nil(value)) {
        return true;
    }
    return false;
}

static void reset_stack() {
    vm.stack_top = vm.stack;
}

static inline Value peek_stack(int distance) {
    return vm.stack_top[-1 - distance];
}

/**
 * 会自动添加换行符。
 */
static void runtime_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);
    size_t index = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[index];
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack();
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
            case OP_NEGATE: {
                Value value = peek_stack(0);
                if (is_int(value)) {
                    push_stack(int_value(-as_int(pop_stack())));
                    break;
                } else if (is_float(value)) {
                    push_stack(float_value(-as_float(pop_stack())));
                    break;
                } else {
                    runtime_error("the value of type: %d is cannot be negated", value.type);
                    return INTERPRET_RUNTIME_ERROR;
                }
            }
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
            case OP_MOD:
                binary_op('%');
                break;
            case OP_NIL:
                push_stack(nil_value());
                break;
            case OP_TRUE:
                push_stack(bool_value(true));
                break;
            case OP_FALSE:
                push_stack(bool_value(false));
                break;
            case OP_NOT:
                push_stack(bool_value(is_falsy(pop_stack())));
                break;
            default:
                runtime_error("unrecognized instruction");
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

