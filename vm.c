//
// Created by Yue Xue  on 11/16/24.
//

#include "vm.h"
#include "compiler.h"
#include "memory.h"
#include "object.h"

#include "stdarg.h"
#include "debug.h"

VM vm;

static uint8_t read_byte();
static bool is_falsy(Value value);
static void reset_stack();
static Value read_constant();
static inline Value read_constant2();
static InterpretResult run();
static void show_stack();
static Value peek_stack(int distance);
static void binary_op(char operator);
static void binary_number_op(Value a, Value b, char operator);

/* ------------------上面是静态函数申明-----------------------
   ------------------下面是静态函数定义----------------------- */

static void binary_number_op(Value a, Value b, char operator) {
    if (operator == '+' &&  (is_ref_of(a, OBJ_STRING) || is_ref_of(a, OBJ_STRING))) {
        String *str = string_concat(a, b);
        push_stack(ref_value(& str->object));
        return;
    }
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
            case '>':
                push_stack(bool_value(a_v > b_v));
                break;
            case '<':
                push_stack(bool_value(a_v < b_v));
                break;
            default:
                IMPLEMENTATION_ERROR("invalid binary operator");
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
            case '>':
                push_stack(bool_value(a_v > b_v));
                break;
            case '<':
                push_stack(bool_value(a_v < b_v));
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
            case '>':
                push_stack(bool_value(a_v > b_v));
                break;
            case '<':
                push_stack(bool_value(a_v < b_v));
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
            case '>':
                push_stack(bool_value(a_v > b_v));
                break;
            case '<':
                push_stack(bool_value(a_v < b_v));
                break;
            default:
                runtime_error("invalid binary operator");
                return;
        }
    }
}

/**
 * 处理二元操作符。两个出栈动作将由该函数执行
 * */
static void binary_op(char operator) {
    Value b = pop_stack();
    Value a = pop_stack();
    switch (operator) {
        case '+':
        case '-':
        case '*':
        case '%':
        case '/':
        case '>':
        case '<':
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
void runtime_error(const char *format, ...) {
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

/**
 * 将下一个字节解释为常数索引，返回其对应的常数值
 * @return 下一个字节代表的常数
 */
static inline Value read_constant() {
    uint8_t index = read_byte();
    return vm.chunk->constants.values[index];
}

static inline Value read_constant2() {
    uint8_t indices[2] = {read_byte(), read_byte()};
    uint16_t index = * ((uint16_t*) indices);
    return vm.chunk->constants.values[index];
}

/**
 * 将下一个字节解释为字符串常数索引，返回其对应的String
 * @return 下一个字节代表的String
 */
static inline String *read_constant_string() {
    Value value = read_constant();
    if (!is_ref_of(value, OBJ_STRING)) {
        IMPLEMENTATION_ERROR("trying to read a constant string, but the value is not a String");
        return NULL;
    }
    return (String*)(as_ref(value));
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
                return INTERPRET_OK;
            }
            case OP_CONSTANT: {
                Value value = read_constant();
                push_stack(value);
                break;
            }
            case OP_CONSTANT2: {
                Value value = read_constant2();
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
            case OP_LESS:
                binary_op('<');
                break;
            case OP_GREATER:
                binary_op('>');
                break;
            case OP_EQUAL: {
                Value b = pop_stack();
                Value a = pop_stack();
                push_stack(bool_value(value_equal(a, b)));
                break;
            }
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
            case OP_PRINT:
#ifdef DEBUG_TRACE_EXECUTION
                printf(">>> ");
#endif
                print_value(pop_stack());
                NEW_LINE();
                break;
            case OP_POP:
                pop_stack();
                break;
            case OP_DEFINE_GLOBAL: {
                String *name = read_constant_string();
                table_set(&vm.globals, name, peek_stack(0));
                pop_stack();
                break;
            }
            case OP_GET_GLOBAL: {
                String *name = read_constant_string();
                Value value;
                if (table_get(&vm.globals, name, &value)) {
                    push_stack(value);
                } else {
                    runtime_error("Accessing an undefined variable");
                }
                break;
            }
            case OP_SET_GLOBAL: {
                String *name = read_constant_string();
                if (table_set(&vm.globals, name, peek_stack(0)) ) {
                    break;
                } else {
                    table_delete(&vm.globals, name);
                    runtime_error("Setting an undefined global variable: %s", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
            }
            case OP_GET_LOCAL: {
                int index = read_byte();
                push_stack(vm.stack[index]);
                break;
            }
            case OP_SET_LOCAL: {
                int index = read_byte();
                vm.stack[index] = peek_stack(0);
                break;
            }
            default:
                runtime_error("unrecognized instruction");
        }
    }
}

/* ------------------上面是静态函数定义-----------------------
   ------------------下面是申明在头文件中的函数定义----------------- */

void init_VM() {
    reset_stack();
    vm.objects = NULL;
    init_table(&vm.string_table);
    init_table(&vm.globals);
}

void free_VM() {
    free_all_objects();
    free_table(&vm.string_table);
    free_table(&vm.globals);
}

inline void push_stack(Value value) {
    * vm.stack_top = value;
    vm.stack_top++;
}

inline Value pop_stack() {
    vm.stack_top--;
    return *(vm.stack_top);
}

/**
 * 先调用 compile 将源代码编译成字节码，然后运行字节码.
 * 在repl中，每一次输入都将执行该函数.
 * Chunk会在该函数内部产生、消失
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

