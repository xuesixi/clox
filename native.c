//
// Created by Yue Xue  on 12/23/24.
//

#include "native.h"
#include "object.h"
#include "liblox_core.h"
#include "liblox_iter.h"
#include "vm.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"

#define FNV_PRIME 16777619
#define FNV_OFFSET_BASIS 2166136261

Class *array_class;
Class *string_class;
Class *int_class;
Class *float_class;
Class *bool_class;
Class *native_class;
Class *class_class;
Class *function_class;
Class *closure_class;
Class *map_class;
Class *method_class;
Class *nil_class;
Class *module_class;
Class *native_object_class;

Class *Error;
Class *TypeError;
Class *IndexError;
Class *ArgError;
Class *NameError;
Class *PropertyError;
Class *ValueError;

String *INIT = NULL;
String *LENGTH = NULL;
String *ITERATOR = NULL;
String *HAS_NEXT = NULL;
String *NEXT = NULL;
String *EQUAL = NULL;
String *HASH = NULL;
String *MESSAGE = NULL;
String *POSITION = NULL;


static Value native_backtrace(int count, Value *value) {
    (void ) count;
    (void ) value;
    int total_len = 0;
    char **css = malloc(sizeof(char *) * vm.frame_count);
    int *lens = malloc(sizeof(int) * vm.frame_count);

    for (int i = vm.frame_count - 1; i >= 0; i--) {
        CallFrame *frame = vm.frames + i;
        LoxFunction *function = frame->closure->function;
        size_t index = frame->PC - frame->closure->function->chunk.code - 1;
        int line = function->chunk.lines[index];
        char *name = value_to_chars(ref_value((Object *) frame->closure), NULL);
        lens[i] = asprintf(css + i, "at [line %d] in %s\n", line, name);
        free(name);
        total_len += lens[i];
    }
    char *result = malloc(total_len + 1);
    char *curr = result;
    for (int i = vm.frame_count - 1; i >= 0; i --) {
        memcpy(curr, css[i], lens[i]);
        curr += lens[i];
        free(css[i]);
    }
    result[total_len] = '\0';
    free(css);
    free(lens);
    String *str = string_copy(result, total_len);
    return ref_value((Object *) str);
}

/**
 * 生成一个新的指定类型的error对象，然后将其置入栈顶。
 */
void new_error(ErrorType type, const char *message) {
    Class *error_class;
    switch (type) {
        case Error_Error:
            error_class = Error;
            break;
        case Error_IndexError:
            error_class = IndexError;
            break;
        case Error_NameError:
            error_class = NameError;
            break;
        case Error_PropertyError:
            error_class = PropertyError;
            break;
        case Error_TypeError:
            error_class = TypeError;
            break;
        case Error_ValueError:
            error_class = ValueError;
            break;
        case Error_ArgError:
            error_class = ArgError;
            break;
    }
    Instance *error_instance = new_instance(error_class);
    stack_push(ref_value((Object *)error_instance));
    String *message_str = auto_length_string_copy(message);
    stack_push(ref_value((Object *) message_str));
    Value bt = native_backtrace(0, NULL);
    stack_push(bt); // err, message, bt
    table_set(&error_instance->fields, MESSAGE, ref_value((Object*) message_str));
    table_set(&error_instance->fields, POSITION, bt);
    stack_pop();
    stack_pop();
}

static void define_native(const char *name, NativeImplementation impl, int arity) {
    int len = (int) strlen(name);

    stack_push(ref_value((Object *) string_copy(name, len)));
    stack_push(ref_value((Object *) new_native(impl, as_string(vm.stack[0]), arity)));
    table_add_new(&vm.builtin, as_string(vm.stack[0]), vm.stack[1], true, false);
    stack_pop();
    stack_pop();
}

/**
 * 如果 LOAD_LIB 不为false，则加载标准库（执行字节码），将标准库的成员导入builtin命名空间中。
 */
void load_libraries() {
    if (LOAD_LIB) {
        if (load_bytes_into_builtin(liblox_iter, liblox_iter_len, "lib_iter") != INTERPRET_EXECUTE_OK) {
            exit(1);
        }
        if (load_bytes_into_builtin(liblox_core, liblox_core_len, "lib_core") != INTERPRET_EXECUTE_OK) {
            exit(1);
        }
        Value class_value;

        table_get(&vm.builtin, auto_length_string_copy("Array"), &class_value);
        array_class = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("String"), &class_value);
        string_class = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("Int"), &class_value);
        int_class = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("Float"), &class_value);
        float_class = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("Bool"), &class_value);
        bool_class = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("Native"), &class_value);
        native_class = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("Class"), &class_value);
        class_class = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("Function"), &class_value);
        function_class = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("Closure"), &class_value);
        closure_class= as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("Method"), &class_value);
        method_class = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("Module"), &class_value);
        module_class = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("Nil"), &class_value);
        nil_class = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("Map"), &class_value);
        map_class = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("NativeObject"), &class_value);
        native_object_class = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("Error"), &class_value);
        Error = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("TypeError"), &class_value);
        TypeError = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("ArgError"), &class_value);
        ArgError = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("IndexError"), &class_value);
        IndexError = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("NameError"), &class_value);
        NameError = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("PropertyError"), &class_value);
        PropertyError = as_class(class_value);

        table_get(&vm.builtin, auto_length_string_copy("ValueError"), &class_value);
        ValueError = as_class(class_value);

    }
    preload_finished = true;
}

static Value native_type(int count, Value *value) {
    (void ) count;
    return ref_value((Object *)value_class(*value));
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

static Value native_range(int count, Value *value) {
    // curr, limit, step
    (void ) count;
    NativeObject *native_object = new_native_object(NativeRangeIter, 3);
    native_object->values[2] = value[2]; // step
    native_object->values[0] = int_value(as_int(value[0]) - as_int(value[2])); // curr
    native_object->values[1] = int_value(as_int(value[1]) - as_int(value[2])); // limit
    return ref_value((Object *) native_object);
}

static Value native_array_iter(int count, Value *value) {
    (void ) count;
    // curr, array
    assert_ref_type(*value, OBJ_ARRAY, "array");
    NativeObject *nativeObject = new_native_object(NativeArrayIter, 2);
    nativeObject->values[0] = int_value(0);
    nativeObject->values[1] = *value;
    return ref_value((Object *) nativeObject);
}

static Value general_hash(int count, Value *given) {
    (void ) count;
    uint32_t hash = FNV_OFFSET_BASIS;
    Value v = *given;
    switch(given->type) {
        case VAL_INT: { // int
            int value = as_int(v);
            const unsigned char* bytes = (const unsigned char*)&value;
            for (size_t i = 0; i < sizeof(int); i++) {
                hash ^= bytes[i];
                hash *= FNV_PRIME;
            }
            break;
        }
        case VAL_FLOAT: { // double
            double value = as_float(v);
            const unsigned char* bytes = (const unsigned char*)&value;
            for (size_t i = 0; i < sizeof(double); i++) {
                hash ^= bytes[i];
                hash *= FNV_PRIME;
            }
            break;
        }
        case VAL_BOOL: { // bool
            bool value = as_bool(v);
            hash ^= value ? 1 : 0;
            hash *= FNV_PRIME;
            break;
        }
        case VAL_NIL: {
            return int_value(0);
        }
        case VAL_REF: { // pointer
            if (is_ref_of(v, OBJ_STRING)) {
                String *string = as_string(v);
                hash = chars_hash(string->chars, string->length);
            } else {
                uintptr_t ptr = (uintptr_t) as_ref(v);
                const unsigned char* bytes = (const unsigned char*)&ptr;
                for (size_t i = 0; i < sizeof(void*); i++) {
                    hash ^= bytes[i];
                    hash *= FNV_PRIME;
                }
            }
            break;
        }
        default:
            IMPLEMENTATION_ERROR("bad");
    }

    return int_value(hash);
}

Value native_value_equal(int count, Value *values) {
    (void ) count;
    Value a = values[0];
    Value b = values[1];
    if (a.type != b.type) {
        return bool_value(false);
    }
    bool result;
    switch (a.type) {
        case VAL_BOOL:
            result = as_bool(a) == as_bool(b);
            break;
        case VAL_INT:
            result = as_int(a) == as_int(b);
            break;
        case VAL_FLOAT:
            result =  as_float(a) == as_float(b);
            break;
        case VAL_NIL:
        case VAL_ABSENCE:
            result = true;
            break;
        case VAL_REF: {
            result = as_ref(a) == as_ref(b);
            break;
        }
    }
    return bool_value(result);
}

Value native_map_iter(int count, Value *value) {
    (void ) count;
    assert_ref_type(*value, OBJ_MAP, "map");
    NativeObject *nativeObject = new_native_object(NativeMapIter, 2);
    // curr, map
    nativeObject->values[0] = int_value(0);
    nativeObject->values[1] = *value;
    return ref_value((Object*) nativeObject);
}

void init_vm_native() {
    define_native("clock", native_clock, 0);
    define_native("int", native_int, 1);
    define_native("float", native_float, 1);
    define_native("rand", native_rand, 2);
    define_native("f", native_format, -1);
    define_native("read", native_read, -1);
    define_native("char_at", native_char_at, 2);
    define_native("type", native_type, 1);
    define_native("native_string_combine_array", native_string_combine_array, 1);
    define_native("native_value_join", native_value_join, 4);
    define_native("native_string_join", native_string_join, 4);
    define_native("native_range", native_range, 3);
    define_native("native_array_iter", native_array_iter, 1);
    define_native("native_map_iter", native_map_iter, 1);
    define_native("native_general_hash", general_hash, 1);
    define_native("native_value_equal", native_value_equal, 2);
    define_native("backtrace", native_backtrace, 0);
}

void additional_repl_init() {
    define_native("help", native_help, 0);
    define_native("exit", native_exit, -1);
}

void init_static_strings() {
    INIT = auto_length_string_copy("init");
    LENGTH = auto_length_string_copy("length");
    ITERATOR = auto_length_string_copy("iterator");
    HAS_NEXT = auto_length_string_copy("has_next");
    NEXT = auto_length_string_copy("next");
    EQUAL = auto_length_string_copy("equal");
    HASH = auto_length_string_copy("hash");
    MESSAGE = auto_length_string_copy("message");
    POSITION = auto_length_string_copy("position");
}

