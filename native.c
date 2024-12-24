//
// Created by Yue Xue  on 12/23/24.
//

#include "native.h"
#include "lib_iter.h"
#include "vm.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"

Value array_iter_class;

static void define_native(const char *name, NativeImplementation impl, int arity) {
    int len = (int) strlen(name);

    stack_push(ref_value((Object *) string_copy(name, len)));
    stack_push(ref_value((Object *) new_native(impl, as_string(vm.stack[0]), arity)));
    table_add_new(&vm.builtin, as_string(vm.stack[0]), vm.stack[1], true, false);
    stack_pop();
    stack_pop();
}

/**
 * 加载标准库（执行字节码）。有内建机制来避免重复加载，即使多次调用也只有第一次回生效。
 * 在仅编译模式(COMPILE_ONLY)下，该函数没有任何效果。
 */
void load_libraries() {
    static bool loaded = false;
    if (loaded || COMPILE_ONLY) {
        return;
    }
    loaded = true;
    if (load_bytes(liblox_iter, liblox_iter_len, "lib_iter") != INTERPRET_OK) {
        exit(1);
    }
    table_get(&vm.builtin, ARRAY_ITERATOR, &array_iter_class);
}

/**
 * read a line. Return it as a String. the \n will not be included
 * */
static Value native_read(int count, Value *values) {
    if (count > 0) {
        print_value(*values);
    }
    size_t len;
    char *line;
    getline(&line, &len, stdin);
    String *str = string_copy(line, len - 1); // NOLINT
    free(line);
    return ref_value((Object *) str);
}

static inline int max(int a, int b) {
    return a > b ? a : b;
}

static Value native_format(int count, Value *values) {
    const char *format = as_string(*values)->chars;
    static int capacity = 0;
    static char *buf = NULL;
    int pre = 0;
    int curr = 0;
    int buf_len = 0;
    int curr_v = 0;

    while (true) {
        while (format[curr] != '\0' && format[curr] != '#') {
            curr++;
        }

        int new_len = curr - pre;
        // buf overflow checking
        if (buf_len + new_len > capacity) {
            capacity = max(2 * capacity, buf_len + new_len);
            buf = realloc(buf, capacity); // NOLINT
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
            int v_chars_len;
            char *v_chars = value_to_chars(v, &v_chars_len);

            if (v_chars_len + buf_len > capacity) {
                capacity = max(2 * capacity, v_chars_len + buf_len);
                buf = realloc(buf, capacity); // NOLINT
            }
            memcpy(buf + buf_len, v_chars, v_chars_len);
            buf_len += v_chars_len;
            curr++;
            pre = curr;
            curr_v++;
            free(v_chars);
        }
    }

    String *string = string_copy(buf, buf_len);
    return ref_value((Object *) string);
}

static Value native_clock(int count, Value *value) {
    (void) count;
    (void) value;
    return float_value((double) clock() / CLOCKS_PER_SEC);
}

static Value native_int(int count, Value *value) {
    (void) count;
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
                int result = strtol(str->chars, &end, 10); // NOLINT
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
    (void) count;
    Value v = *value;
    switch (v.type) {
        case VAL_INT:
            return float_value((double) as_int(v));
        case VAL_FLOAT:
            return v;
        case VAL_BOOL:
            return float_value((double) as_bool(v));
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
    (void) count;
    (void) value;
    printf("You are in the REPL mode because you run_vm clox directly without providing any arguments.\n");
    printf("You can also do `clox path/to/script` to run_vm a lox script.\n");
    printf("Or do `clox -h` to see more options\n");
    printf("In this REPL mode, expression results will be printed out automatically in gray color. \n");
    printf("You may also omit the last semicolon for a statement.\n");
    printf("Use exit(), ctrl+C or ctrl+D to quit.\n");
    return nil_value();
}

static Value native_exit(int count, Value *value) {
    (void ) count;
    (void ) value;
    longjmp(error_buf, INTERPRET_REPL_EXIT);
    return nil_value();
}

static Value native_rand(int count, Value *value) {
    (void) count;
    Value a = value[0];
    Value b = value[1];
    if (!is_int(a) || !is_int(b)) {
        runtime_error_and_catch("arguments need to be int");
    }
    return int_value(as_int(a) + rand() % as_int(b)); // NOLINT(*-msc50-cpp)
}

void init_vm_native() {
    define_native("clock", native_clock, 0);
    define_native("int", native_int, 1);
    define_native("float", native_float, 1);
    define_native("rand", native_rand, 2);
    define_native("f", native_format, -1);
    define_native("read", native_read, -1);

//    load_libraries();
}

void additional_repl_init() {
    define_native("help", native_help, 0);
    define_native("exit", native_exit, -1);
}
