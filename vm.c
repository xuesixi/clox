//
// Created by Yue Xue  on 11/16/24.
//

#include "vm.h"
#include "native.h"

#include "compiler.h"
#include "io.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "time.h"
#include "math.h"

#include "stdarg.h"
#include "setjmp.h"
#include "string.h"

VM vm;

String *INIT = NULL;
String *LENGTH = NULL;
String *ARRAY_CLASS = NULL;
String *ITERATOR = NULL;
Module *repl_module = NULL;
//String *SCRIPT = NULL;
//String *ANONYMOUS_MODULE = NULL;

jmp_buf error_buf;

static CallFrame *curr_frame;
static Table *curr_closure_global;
static Value *curr_const_pool;

static uint8_t read_byte();
static uint16_t read_uint16();

static bool is_falsy(Value value);

static void reset_stack();

//static Value read_constant();

static Value read_constant16();

static InterpretResult run_vm();

static void import(const char *src, String *path);

static void show_stack();

static void build_array(int length);

static Value stack_peek(int distance);

static void binary_number_op(Value a, Value b, char operator);

static void call_value(Value value, int arg_count);

static void invoke_from_class(Class *class, String *name, int arg_count);

static void call_closure(Closure *closure, int arg_count);

static Value multi_dimension_array(int dimension, Value *lens);

static void warmup(LoxFunction *function, const char *path_chars, String *path_string, bool care_repl);



// ------------------上面是静态函数申明-----------------------
// ------------------下面是静态函数定义-----------------------

static Value multi_dimension_array(int dimension, Value *lens) {
    Value len_value = *lens;
    if (!is_int(len_value)) {
        runtime_error_catch_1("%s cannot be used as the array length", len_value);
    }
    int len = as_int(*lens);
    Array *arr = new_array(len);
    if (dimension == 1) {
        return ref_value((Object *) arr);
    }
    for (int i = 0; i < len; ++i) {
        arr->values[i] = multi_dimension_array(dimension - 1, lens + 1);
    }
    return ref_value((Object *) arr);

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
    } else if (is_float(a) && is_float(b)) {
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
    } else if (operator == '+' && (is_ref_of(a, OBJ_STRING) || is_ref_of(b, OBJ_STRING))) {
        stack_push(a);
        stack_push(b);
        String *str = string_concat(a, b);
        stack_pop();
        stack_pop();
        stack_push(ref_value(&str->object));
        return;
    } else {
        char *a_text = value_to_chars(a, NULL);
        char *b_text = value_to_chars(b, NULL);
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
        if (i == curr_frame->FP) {
            start_color(BOLD_RED);
            printf("@");
            end_color();
        } else {
            printf(" ");
        }
        printf("[");
        print_value_with_color(*i);
        printf("]");
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
    curr_frame = NULL;
    vm.open_upvalues = NULL;
}

static inline Value stack_peek(int distance) {
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

    for (int i = vm.frame_count - 1; i >= 0; i--) {
        CallFrame *frame = vm.frames + i;
        LoxFunction *function = frame->closure->function;
        size_t index = frame->PC - frame->closure->function->chunk.code - 1;
        int line = function->chunk.lines[index];
        fprintf(stderr, "at [line %d] in ", line);
        char *name = value_to_chars(ref_value((Object *) frame->closure), NULL);
        fprintf(stderr, "%s\n", name);
        free(name);
//        if (i == 0) {
//            fprintf(stderr, "main()\n");
//        } else {
//            fprintf(stderr, "%s()\n", function->name->chars);
//        }
    }

    reset_stack();
}

void runtime_error_catch_1(const char *format, Value value) {
    char *str = value_to_chars(value, NULL);
    runtime_error(format, str);
    free(str);
    catch();
}

void runtime_error_catch_2(const char *format, Value v1, Value v2) {
    char *str1 = value_to_chars(v1, NULL);
    char *str2 = value_to_chars(v2, NULL);
    runtime_error(format, str1, str2);
    free(str1);
    free(str2);
    catch();
}

/**
 * 跳转到错误处理处。
 * 该函数只应该用在runtime_error()后面。
 */
inline void catch() {
    longjmp(error_buf, INTERPRET_RUNTIME_ERROR);
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

    for (int i = vm.frame_count - 1; i >= 0; i--) {
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
    return *curr_frame->PC++;
}

static inline uint16_t read_uint16() {
    uint8_t i0 = read_byte();
    uint8_t i1 = read_byte();
    return u8_to_u16(i0, i1);
}

static inline Value read_constant16() {
    return curr_const_pool[read_uint16()];
//    return curr_frame->closure->function->chunk.constants.values[read_uint16()];
}

/**
 * 将下一个u16解释为字符串常数索引，返回其对应的String
 * @return 下一个字节代表的String
 */
static inline String *read_constant_string() {
    Value value = read_constant16();
#ifdef IMPLEMENTATION_CHECK
    if (!is_ref_of(value, OBJ_STRING)) {
        IMPLEMENTATION_ERROR("trying to read a constant string, but the value is not a String");
        return NULL;
    }
#endif
    return (String *) (as_ref(value));
}

/**
 * 返回一个捕获了当前处于value位置上的那个值的UpValueObject对象。
 * 如果那个只是第一次被捕获，新建UpValueObject，将其添加入链表中，然后返回之，否则沿用旧有的。
 */
static UpValue *capture_upvalue(Value *value) {
    // vm.open_upvalues 是有序的。第一个元素的position值是最大的（处于stack中更高的位置）
    UpValue *curr = vm.open_upvalues;
    UpValue *pre = NULL;
    while (curr != NULL && curr->position > value) {
        pre = curr;
        curr = curr->next;
    }
    // 三种情况：curr为NULL；curr->position == value; curr->position < value

    if (curr != NULL && curr->position == value) {
        return curr;
    }

    UpValue *new_capture = new_upvalue(value);
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
    UpValue *curr = vm.open_upvalues;
    while (curr != NULL && curr->position >= position) {
        curr->closed = *curr->position; // 把栈上的值保存入closed中
        curr->position = &curr->closed; // 重新设置position。如此一来upvalue的get，set指令可以正常运行
        curr = curr->next;
    }
    vm.open_upvalues = curr;
}


/**
 * 在指定的class的methods中寻找某个closure，然后将其包装成一个Method
 */
static Method *bind_method(Class *class, String *name, Value receiver) {
    Value value;
    if (class == NULL || table_get(&class->methods, name, &value) == false) {
        runtime_error_and_catch("no such property: %s", name->chars);
    }
    Method *method = new_method(as_closure(value), receiver);
    return method;
}


/**
 * 在指定的class中寻找对应method，然后调用之。
 * 在调用该函数时，请确保stack_peek(arg_count)为method的receiver。
 * 如果没有找到对应的method，该函数自己会发出runtime error
 */
static void invoke_from_class(Class *class, String *name, int arg_count) {

    // stack: obj, arg1, arg2, top
    // code: op, name_index, arg_count
    Value closure_value;
    if (!table_get(&class->methods, name, &closure_value)) {
        Value receiver = stack_peek(arg_count);
        runtime_error_catch_2("%s does not have the property or method %s", receiver, ref_value((Object *) name));
    }
    call_closure(as_closure(closure_value), arg_count);
}

static void call_closure(Closure *closure, int arg_count) {
    LoxFunction *function = closure->function;
    if (function->var_arg) {
        int array_len = arg_count - function->arity;
        if (array_len < 0) {
            runtime_error_and_catch("%s expects at least %d arguments, but got %d", function->name->chars, function->arity, arg_count);
        } else {
            arg_count = function->arity + 1;
            build_array(array_len);
        }
    } else {
        if (function->arity != arg_count) {
            runtime_error_and_catch("%s expects %d arguments, but got %d", function->name->chars, function->arity, arg_count);
        }
    }
    if (vm.frame_count == FRAME_MAX) {
        runtime_error_and_catch("Stack overflow.");
    }
    vm.frame_count++;
    curr_frame = vm.frames + vm.frame_count - 1;
    CallFrame *frame = curr_frame;
    frame->FP = vm.stack_top - arg_count - 1;
    frame->PC = function->chunk.code;
    frame->closure = closure;
    curr_closure_global = &closure->module->globals;
    curr_const_pool = closure->function->chunk.constants.values;
}

/**
 * 创建一个call frame。
 * 如果value不可调用，或者参数数量不匹配，则出现runtime错误
 * @param value 要调用的closure、method、native、class
 * @param arg_count 传入的参数的个数
 */
static void call_value(Value value, int arg_count) {
    if (!is_ref(value)) {
        runtime_error_catch_1("%s is not callable", value);
    }
    switch (as_ref(value)->type) {
        case OBJ_CLOSURE: {
            call_closure(as_closure(value), arg_count);
            break;
        }
        case OBJ_NATIVE: {
            NativeFunction *native = as_native(value);
            if (native->arity != arg_count && native->arity != -1) {
                runtime_error_and_catch("%s expects %d arguments, but got %d", native->name->chars, native->arity,
                                        arg_count);
            }
            Value result = native->impl(arg_count, vm.stack_top - arg_count);
            vm.stack_top -= arg_count + 1;
            stack_push(result);
            break;
        }
        case OBJ_CLASS: {
            Class *class = as_class(value);
            Instance *instance = new_instance(class);
            Value init_closure;
            if (table_get(&class->methods, INIT, &init_closure)) {
                Method *initializer = new_method(as_closure(init_closure), ref_value((Object *) instance));
                call_value(ref_value((Object *) initializer), arg_count);
            } else if (arg_count != 0) {
                runtime_error_and_catch("%s does not define init() but got %d arguments", class->name->chars,
                                        arg_count);
            } else {
                stack_pop();
                stack_push(ref_value((Object *) instance));
            }
            break;
        }
        case OBJ_METHOD: {
            Method *method = as_method(value);
            vm.stack_top[-arg_count - 1] = method->receiver;
            call_closure(method->closure, arg_count);
            break;
        }
        default: {
            runtime_error_catch_1("%s is not callable", value);
        }
    }
}

static String *auto_length_string_copy(const char *name) {
    int len = strlen(name); // NOLINT
    return string_copy(name, len);
}

static void init_static_strings() {
    INIT = auto_length_string_copy("init");
    LENGTH = auto_length_string_copy("length");
    ARRAY_CLASS = auto_length_string_copy("Array");
//    SCRIPT = auto_length_string_copy("$script");
//    ANONYMOUS_MODULE = auto_length_string_copy("$anonymous");
    ITERATOR = auto_length_string_copy("iterator");
}


void init_VM() {
    reset_stack();
    vm.objects = NULL;
    vm.open_upvalues = NULL;
    vm.gray_count = 0;
    vm.gray_capacity = 0;
    vm.gray_stack = NULL;
    vm.allocated_size = 0;
    vm.next_gc = INITIAL_GC_SIZE;
    srand(time(NULL)); // NOLINT(*-msc51-cpp)

    vm.current_module = NULL;
    init_table(&vm.builtin);
    init_table(&vm.string_table);
    init_static_strings();

    init_vm_native();
}

/**
 * 清除下列数据
 * @par objects
 * @par string_table
 * @par globals
 * @par const_table
 */
void free_VM() {
    free_all_objects();
    free_table(&vm.builtin);
    free_table(&vm.string_table);
//    free_table(&vm.globals);
    free(vm.gray_stack);
}

/**
 * 将value置于stack_top处，然后自增stack_top
 * @param value 想要添加的value
 */
inline void stack_push(Value value) {
    *vm.stack_top = value;
    vm.stack_top++;
}

inline Value stack_pop() {
    vm.stack_top--;
    return *(vm.stack_top);
}

/**
 * 调用 compile() 将源代码编译成一个function，构造一个closure，设置栈帧，
 * 将closure置入栈中，然后用run()运行代码，返回其结果。
 * 在repl中，每一次输入都将执行该函数。
 * 如果是repl，那么使用repl_module为初始module；否则，创建新的module
 *
 * @param src 要执行的代码
 * @parem path 该脚本的路径。如果是REPL模式，不会用到该值。
 * @return 执行结果（是否出错等）
 */
InterpretResult interpret(const char *src, const char *path) {

    LoxFunction *function = compile(src);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    DISABLE_GC;

    warmup(function, path, NULL, true);

    ENABLE_GC;

    return run_vm();
}

static void import(const char *src, String *path) {

    stack_push(ref_value((Object *) vm.current_module));
    // [old_module, top]

    LoxFunction *function = compile(src);

    DISABLE_GC;

    if (function == NULL) {
        runtime_error_and_catch("the module fails to compile");
        return;
    }

    warmup(function, NULL, path, false);

    ENABLE_GC;

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

    DISABLE_GC;

    LoxFunction *function = read_function(file);
    fclose(file);
    warmup(function, path, NULL, false);

    ENABLE_GC;

    return run_vm();
}

InterpretResult load_bytes(unsigned char *bytes, size_t len, const char *path) {
    FILE *file = fmemopen(bytes, len, "rb");
    LoxFunction *function = read_function(file);
    warmup(function, path, NULL, false);
    InterpretResult result = run_vm();
    if (result == INTERPRET_OK) {
        table_add_all(curr_closure_global, &vm.builtin, true);
    } else {
        IMPLEMENTATION_ERROR("Error when loading lib");
    }
    return result;
}


/**
 * 使用给定的函数，构造一个closure，设置栈帧，将closure置入栈中。
 * 设置vm.current_module, curr_frame, curr_closure_global, curr_const_pool等值。
 * @param function 要执行的main函数
 * @param path_chars 该模块的路径。如果为NULL，则改为使用path_string
 * @param path_string 如果path_chars为NULL，那么该模块的路径由该值决定
 * @param care_repl 如果 care_repl && REPL，则不创建新的模块，而是使用先前保存的repl_module
 */
static void warmup(LoxFunction *function, const char *path_chars, String *path_string, bool care_repl) {

    if (preload_started || COMPILE_ONLY) {

    } else {
        preload_started = true;
        load_libraries();
        preload_finished = true;
    }

    vm.frame_count++;

    curr_frame = vm.frames + vm.frame_count - 1;

    Closure *closure = new_closure(function);
    curr_frame->closure = closure;
    curr_frame->FP = vm.stack_top;
    curr_frame->PC = function->chunk.code;

    Module *module;
    if (care_repl && REPL) {
        module = repl_module;
    } else if (path_chars != NULL) {
        module = new_module(auto_length_string_copy(path_chars));
    } else {
        module = new_module(path_string);
    }

    closure->module = module;
    vm.current_module = module;
    curr_closure_global = &closure->module->globals;
    curr_const_pool = closure->function->chunk.constants.values;

    stack_push(ref_value((Object *) closure));
}

static void build_array(int length) {
    Array *array = new_array(length);
    for (int i = length - 1; i >= 0; i--) {
        array->values[i] = stack_pop();
    }
    stack_push(ref_value((Object *) array));
}

/**
 * 遍历虚拟机的 curr_frame 的 function 的 chunk 中的每一个指令，执行之
 * 如果发生了runtime error，该函数将立刻返回。
 * @return 执行的结果
 */
static InterpretResult run_vm() {
    InterpretResult error_code;
    if ( (error_code = setjmp(error_buf))) {
        //  运行时错误会跳转至这里
        return error_code;
    }
    while (true) {
        if (TRACE_EXECUTION && TRACE_SKIP == -1 && preload_finished) {
            show_stack();
            if (getchar() == 'o') {
                TRACE_SKIP = vm.frame_count - 1; // skip until the frame_count is equal to TRACE_SKIP
                while (getchar() != '\n');
            } else {
                CallFrame *frame = curr_frame;
                disassemble_instruction(&frame->closure->function->chunk, (int) (frame->PC - frame->closure->function->chunk.code));
            }
        }

        uint8_t instruction = read_byte();
        switch (instruction) {
            case OP_RETURN: {
                Value result = stack_pop(); // 返回值
                vm.stack_top = curr_frame->FP;
                close_upvalue(vm.stack_top);
                vm.frame_count--;
                curr_frame = vm.frames + vm.frame_count - 1;
                if (vm.frame_count == TRACE_SKIP) {
                    TRACE_SKIP = -1; // no skip. Step by step
                }
                if (vm.frame_count == 0) {
                    return INTERPRET_OK; // 程序运行结束
                }
                curr_const_pool = curr_frame->closure->function->chunk.constants.values;
                curr_closure_global = & curr_frame->closure->module->globals;
                stack_push(result);
                break;
            }
            case OP_CONSTANT: {
                Value value = read_constant16();
                stack_push(value);
                break;
            }
            case OP_CONSTANT2: {
                Value value = read_constant16();
                stack_push(value);
                break;
            }
            case OP_NEGATE: {
                Value value = stack_peek(0);
                if (is_int(value)) {
                    stack_push(int_value(-as_int(stack_pop())));
                    break;
                } else if (is_float(value)) {
                    stack_push(float_value(-as_float(stack_pop())));
                    break;
                } else {
                    runtime_error_and_catch("the value of type: %d is cannot be negated", value.type);
                }
                break;
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
                    runtime_error_catch_2("the operands, %s and %s, do not support the power operation", a, b);
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
            case OP_REPL_AUTO_PRINT: {
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
            case OP_DEF_GLOBAL: {
                String *name = read_constant_string();
                if (! table_add_new(curr_closure_global, name, stack_peek(0), false, false)) {
                    runtime_error_and_catch("re-defining the existent global variable %s", name->chars);
                }
                stack_pop();
                break;
            }
            case OP_DEF_GLOBAL_CONST: {
                String *name = read_constant_string();
                if (!table_add_new(curr_closure_global, name, stack_peek(0), false, true)) {
                    runtime_error_and_catch("re-defining the existent global variable %s", name->chars);
                }
                stack_pop();
                break;
            }
            case OP_DEF_PUB_GLOBAL: {
                String *name = read_constant_string();
                if (! table_add_new(curr_closure_global, name, stack_peek(0), true, false)) {
                    runtime_error_and_catch("re-defining the existent global variable %s", name->chars);
                }
                stack_pop();
                break;
            }
            case OP_DEF_PUB_GLOBAL_CONST: {
                String *name = read_constant_string();
                if (! table_add_new(curr_closure_global, name, stack_peek(0), true, true)) {
                    runtime_error_and_catch("re-defining the existent global variable %s", name->chars);
                }
                stack_pop();
                break;
            }
            case OP_GET_GLOBAL: {
                String *name = read_constant_string();
                Value value;
                if (table_get(curr_closure_global, name, &value)) {
                    stack_push(value);
                } else if (table_get(&vm.builtin, name, &value)) {
                    stack_push(value);
                } else {
                    runtime_error_and_catch("Accessing an undefined variable: %s", name->chars);
                }
                break;
            }
            case OP_SET_GLOBAL: {
                String *name = read_constant_string();
                char result = table_set_existent(curr_closure_global, name, stack_peek(0), false);
                if (result != 0) {
                    if (result == 1) {
                        runtime_error_and_catch("Setting an undefined variable: %s", name->chars);
                    } else {
                        runtime_error_and_catch("Setting a const variable: %s", name->chars);
                    }
                }
                break;
            }
            case OP_GET_LOCAL: {
                int index = read_byte();
                stack_push(curr_frame->FP[index]);
                break;
            }
            case OP_SET_LOCAL: {
                int index = read_byte();
                curr_frame->FP[index] = stack_peek(0);
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = read_uint16();
                if (is_falsy(stack_peek(0))) {
                    curr_frame->PC += offset;
                }
                break;
            }
            case OP_JUMP_IF_TRUE: {
                uint16_t offset = read_uint16();
                if (!is_falsy(stack_peek(0))) {
                    curr_frame->PC += offset;
                }
                break;
            }
            case OP_JUMP: {
                uint16_t offset = read_uint16();
                curr_frame->PC += offset;
                break;
            }
            case OP_JUMP_BACK: {
                uint16_t offset = read_uint16();
                curr_frame->PC -= offset;
                break;
            }
            case OP_JUMP_IF_NOT_EQUAL: {
                uint16_t offset = read_uint16();
                Value b = stack_peek(0);
                Value a = stack_peek(1);
                if (!value_equal(a, b)) {
                    curr_frame->PC += offset;
                }
                break;
            }
            case OP_JUMP_IF_FALSE_POP: {
                uint16_t offset = read_uint16();
                if (is_falsy(stack_pop())) {
                    curr_frame->PC += offset;
                }
                break;
            }
            case OP_JUMP_IF_TRUE_POP: {
                uint16_t offset = read_uint16();
                if (!is_falsy(stack_pop())) {
                    curr_frame->PC += offset;
                }
                break;
            }
            case OP_CALL: {
                int count = read_byte();
                Value callee = stack_peek(count);
                call_value(callee, count);
                break;
            }
            case OP_CLOSURE: {
                /* 值得注意的是，对于嵌套的closure，虽然在编译时，是内部的
                 * 函数先编译，但是在运行时，是先执行外层函数的OP_closure。
                 * 然后等到外层函数被调用、内层函数被定义后，内层函数的OP_closure
                 * 才会被执行
                 */
                Value f = read_constant16();
                Closure *closure = new_closure(as_function(f));
                closure->module = vm.current_module;
                stack_push(ref_value((Object *) closure));
                for (int i = 0; i < closure->upvalue_count; ++i) {
                    bool is_local = read_byte();
                    int index = read_byte();
                    if (is_local) {
                        closure->upvalues[i] = capture_upvalue(curr_frame->FP + index);
                    } else {
                        closure->upvalues[i] = curr_frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_GET_UPVALUE: {
                int index = read_byte();
                stack_push(*curr_frame->closure->upvalues[index]->position);
                break;
            }
            case OP_SET_UPVALUE: {
                int index = read_byte();
                *curr_frame->closure->upvalues[index]->position = stack_peek(0);
                break;
            }
            case OP_CLOSE_UPVALUE: {
                close_upvalue(vm.stack_top - 1);
                stack_pop();
                break;
            }
            case OP_CLASS: {
                String *name = read_constant_string();
                stack_push(ref_value((Object *) new_class(name)));
                break;
            }
            case OP_GET_PROPERTY: {
                Value value = stack_pop(); // instance
                String *field_name = read_constant_string();
                if (is_ref_of(value, OBJ_INSTANCE) == false) {
                    switch (as_ref(value)->type) {
                        case OBJ_ARRAY: {
                            if (field_name == LENGTH) {
                                Array *array = as_array(value);
                                stack_push(int_value(array->length));
                                break;
                            } else {
                                goto OP_GET_PROPERTY_not_found;
                            }
                            break;
                        }
                        case OBJ_CLASS: {
                            Class *class = as_class(value);
                            Value static_field;
                            if (!table_get(&class->static_fields, field_name, &static_field)) {
                                goto OP_GET_PROPERTY_not_found;
                            }
                            stack_push(static_field);
                            break;
                        }
                        case OBJ_MODULE: {
                            Module *module = as_module(value);
                            Value property;
                            if (!table_conditional_get(&module->globals, field_name, &property, true, false)) {
                                runtime_error_catch_2("%s does not the have public property: %s", value, ref_value((Object *) field_name));
                            } else {
                                stack_push(property);
                            }
                            break;
                        }
                        OP_GET_PROPERTY_not_found:
                        default:
                            runtime_error_catch_2("%s does not have the property: %s", value, ref_value((Object *) field_name));
                    }
                } else {
                    Instance *instance = as_instance(value);
                    Value result = nil_value();
                    bool found = table_get(&instance->fields, field_name, &result);
                    if (found == false) {
                        Method *method = bind_method(instance->class, field_name, value);
                        result = ref_value((Object *) method);
                    }
                    stack_push(result);
                }
                break;
            }
            case OP_SET_PROPERTY: {
                Value target = stack_peek(1); // prevent being gc
                Value value = stack_peek(0);
                String *property_name = read_constant_string();
                if (is_ref_of(target, OBJ_INSTANCE) == false) {
                    if (is_ref_of(target, OBJ_CLASS)) {
                        Class *class = as_class(target);
                        if (table_set_existent(&class->static_fields, property_name, value, false)) {
                            runtime_error_catch_2("%s does not have the static field: %s", ref_value((Object *) class), ref_value((Object *) property_name));
                        }
                        break;
                    } else if (is_ref_of(target, OBJ_MODULE)) {
                        Module *module = as_module(target);
                        char result = table_set_existent(&module->globals, property_name, value, true);
                        switch (result) {
                            case 0:
                                break;
                            case 1:
                                runtime_error_catch_2("%s does not have the property: %s", target, ref_value((Object *) property_name));
                                break;
                            case 2:
                                runtime_error_and_catch("cannot modify the const property: %s", property_name->chars);
                                break;
                            case 3:
                                runtime_error_and_catch("cannot access the non-public property: %s", property_name->chars);
                                break;
                            default:
                                IMPLEMENTATION_ERROR("bad");
                                break;
                        }
                    } else {
                        runtime_error_catch_2("%s does not have the property: %s", target,
                                              ref_value((Object *) property_name));
                    }
                } else {
                    Instance *instance = as_instance(target);
                    table_set(&instance->fields, property_name, value); // potential gc
                    vm.stack_top -= 2;
                    stack_push(value);
                }
                break;
            }
            case OP_METHOD: {
                // stack: class, closure,
                // op-method
                Closure *closure = as_closure(stack_peek(0));
                Class *class = as_class(stack_peek(1));
                table_set(&class->methods, closure->function->name, ref_value((Object *) closure));
                stack_pop();
                break;
            }
            case OP_PROPERTY_INVOKE: {
                // stack: [obj, arg1, arg2, top]
                // code: op, name_index, arg_count
                String *name = read_constant_string();
                int arg_count = read_byte();
                Value receiver = stack_peek(arg_count);
                if (!is_ref_of(receiver, OBJ_INSTANCE)) {
                    if (is_ref_of(receiver, OBJ_CLASS)) {
                        Class *class = as_class(receiver);
                        Value static_field;
                        if (table_get(&class->static_fields, name, &static_field)) {
                            call_value(static_field, arg_count);
                            break;
                        }
                    } else if (is_ref_of(receiver, OBJ_MODULE)) {
                        Module *module = as_module(receiver);
                        Value property;
                        if (table_conditional_get(&module->globals, name, &property, true, false)) {
                            call_value(property, arg_count);
                            break;
                        }
                    } else if (is_ref_of(receiver, OBJ_ARRAY)) {
                        invoke_from_class(as_class(array_class), name, arg_count);
                        break;
                    }
                    runtime_error_catch_2("%s does not have the property: %s", receiver, ref_value((Object *) name));
                }
                Instance *instance = as_instance(receiver);
                Value closure_value;
                if (!table_get(&instance->fields, name, &closure_value)) {
                    Class *class = instance->class;
                    invoke_from_class(class, name, arg_count);
                } else {
                    call_closure(as_closure(closure_value), arg_count);
                }
                break;
            }
            case OP_INHERIT: {
                //  super,sub,top
                Value super = stack_peek(1);
                if (!is_ref_of(super, OBJ_CLASS)) {
                    runtime_error_catch_1("%s cannot be used as a super class", super);
                }
                Class *sub = as_class(stack_peek(0));
                Class *super_class = as_class(super);
                table_add_all(&super_class->methods, &sub->methods, false);
                stack_pop();
                // super, top
                break;
            }
            case OP_SUPER_ACCESS: {
                // stack: receiver, super_class, top
                // code: op, name_index
                String *name = read_constant_string();
                Class *class = as_class(stack_pop());
                Value receiver = stack_pop();
                Method *method = bind_method(class, name, receiver);
                stack_push(ref_value((Object *) method));
                // stack: method, top
                break;
            }
            case OP_SUPER_INVOKE: {
                // [receiver, args1, arg2, superclass, top]
                // op, name, arg_count
                String *name = read_constant_string();
                int arg_count = read_byte();
                Class *class = as_class(stack_pop());
                invoke_from_class(class, name, arg_count);
                break;
            }
            case OP_DIMENSION_ARRAY: {
                // [len1, len2, top]
                int dimension = read_byte();
                Value arr = multi_dimension_array(dimension, vm.stack_top - dimension);
                for (int i = 0; i < dimension; ++i) {
                    stack_pop();
                }
                stack_push(arr);
                break;
            }
            case OP_COPY: {
                stack_push(stack_peek(0));
                break;
            }
            case OP_COPY2: {
                stack_push(stack_peek(1));
                stack_push(stack_peek(1));
                break;
            }
            case OP_INDEXING_GET: {
                // [arr, index, top]
                Value index_value = stack_pop();
                if (!is_int(index_value)) {
                    runtime_error_catch_1("%s is not an integer and cannot be used as an index", index_value);
                }
                Value arr_value = stack_pop();
                if (!is_ref_of(arr_value, OBJ_ARRAY)) {
                    runtime_error_catch_1("%s is not an array and does not support indexing", arr_value);
                }
                Array *array = as_array(arr_value);
                int index = as_int(index_value);
                if (index < 0 || index >= array->length) {
                    runtime_error_and_catch("index %d is out of bound: [0, %d]", index, array->length - 1);
                }
                stack_push(array->values[index]);
                break;
            }
            case OP_INDEXING_SET: {
                // op: [array, index, value, top]
                Value value = stack_pop();
                Value index_value = stack_pop();
                if (!is_int(index_value)) {
                    runtime_error_catch_1("%s is not an integer and cannot be used as an index", index_value);
                }
                Value arr_value = stack_pop();
                if (!is_ref_of(arr_value, OBJ_ARRAY)) {
                    runtime_error_catch_1("%s is not an array and does not support indexing", arr_value);
                }
                Array *array = as_array(arr_value);
                int index = as_int(index_value);
                if (index < 0 || index >= array->length) {
                    runtime_error_and_catch("index %d is out of bound: [0, %d]", index, array->length - 1);
                }
                array->values[index] = value;
                stack_push(value);
                break;
            }
            case OP_BUILD_ARRAY: {
                int length = read_byte();
                build_array(length);
//                Array *array = new_array(length);
//                for (int i = length - 1; i >= 0; i--) {
//                    array->values[i] = stack_pop();
//                }
//                stack_push(ref_value((Object *) array));
                break;
            }
            case OP_UNPACK_ARRAY: {
//                int length = read_byte();
                read_byte();
                break;
            }
            case OP_CLASS_STATIC_FIELD: {
                // [class, field, top]
                String *name = read_constant_string();
                Value field = stack_peek(0);
                Class *class = as_class(stack_peek(1));
                table_add_new(&class->static_fields, name, field, false, false);
                stack_pop();
                break;
            }
            case OP_IMPORT: {
                String *relative = as_string(stack_pop());
                char *relative_path;
                asprintf(&relative_path, "%s/../%s", vm.current_module->path->chars, relative->chars);
                char *resolved_path = resolve_path(relative_path);
                char *src = read_file(resolved_path);
                if (src == NULL) {
                    runtime_error_and_catch("error when reading the file %s (%s)\n", resolved_path, relative_path);
                }
                free(relative_path);
                String *path_string = auto_length_string_copy(resolved_path);
                import(src, path_string);
                free(src);
                break;
            }
            case OP_COPY_N: {
                int n = read_byte();
                stack_push(stack_peek(n));
                break;
            }
            case OP_SWAP: {
                int n = read_byte();
                Value temp = stack_peek(n);
                vm.stack_top[-1-n] = stack_peek(0);
                vm.stack_top[-1] = temp;
                break;
            }
            case OP_RESTORE_MODULE: {
                // [old_module, nil, top] -> [new_module, top]
                stack_pop();
                Module *old_module = as_module(stack_pop());
                stack_push(ref_value((Object *) vm.current_module));
                vm.current_module = old_module;
                curr_closure_global = &curr_frame->closure->module->globals;
//                curr_const_pool = curr_frame->closure->function->chunk.constants.values;
                break;
            }
            case OP_EXPORT: {
                String *name = read_constant_string();
                Entry *entry = table_find_entry(&vm.current_module->globals, name, false, false);
                if (entry->key == NULL && is_bool(entry->value)) {
                    runtime_error_and_catch("No such global variable: %s", name->chars);
                } else {
                    entry->is_public = true;
                }
                break;
            }
            default: {
                runtime_error_and_catch("unrecognized instruction");
                break;
            }
        }
    }
}
