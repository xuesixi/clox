//
// Created by Yue Xue  on 12/23/24.
//

#include "native.h"
#include "liblox_core.h"
#include "liblox_iter.h"
#include "vm.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"

Class *array_class;
Class *string_class;
Class *int_class;
Class *float_class;
Class *bool_class;
Class *native_class;
Class *class_class;
Class *function_class;
Class *method_class;
Class *nil_class;
Class *module_class;

static void define_native(const char *name, NativeImplementation impl, int arity) {
    int len = (int) strlen(name);

    stack_push(ref_value((Object *) string_copy(name, len)));
    stack_push(ref_value((Object *) new_native(impl, as_string(vm.stack[0]), arity)));
    table_add_new(&vm.builtin, as_string(vm.stack[0]), vm.stack[1], true, false);
    stack_pop();
    stack_pop();
}

/**
 * 加载标准库（执行字节码），将标准库的成员导入builtin命名空间中。
 */
void load_libraries() {
#ifdef LOAD_LIB
    if (load_bytes_into_builtin(liblox_iter, liblox_iter_len, "lib_iter") != INTERPRET_OK) {
        exit(1);
    }
    if (load_bytes_into_builtin(liblox_core, liblox_core_len, "lib_core") != INTERPRET_OK) {
        exit(1);
    }
    Value array_class_value;
    Value string_class_value;
    Value int_class_value;
    Value float_class_value;
    Value bool_class_value;
    Value native_class_value;
    Value class_class_value;
    Value function_class_value;
    Value method_class_value;
    Value module_class_value;
    Value nil_class_value;

    table_get(&vm.builtin, ARRAY_CLASS, &array_class_value);
    array_class = as_class(array_class_value);

    table_get(&vm.builtin, STRING_CLASS, &string_class_value);
    string_class = as_class(string_class_value);

    table_get(&vm.builtin, INT_CLASS, &int_class_value);
    int_class = as_class(int_class_value);

    table_get(&vm.builtin, FLOAT_CLASS, &float_class_value);
    float_class = as_class(float_class_value);

    table_get(&vm.builtin, BOOL_CLASS, &bool_class_value);
    bool_class = as_class(bool_class_value);

    table_get(&vm.builtin, NATIVE_CLASS, &native_class_value);
    native_class = as_class(native_class_value);

    table_get(&vm.builtin, CLASS_CLASS, &class_class_value);
    class_class = as_class(class_class_value);

    table_get(&vm.builtin, FUNCTION_CLASS, &function_class_value);
    function_class = as_class(function_class_value);

    table_get(&vm.builtin, METHOD_CLASS, &method_class_value);
    method_class = as_class(method_class_value);

    table_get(&vm.builtin, MODULE_CLASS, &module_class_value);
    module_class = as_class(module_class_value);

    table_get(&vm.builtin, NIL_CLASS, &nil_class_value);
    nil_class = as_class(nil_class_value);

    preload_finished = true;
#endif
}

static Value native_type(int count, Value *value) {
    (void ) count;
    switch (value->type) {
        case VAL_NIL:
            return ref_value((Object *)nil_class);
        case VAL_INT:
            return ref_value((Object *)int_class);
        case VAL_FLOAT:
            return ref_value((Object *)float_class);
        case VAL_BOOL:
            return ref_value((Object *)bool_class);
        case VAL_REF: {
            switch (as_ref(*value)->type) {
                case OBJ_STRING:
                    return ref_value((Object *)string_class);
                case OBJ_ARRAY:
                    return ref_value((Object *)array_class);
                case OBJ_CLASS:
                    return ref_value((Object *)class_class);
                case OBJ_NATIVE:
                    return ref_value((Object *)native_class);
                case OBJ_METHOD:
                    return ref_value((Object *)method_class);
                case OBJ_CLOSURE:
                    return ref_value((Object *)function_class);
                case OBJ_MODULE:
                    return ref_value((Object *)module_class);
                case OBJ_INSTANCE: {
                    Instance *instance = as_instance(*value);
                    return ref_value((Object *)instance->class);
                }
                default:
                    IMPLEMENTATION_ERROR("should not happen");
                    return nil_value();
            }
        }
        case VAL_ABSENCE:
            IMPLEMENTATION_ERROR("should not happen");
            return nil_value();
    }
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

static Value native_string_combine(int count, Value *values) {
    int total_len = 0;
    char **css = malloc(sizeof(char *) * count);
    int *lens = malloc(sizeof(int ) * count);
    for (int i = 0; i < count; ++i) {
        css[i] = value_to_chars(values[i], lens + i);
        total_len += lens[i];
    }
    char *result= malloc(total_len + 1);
    char *curr = result;
    for (int i = 0; i < count; ++i) {
        memcpy(curr, css[i], lens[i]);
        curr += lens[i];
        free(css[i]);
    }
    result[total_len] = '\0';
    free(lens);
    free(css);
    String *str = string_allocate(result, total_len);
    return ref_value((Object *) str);
}

static Value native_string_combine_array(int count, Value *value) {
    (void ) count;
    Value v = *value;
    if (!is_ref_of(v, OBJ_ARRAY)) {
        runtime_error_catch_1("The function only support one array as the argument", v);
    }
    Array *array = as_array(v);
    return native_string_combine(array->length, array->values);
}

static Value native_string_join(int count, Value *values) {
    // 0: delimiter, 1: prefix, 2: suffix, 3: array
    (void ) count;
    Value v0 = values[0];
    Value v1 = values[1];
    Value v2 = values[2];
    Value v3 = values[3];
    assert_ref_type(v0, OBJ_STRING, "string");
    assert_ref_type(v1, OBJ_STRING, "string");
    assert_ref_type(v2, OBJ_STRING, "string");
    assert_ref_type(v3, OBJ_ARRAY, "array");
    String *delimiter = as_string(v0);
    String *prefix = as_string(v1);
    String *suffix = as_string(v2);
    Array *array = as_array(v3);
    int array_len = array->length;

    int total_len = prefix->length + suffix->length + delimiter->length * (array_len - 1);

    for (int i = 0; i < array_len; ++i) {
        Value v = array->values[i];
        assert_ref_type(v, OBJ_STRING, "string");
        total_len += as_string(v)->length;
    }
    char *result= malloc(total_len + 1);
    char *curr = result;
    memcpy(curr, prefix->chars, prefix->length);
    curr += prefix->length;
    for (int i = 0; i < array_len; ++i) {
        String *s = as_string(array->values[i]);
        memcpy(curr, s->chars, s->length);
        curr += s->length;
        if (i != array_len - 1) {
            memcpy(curr, delimiter->chars, delimiter->length);
            curr += delimiter->length;
        }
    }
    memcpy(curr, suffix->chars, suffix->length);
    result[total_len] = '\0';
    String *str = string_allocate(result, total_len);
    return ref_value((Object *) str);
}

static Value native_value_join(int count, Value *values) {
   // 0: delimiter, 1: prefix, 2: suffix, 3: array
    (void ) count;
    Value v0 = values[0];
    Value v1 = values[1];
    Value v2 = values[2];
    Value v3 = values[3];
    assert_ref_type(v0, OBJ_STRING, "string");
    assert_ref_type(v1, OBJ_STRING, "string");
    assert_ref_type(v2, OBJ_STRING, "string");
    assert_ref_type(v3, OBJ_ARRAY, "array");
    String *delimiter = as_string(v0);
    String *prefix = as_string(v1);
    String *suffix = as_string(v2);
    Array *array = as_array(v3);
    int array_len = array->length;

    int total_len = prefix->length + suffix->length + delimiter->length * (array_len - 1);
    char **css = malloc(sizeof(char *) * array_len);
    int *lens = malloc(sizeof(int ) * array_len);

    for (int i = 0; i < array_len; ++i) {
        css[i] = value_to_chars(array->values[i], lens + i);
        total_len += lens[i];
    }
    char *result= malloc(total_len + 1);
    char *curr = result;
    memcpy(curr, prefix->chars, prefix->length);
    curr += prefix->length;
    for (int i = 0; i < array_len; ++i) {
        memcpy(curr, css[i], lens[i]);
        curr += lens[i];
        free(css[i]);
        if (i != array_len - 1) {
            memcpy(curr, delimiter->chars, delimiter->length);
            curr += delimiter->length;
        }
    }
    memcpy(curr, suffix->chars, suffix->length);
    result[total_len] = '\0';
    free(lens);
    free(css);
    String *str = string_allocate(result, total_len);
    return ref_value((Object *) str);
}

static Value native_char_at(int count, Value *value) {
    (void )count;
    assert_ref_type(value[0], OBJ_STRING, "string");
    assert_value_type(value[1], VAL_INT, "int");
    String *string = as_string(*value);
    int index = as_int(value[1]);
    if (index < 0 || index >= string->length) {
        runtime_error_and_catch("index %d is out of bound: [%d, %d]", index, 0, string->length - 1);
    }
    String *c = string_copy(string->chars+index, 1);
    return ref_value((Object *) c);
}

void init_vm_native() {
    define_native("clock", native_clock, 0);
    define_native("int", native_int, 1);
    define_native("float", native_float, 1);
    define_native("rand", native_rand, 2);
    define_native("f", native_format, -1);
    define_native("native_string_combine_array", native_string_combine_array, 1);
    define_native("native_value_join", native_value_join, 4);
    define_native("native_string_join", native_string_join, 4);
    define_native("read", native_read, -1);
    define_native("char_at", native_char_at, 2);
    define_native("type", native_type, 1);
}

void additional_repl_init() {
    define_native("help", native_help, 0);
    define_native("exit", native_exit, -1);
}
