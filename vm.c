//
// Created by Yue Xue  on 11/16/24.
//

#include "vm.h"

#include "compiler.h"
#include "io.h"
#include "debug.h"
#include "memory.h"
#include "object.h"

#include "stdarg.h"
#include "setjmp.h"
#include "string.h"
#include "time.h"
#include "math.h"

VM vm;

jmp_buf error_buf;

static CallFrame *curr_frame();
static uint8_t read_byte();
static uint16_t read_uint16();
static bool is_falsy(Value value);
static void reset_stack();
static Value read_constant();
static Value read_constant2();
static InterpretResult run();
static void show_stack();
static Value peek_stack(int distance);
static void binary_number_op(Value a, Value b, char operator);
static void call_value(Value value, int arg_count);
static void runtime_error(const char *format, ...);
void catch();
void runtime_error_and_catch(const char *format, ...);
static void stack_push(Value value);
static Value stack_pop();




/* ------------------上面是静态函数申明-----------------------
   ------------------下面是静态函数定义----------------------- */

static inline CallFrame *curr_frame() {
    return vm.frames + vm.frame_count - 1;
}

static void binary_number_op(Value a, Value b, char operator) {
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
                runtime_error_and_catch("invalid binary operator");
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
                runtime_error_and_catch("invalid binary operator");
                return;
        }
    } else if (is_float(a) && is_float(b)){
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
                runtime_error_and_catch("invalid binary operator");
                return;
        }
    } else if (operator== '+' && (is_ref_of(a, OBJ_STRING) || is_ref_of(b, OBJ_STRING))) {
        String *str = string_concat(a, b);
        stack_push(ref_value(&str->object));
        return;
    } else {
        char *a_text = to_print_chars(a);
        char *b_text = to_print_chars(b);
        runtime_error("the operands, %s and %s, do not support the operation: %c", a_text, b_text, operator);
        free(a_text);
        free(b_text);
        catch();
        return;
    }
}

static void show_stack() {
    printf(" ");
    for (Value *i = vm.stack; i < vm.stack_top; i++) {
        if (i == curr_frame()->FP) {
            start_color(BOLD_RED);
            printf("@");
            end_color();
        } else {
            printf(" ");
        }
        printf("[ ");
        print_value_with_color(*i);
        printf(" ]");
    }
    NEW_LINE();

}

static inline bool is_falsy(Value value) {
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
    vm.open_upvalues = NULL;
}

static inline Value peek_stack(int distance) {
    return vm.stack_top[-1 - distance];
}

/**
 * 输出错误消息（会自动添加换行符）
 * 打印调用栈消息。清空栈。
 */
void runtime_error(const char *format, ...) {
    fputs("\nRuntime Error: ", stderr);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frame_count - 1; i >= 0; i --) {
        CallFrame *frame = vm.frames + i;
        LoxFunction *function = frame->closure->function;
        size_t index = frame->PC - frame->closure->function->chunk.code - 1;
        int line = function->chunk.lines[index];
        fprintf(stderr, "at [line %d] in ", line);
        if (i == 0) {
            fprintf(stderr, "main()\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    reset_stack();
}

/**
 * 跳转到错误处理处。
 * 该函数只应该用在runtime_error()后面。
 */
inline void catch() {
    longjmp(error_buf, 1);
}

/**
 * 输出错误消息（会自动添加换行符）
 * 打印调用栈消息。清空栈。然后跳转至错误处理处
 */
void runtime_error_and_catch(const char *format, ...) {
    fputs("\nRuntime Error: ", stderr);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frame_count - 1; i >= 0; i --) {
        CallFrame *frame = vm.frames + i;
        LoxFunction *function = frame->closure->function;
        size_t index = frame->PC - frame->closure->function->chunk.code - 1;
        int line = function->chunk.lines[index];
        fprintf(stderr, "at [line %d] in ", line);
        if (i == 0) {
            fprintf(stderr, "main()\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    reset_stack();
    catch();
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
    return curr_frame()->closure->function->chunk.constants.values[read_byte()];
}

static inline Value read_constant2() {
    return curr_frame()->closure->function->chunk.constants.values[read_uint16()];
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
    return (String *)(as_ref(value));
}

/**
 * 返回一个捕获了当前处于value位置上的那个值的UpValueObject对象。
 * 如果那个只是第一次被捕获，新建UpValueObject，将其添加入链表中，然后返回之，否则沿用旧有的。
 */
static UpValueObject *capture_upvalue(Value *value) {
    // vm.open_upvalues 是有序的。第一个元素的position值是最大的（处于stack中更高的位置）
    UpValueObject *curr = vm.open_upvalues;
    UpValueObject *pre = NULL;
    while (curr != NULL && curr->position > value) {
        pre = curr;
        curr = curr->next;
    }
    // 三种情况：curr为NULL；curr->position == value; curr->position < value


    if (curr != NULL && curr->position == value) {
        return curr;
    }

    UpValueObject *new_capture = new_upvalue(value);
    new_capture->next = curr;
    if (pre == NULL) {
        vm.open_upvalues = new_capture;
    } else {
        pre->next = curr;
    }

    return new_capture;
}


/**
 * 将vm.open_upvalues中所有位置处于position以及其上的upvalue的值进行“close”，
 * 也就是说，将对应的栈上的值保存到UpValueObject其自身中。
 */
static void close_upvalue(Value *position) {
    UpValueObject *curr = vm.open_upvalues;
    while (curr != NULL && curr->position >= position) {
        curr->closed = *curr->position; // 把栈上的值保存入closed中
        curr->position = &curr->closed; // 重新设置position。如此一来upvalue的get，set指令可以正常运行
        curr = curr->next;
    }
    vm.open_upvalues = curr;
}

/**
 * 遍历虚拟机的 curr_frame 的 function 的 chunk 中的每一个指令，执行之
 * @return 执行的结果
 */
static InterpretResult run() {

    if (setjmp(error_buf)) {
        //  运行时错误会跳转至这里
        return INTERPRET_RUNTIME_ERROR;
    }
//    char console[20];
    while (true) {
        if (TRACE_EXECUTION && TRACE_SKIP == -1) {
            show_stack();
//            fgets(console, 20, stdin);
//            if (strcmp(console, "o\n") == 0) {
//                TRACE_SKIP = true;
//            }
            if (getchar() == 'o') {
                TRACE_SKIP = vm.frame_count - 1; // skip until the frame_count is equal to TRACE_SKIP
                while (getchar() != '\n');
            } else {
                CallFrame *frame = curr_frame();
                disassemble_instruction(& frame->closure->function->chunk, (int)(frame->PC - frame->closure->function->chunk.code));
            }
        }

        uint8_t instruction = read_byte();
        switch (instruction) {
            case OP_RETURN: {
                Value result = stack_pop(); // 返回值
                vm.stack_top = curr_frame()->FP;
                close_upvalue(vm.stack_top);
                vm.frame_count--;
                if (vm.frame_count == TRACE_SKIP) {
                    TRACE_SKIP = -1; // no skip. Step by step
                }
                if (vm.frame_count == 0) {
                    return INTERPRET_OK; // 程序运行结束
                }
                stack_push(result);
                break;
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
                    runtime_error_and_catch("the value of type: %d is cannot be negated", value.type);
                }
            }
            case OP_ADD: {
                Value b = stack_pop();
                Value a = stack_pop();
                binary_number_op(a, b, '+');
                break;
            }
            case OP_SUBTRACT: {
                Value b = stack_pop();
                Value a = stack_pop();
                binary_number_op(a, b, '-');
                break;
            }
            case OP_MULTIPLY: {
                Value b = stack_pop();
                Value a = stack_pop();
                binary_number_op(a, b, '*');
                break;
            }
            case OP_DIVIDE: {
                Value b = stack_pop();
                Value a = stack_pop();
                binary_number_op(a, b, '/');
                break;
            }
            case OP_MOD: {
                Value b = stack_pop();
                Value a = stack_pop();
                binary_number_op(a, b, '%');
                break;
            }
            case OP_POWER: {
                Value b = stack_pop();
                Value a = stack_pop();
                if (is_number(a) && is_number(b)) {
                    stack_push(float_value(pow(AS_NUMBER(a), AS_NUMBER(b))));
                } else {
                    char *a_text = to_print_chars(a);
                    char *b_text = to_print_chars(b);
                    runtime_error("the operands, %s and %s, do not support the power operation", a_text, b_text);
                    free(a_text);
                    free(b_text);
                    catch();
                }
                break;
            }
            case OP_LESS: {
                Value b = stack_pop();
                Value a = stack_pop();
                binary_number_op(a, b, '<');
                break;
            }
            case OP_GREATER: {
                Value b = stack_pop();
                Value a = stack_pop();
                binary_number_op(a, b, '>');
                break;
            }
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
            case OP_PRINT: {
                if (REPL || TRACE_EXECUTION) {
                    start_color(BOLD_GREEN);
                }
                print_value(stack_pop());
                NEW_LINE();
                if (REPL || TRACE_EXECUTION) {
                    end_color();
                }
                break;
            }
            case OP_EXPRESSION_PRINT: {
                Value to_print = stack_pop();
                start_color(GRAY);
                print_value(to_print);
                NEW_LINE();
                end_color();
                break;
            }
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
                    runtime_error_and_catch("Accessing an undefined variable: %s", name->chars);
                }
                break;
            }
            case OP_SET_GLOBAL: {
                String *name = read_constant_string();
                if (table_has(&vm.const_table, name)) {
                    runtime_error_and_catch("The const variable %s cannot be re-assigned", name->chars);
                }
                if (table_set(&vm.globals, name, peek_stack(0))) {
                    break;
                } else {
                    table_delete(&vm.globals, name);
                    runtime_error_and_catch("Setting an undefined variable: %s", name->chars);
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
            case OP_CALL: {
                int count = read_byte();
                Value callee = peek_stack(count);
                call_value(callee, count);
                break;
            }
            case OP_CLOSURE: {
                /* 值得注意的是，对于嵌套的closure，虽然在编译时，是内部的
                 * 函数先编译，但是在运行时，是先执行外层函数的OP_closure。
                 * 然后等到外层函数被调用、内层函数被定义后，内层函数的OP_closure
                 * 才会被执行
                 */
                Value f = read_constant();
                Closure *closure = new_closure(as_function(f));
                stack_push(ref_value((Object *) closure));

                for (int i = 0; i < closure->upvalue_count; ++i) {
                    bool is_local = read_byte();
                    int index = read_byte();
                    if (is_local) {
                        closure->upvalues[i] = capture_upvalue(curr_frame()->FP + index);
                    } else {
                        closure->upvalues[i] = curr_frame()->closure->upvalues[index];
                    }
                }
                
                break;
            }
            case OP_GET_UPVALUE: {
                int index = read_byte();
                stack_push(* curr_frame()->closure->upvalues[index]->position);
                break;
            }
            case OP_SET_UPVALUE: {
                int index = read_byte();
                * curr_frame()->closure->upvalues[index]->position = peek_stack(0);
                break;
            }
            case OP_CLOSE_UPVALUE: {
                close_upvalue(vm.stack_top - 1);
                stack_pop();
                break;
            }
            default: {
                runtime_error_and_catch("unrecognized instruction");
                break;
            }
        }
    }
}

/**
 * 创建一个call frame。
 * 如果value不是函数，或者参数数量不匹配，则出现runtime错误
 * @param value 要调用的函数
 * @param arg_count 传入的参数的个数
 */
static void call_value(Value value, int arg_count) {
    if (is_ref_of(value, OBJ_CLOSURE)) {
        LoxFunction *function = as_closure(value)->function;
        if (function->arity != arg_count) {
            runtime_error_and_catch("%s expects %d arguments, but got %d", function->name->chars, function->arity, arg_count);
        }
        if (vm.frame_count == FRAME_MAX) {
            runtime_error_and_catch("Stack overflow.");
        }
        vm.frame_count ++;
        CallFrame *frame = curr_frame();
        frame->FP = vm.stack_top - arg_count - 1;
        frame->PC = function->chunk.code;
        frame->closure = as_closure(value);
    } else if (is_ref_of(value, OBJ_NATIVE)) {
        NativeFunction *native = as_native(value);
        if (native->arity != arg_count && native->arity != -1) {
            runtime_error_and_catch("%s expects %d arguments, but got %d", native->name->chars, native->arity, arg_count);
        }
        Value result = native->impl(arg_count, vm.stack_top - arg_count);
        vm.stack_top -= arg_count + 1;
        stack_push(result);
    } else {
        char *name = to_print_chars(value);
        runtime_error("%s is not callable", name);
        free(name);
        catch();
    }
}

static void define_native(const char *name, NativeImplementation impl, int arity) {
    int len = (int) strlen(name);
//    String *str = string_copy(name, len);
//    NativeFunction *nativeFunction = new_native(impl);
//    table_set(&vm.globals, str, ref_value((Object*)nativeFunction));

    stack_push(ref_value((Object *)string_copy(name, len)));
    stack_push(ref_value((Object *)new_native(impl, as_string(vm.stack[0]), arity)));
    table_set(&vm.globals, as_string(vm.stack[0]), vm.stack[1]);
    stack_pop();
    stack_pop();
}

static inline int max(int a, int b) {
    return a > b ? a : b;
}

static Value native_format(int count, Value *values) {
    const char *format = as_string(*values)->chars;
    int capacity = 25;
    char *buf = malloc(capacity);
    int pre = 0;
    int curr = 0;
    int buf_len = 0;
    int curr_v = 0;

    while (true) {
        while (format[curr] != '\0' && format[curr] != '#') {
            curr ++;
        }

        int new_len = curr - pre;
        // buf overflow checking
        if (buf_len + new_len > capacity) {
            capacity = max(2 * capacity, buf_len + new_len);
            buf = realloc(buf, capacity);
        }

        memcpy(buf + buf_len, format + pre, new_len);
        buf_len += new_len;
        pre = curr;

        if (format[curr] == '\0') {
            if (curr_v != count - 1) {
                free(buf);
                runtime_error_and_catch("format: more arguments than placeholders");
            }
            break;
        }

        if (format[curr] == '#') {
            if (curr_v == count - 1) {
                free(buf);
                runtime_error_and_catch("format: more placeholders than arguments");
            }

            Value v = values[curr_v + 1];
            char *v_chars = to_print_chars(v);

            // buf overflow checking
            int v_chars_len;
            if (is_ref_of(v, OBJ_STRING)) {
                v_chars_len = as_string(v)->length;
            } else {
                v_chars_len = strlen(v_chars);
            }
            if (v_chars_len + buf_len > capacity) {
                capacity = max(2 * capacity, v_chars_len + buf_len);
                buf = realloc(buf, capacity);
            }
            memcpy(buf + buf_len, v_chars, v_chars_len);
            buf_len += v_chars_len;
            curr ++;
            pre = curr;
            curr_v ++;
            free(v_chars);
        }
    }

    String *string = string_copy(buf, buf_len);
    free(buf);
    return ref_value((Object *) string);
}

static Value native_clock(int count, Value *value) {
    (void )count;
    (void )value;
    return float_value((double)clock() / CLOCKS_PER_SEC);
}

static Value native_int(int count, Value *value) {
    (void )count;
    switch (value->type) {
        case VAL_INT:
            return *value;
        case VAL_FLOAT:
            return int_value((int) as_float(*value));
        case VAL_BOOL:
            return int_value((int) as_bool(*value));
        default:
            if (is_ref_of(*value, OBJ_STRING)) {
                String *str = as_string(*value);
                char *end;
                int result = strtol(str->chars, &end, 10);
                if (end == str->chars) {
                    runtime_error_and_catch("not a valid int: %s", str->chars);
                }
                return int_value(result);
            }
            runtime_error_and_catch("not a valid input");
            return nil_value();
    }
}

static Value native_float(int count, Value *value) {
    (void )count;
    Value v = *value;
    switch (v.type) {
        case VAL_INT:
            return float_value((double ) as_int(v));
        case VAL_FLOAT:
            return v;
        case VAL_BOOL:
            return float_value((double ) as_bool(v));
        default:
            if (is_ref_of(v, OBJ_STRING)) {
                String *str = as_string(v);
                char *end;
                double result = strtod(str->chars, &end);
                if (end == str->chars) {
                    runtime_error_and_catch("not a valid float: %s", str->chars);
                }
                return float_value(result);
            }
            runtime_error_and_catch("not a valid input");
            return nil_value();
    }
}

static Value native_help(int count, Value *value) {
    (void ) count;
    (void ) value;
    printf("You are in the REPL mode because you run clox directly without providing any arguments.\n");
    printf("You can also do `clox path/to/script` to run a lox script.\n");
    printf("Or do `clox -h` to see more options\n");
    printf("In this REPL mode, expression results will be printed out automatically in gray color. \n");
    printf("You may also omit the last semicolon for a statement.\n");
    printf("Use ctrl+C or ctrl+D to quit.\n");
    return nil_value();
}

static Value native_rand(int count, Value *value) {
    (void )count;
    Value a = value[0];
    Value b = value[1];
    if (!is_int(a) || !is_int(b)) {
        runtime_error_and_catch("arguments need to be int");
    }
    return int_value(as_int(a) + rand() % as_int(b));
}


/* ------------------上面是静态函数定义-----------------------
   ------------------下面是申明在头文件中的函数定义----------------- */

void init_VM() {
    reset_stack();
    vm.objects = NULL;
    vm.open_upvalues = NULL;
    srand(time(NULL));
    init_table(&vm.string_table);
    init_table(&vm.globals);
    init_table(&vm.const_table);
    define_native("clock", native_clock, 0);
    define_native("int", native_int, 1);
    define_native("float", native_float, 1);
    define_native("rand", native_rand, 2);
    define_native("f", native_format, -1);
}

void additional_repl_init() {
    define_native("help", native_help, 0);
}

/**
 * 清除下列数据
 * @par objects
 * @par string_table
 * @par globals
 * @par const_table
 * @par label_map
 */
void free_VM() {
    free_all_objects();
    free_table(&vm.string_table);
    free_table(&vm.globals);
    free_table(&vm.const_table);
//    free_map(&label_map);
}

/**
 * 将value置于stack_top处，然后自增stack_top
 * @param value 想要添加的value
 */
static inline void stack_push(Value value) {
    *vm.stack_top = value;
    vm.stack_top++;
}

static inline Value stack_pop() {
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

    stack_push(ref_value((Object *) function));

    vm.frame_count++;
    CallFrame *frame = curr_frame();

    Closure *closure = new_closure(function);
    frame->closure = closure;
    frame->FP = vm.stack;
    frame->PC = function->chunk.code;

    stack_pop();
    stack_push(ref_value((Object *) closure));

    return run();
}

/**
 * 目前还没写好。因为chunk内部是code指针
 * @param src
 * @param path
 * @return
 */
InterpretResult produce(const char *src, const char *path) {
    LoxFunction *function = compile(src);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        printf("Error when opening the file: %s\n", path);
        return INTERPRET_PRODUCE_ERROR;
    }
    write_function(file, function);
    fclose(file);
    return INTERPRET_OK;
}

InterpretResult read_run_bytecode(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        printf("Error when opening the file: %s\n", path);
        return INTERPRET_READ_ERROR;
    }
    LoxFunction *function = read_function(file);
    fclose(file);

    stack_push(ref_value((Object *) function));
    vm.frame_count++;
    CallFrame *frame = curr_frame();

    Closure *closure = new_closure(function);
    frame->closure = closure;
    frame->FP = vm.stack;
    frame->PC = function->chunk.code;

    stack_pop();
    stack_push(ref_value((Object *) closure));

    return run();
}
