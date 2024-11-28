//
// Created by Yue Xue  on 11/16/24.
//

#include "vm.h"

#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "stdarg.h"

VM vm;

static CallFrame *curr_frame();
static uint8_t read_byte();
static uint16_t read_uint16();
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

static inline CallFrame *curr_frame() {
    return vm.frames + vm.frame_count - 1;
}

static void binary_number_op(Value a, Value b, char operator) {
    if (operator==
        '+' &&(is_ref_of(a, OBJ_STRING) || is_ref_of(b, OBJ_STRING))) {
        String *str = string_concat(a, b);
        stack_push(ref_value(&str->object));
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
                stack_push(int_value(a_v + b_v));
                break;
            case '-':
                stack_push(int_value(a_v - b_v));
                break;
            case '*':
                stack_push(int_value(a_v * b_v));
                break;
            case '/':
                stack_push(int_value(a_v / b_v));
                break;
            case '%':
                stack_push(int_value(a_v % b_v));
                break;
            case '>':
                stack_push(bool_value(a_v > b_v));
                break;
            case '<':
                stack_push(bool_value(a_v < b_v));
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
                stack_push(float_value(a_v + b_v));
                break;
            case '-':
                stack_push(float_value(a_v - b_v));
                break;
            case '*':
                stack_push(float_value(a_v * b_v));
                break;
            case '/':
                stack_push(float_value(a_v / b_v));
                break;
            case '>':
                stack_push(bool_value(a_v > b_v));
                break;
            case '<':
                stack_push(bool_value(a_v < b_v));
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
                stack_push(float_value(a_v + b_v));
                break;
            case '-':
                stack_push(float_value(a_v - b_v));
                break;
            case '*':
                stack_push(float_value(a_v * b_v));
                break;
            case '/':
                stack_push(float_value(a_v / b_v));
                break;
            case '>':
                stack_push(bool_value(a_v > b_v));
                break;
            case '<':
                stack_push(bool_value(a_v < b_v));
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
                stack_push(float_value(a_v + b_v));
                break;
            case '-':
                stack_push(float_value(a_v - b_v));
                break;
            case '*':
                stack_push(float_value(a_v * b_v));
                break;
            case '/':
                stack_push(float_value(a_v / b_v));
                break;
            case '>':
                stack_push(bool_value(a_v > b_v));
                break;
            case '<':
                stack_push(bool_value(a_v < b_v));
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
    Value b = stack_pop();
    Value a = stack_pop();
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

static inline void reset_stack() {
    vm.stack_top = vm.stack;
    vm.frame_count = 0;
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
    CallFrame *frame = curr_frame();
    size_t index = frame->PC - frame->function->chunk.code - 1;
    int line = frame->function->chunk.lines[index];
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack();
}

/**
 * 读取curr_frame的下一个字节，然后移动PC
 * @return 下一个字节
 */
static inline uint8_t read_byte() {
    return *curr_frame()->PC ++;
}

static inline uint16_t read_uint16() {
    uint8_t i0 = read_byte();
    uint8_t i1 = read_byte();
    return u8_to_u16(i0, i1);
}

/**
 * 将下一个字节解释为常数索引，返回其对应的常数值
 * @return 下一个字节代表的常数
 */
static inline Value read_constant() {
    return curr_frame()->function->chunk.constants.values[read_byte()];
}

static inline Value read_constant2() {
    return curr_frame()->function->chunk.constants.values[read_uint16()];
}

/**
 * 将下一个字节解释为字符串常数索引，返回其对应的String
 * @return 下一个字节代表的String
 */
static inline String *read_constant_string() {
    Value value = read_constant();
    if (!is_ref_of(value, OBJ_STRING)) {
        IMPLEMENTATION_ERROR(
            "trying to read a constant string, but the value is not a String");
        return NULL;
    }
    return (String *)(as_ref(value));
}

/**
 * 遍历虚拟机的 curr_frame 的 chunk 中的每一个指令，执行之
 * @return 执行的结果
 */
static InterpretResult run() {
    while (true) {
        if (TRACE_EXECUTION) {
            show_stack();
            CallFrame *frame = curr_frame();
            disassemble_instruction(& frame->function->chunk, (int)(frame->PC - frame->function->chunk.code));
        }
        uint8_t instruction = read_byte();

        switch (instruction) {
            case OP_RETURN: {
                return INTERPRET_OK;
            }
            case OP_CONSTANT: {
                Value value = read_constant();
                stack_push(value);
                break;
            }
            case OP_CONSTANT2: {
                Value value = read_constant2();
                stack_push(value);
                break;
            }
            case OP_NEGATE: {
                Value value = peek_stack(0);
                if (is_int(value)) {
                    stack_push(int_value(-as_int(stack_pop())));
                    break;
                } else if (is_float(value)) {
                    stack_push(float_value(-as_float(stack_pop())));
                    break;
                } else {
                    runtime_error("the value of type: %d is cannot be negated",
                                  value.type);
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
                Value b = stack_pop();
                Value a = stack_pop();
                stack_push(bool_value(value_equal(a, b)));
                break;
            }
            case OP_NIL:
                stack_push(nil_value());
                break;
            case OP_TRUE:
                stack_push(bool_value(true));
                break;
            case OP_FALSE:
                stack_push(bool_value(false));
                break;
            case OP_NOT:
                stack_push(bool_value(is_falsy(stack_pop())));
                break;
            case OP_PRINT:
                if (TRACE_EXECUTION) {
                    printf(">>> ");
                }
                print_value(stack_pop());
                NEW_LINE();
                break;
            case OP_POP:
                stack_pop();
                break;
            case OP_DEFINE_GLOBAL: {
                String *name = read_constant_string();
                table_set(&vm.globals, name, peek_stack(0));
                stack_pop();
                break;
            }
            case OP_DEFINE_GLOBAL_CONST: {
                String *name = read_constant_string();
                table_set(&vm.globals, name, peek_stack(0));
                table_set(&vm.const_table, name, bool_value(true));
                stack_pop();
                break;
            }
            case OP_GET_GLOBAL: {
                String *name = read_constant_string();
                Value value;
                if (table_get(&vm.globals, name, &value)) {
                    stack_push(value);
                } else {
                    runtime_error("Accessing an undefined variable: %s",
                                  name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_GLOBAL: {
                String *name = read_constant_string();
                if (table_has(&vm.const_table, name)) {
                    runtime_error("The const variable %s cannot be re-assigned",
                                  name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (table_set(&vm.globals, name, peek_stack(0))) {
                    break;
                } else {
                    table_delete(&vm.globals, name);
                    runtime_error("Setting an undefined variable: %s",
                                  name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
            }
            case OP_GET_LOCAL: {
                int index = read_byte();
                stack_push(curr_frame()->FP[index]);
                break;
            }
            case OP_SET_LOCAL: {
                int index = read_byte();
                curr_frame()->FP[index] = peek_stack(0);
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = read_uint16();
                if (is_falsy(peek_stack(0))) {
                    curr_frame()->PC += offset;
                }
                break;
            }
            case OP_JUMP_IF_TRUE: {
                uint16_t offset = read_uint16();
                if (!is_falsy(peek_stack(0))) {
                    curr_frame()->PC += offset;
                }
                break;
            }
            case OP_JUMP: {
                uint16_t offset = read_uint16();
                curr_frame()->PC += offset;
                break;
            }
            case OP_JUMP_BACK: {
                uint16_t offset = read_uint16();
                curr_frame()->PC -= offset;
                break;
            }
            case OP_JUMP_IF_NOT_EQUAL: {
                uint16_t offset = read_uint16();
                Value b = peek_stack(0);
                Value a = peek_stack(1);
                if (!value_equal(a, b)) {
                    curr_frame()->PC += offset;
                }
                break;
            }
            case OP_JUMP_IF_FALSE_POP: {
                uint16_t offset = read_uint16();
                if (is_falsy(stack_pop())) {
                    curr_frame()->PC += offset;
                }
                break;
            }
            case OP_JUMP_IF_TRUE_POP: {
                uint16_t offset = read_uint16();
                if (!is_falsy(stack_pop())) {
                    curr_frame()->PC += offset;
                }
                break;
            }
            default: {
                runtime_error("unrecognized instruction");
            }
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
    init_table(&vm.const_table);
}

void free_VM() {
    free_all_objects();
    free_table(&vm.string_table);
    free_table(&vm.globals);
    free_table(&vm.const_table);
    free_map(&label_map);
}

inline void stack_push(Value value) {
    *vm.stack_top = value;
    vm.stack_top++;
}

inline Value stack_pop() {
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

    LoxFunction *function = compile(src);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    vm.frame_count++;
    CallFrame *frame = curr_frame();
    frame->function = function;
    frame->FP = vm.stack;
    frame->PC = function->chunk.code;
    stack_push(ref_value((Object *) function));
    return run();
}

/**
 * 目前还没写好。因为chunk内部是code指针
 * @param src
 * @param path
 * @return
 */
InterpretResult produce(const char *src, const char *path) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        printf("Error when opening the file: %s\n", path);
        return INTERPRET_PRODUCE_ERROR;
    }
    Chunk chunk;
    init_chunk(&chunk);
    if (!compile(src)) {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    unsigned long result = fwrite(&chunk, sizeof(Chunk), 1, file);
    fclose(file);
    free_chunk(&chunk);
    if (result == 1) {
        return INTERPRET_OK;
    } else {
        printf("Error when writing to the file: %s\n", path);
        return INTERPRET_PRODUCE_ERROR;
    }
}
