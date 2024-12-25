//
// Created by Yue Xue  on 11/14/24.
//

#include "value.h"

#include <string.h>

#include "memory.h"
#include "object.h"

char *RED = "\033[31m";
char *BOLD_RED = "\033[31;1m";
char *GREEN = "\033[32m";
char *BOLD_GREEN = "\033[1;32m";
char *CYAN = "\033[36m";
char *BOLD_CYAN = "\033[1;36m";
char *GRAY = "\033[0;90m";
char *MAGENTA = "\033[35m";
char *BLUE = "\033[38;5;32m";
char *YELLOW = "\033[1;33m";
char *BOLD_BLUE = "\033[1;94m";
char *BOLD_MAGENTA = "\033[1;95m";

inline void start_color(char *color) {
    printf("%s", color);
}

inline void end_color() {
    printf("\033[0m");
}

void print_value_with_color(Value value) {
    char *str = value_to_chars(value, NULL);
    switch (value.type) {
        case VAL_INT:
        case VAL_FLOAT:
        case VAL_NIL:
        case VAL_BOOL:
            start_color(YELLOW);
            break;
        case VAL_ABSENCE:
            start_color(GRAY);
            break;
        case VAL_REF: {
            switch (as_ref(value)->type) {
                case OBJ_STRING:
                    start_color(MAGENTA);
                    break;
                case OBJ_CLOSURE:
                case OBJ_NATIVE:
                case OBJ_FUNCTION:
                case OBJ_METHOD:
                    start_color(BLUE);
                    break;
                case OBJ_UPVALUE:
                    break;
                case OBJ_CLASS:
                case OBJ_MODULE:
                    start_color(BOLD_BLUE);
                    break;
                case OBJ_INSTANCE:
                case OBJ_ARRAY:
                    start_color(BOLD_CYAN);
                    break;
            }
            break;
        }
    }
    printf("%s", str);
    end_color();
    free(str);
}

static char *ref_to_chars(Value value, int *len);

void init_ValueArray(ValueArray *array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void append_ValueArray(ValueArray *array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = array->capacity < 8 ? 8 : 2 * array->capacity;
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void free_ValueArray(ValueArray *array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    init_ValueArray(array);
}

bool value_equal(Value a, Value b) {
    if (a.type != b.type) {
        return false;
    }
    switch (a.type) {
        case VAL_BOOL:
            return as_bool(a) == as_bool(b);
        case VAL_INT:
            return as_int(a) == as_int(b);
        case VAL_FLOAT:
            return as_float(a) == as_float(b);
        case VAL_NIL:
        case VAL_ABSENCE:
            return true;
        case VAL_REF:
            return as_ref(a) == as_ref(b);
        default:
            return true;
    }
}

/**
 * 旧版的比较。由于interned string的实现，所有同值的String都具有相同的地址。
 * 因此直接比较地址即可。
 */
bool object_equal(Object *a, Object *b) {
    if (a->type != b->type) {
        return false;
    }
    switch (a->type) {
        case OBJ_STRING: {
            String *a_str = (String *) a;
            String *b_str = (String *) b;
            return a_str->length == b_str->length &&
                   memcmp(a_str->chars, b_str->chars, a_str->length) == 0;
        }
        default:
            return false;
    }
}

static char *ref_to_chars(Value value, int *len) {
    char *buffer = NULL;
    ObjectType type = as_ref(value)->type;
    switch (type) {
        case OBJ_STRING: {
            String *str = as_string(value);
            buffer = malloc(str->length + 1);
            memcpy(buffer, str->chars, str->length + 1);
            *len = str->length;
            break;
        }
        case OBJ_CLOSURE: {
            LoxFunction *fun = as_closure(value)->function;
            Closure *closure = as_closure(value);
            if (fun->type == TYPE_MAIN) {
                *len = asprintf(&buffer, "<main: %s>", get_filename(closure->module->path->chars));
            } else if (fun->type == TYPE_LAMBDA) {
                *len = asprintf(&buffer, "<lambda>");
            } else {
                *len = asprintf(&buffer, "<fn: %s>", fun->name->chars);
            }
            break;
        }
        case OBJ_NATIVE:
            *len = asprintf(&buffer, "<native: %s>", as_native(value)->name->chars);
            break;
        case OBJ_FUNCTION: {
            LoxFunction *fun = as_function(value);
            if (fun->type == TYPE_MAIN) {
                *len = asprintf(&buffer, "<proto: main>");
            } else if (fun->type == TYPE_LAMBDA) {
                *len = asprintf(&buffer, "<proto: lambda>");
            } else {
                *len = asprintf(&buffer, "<proto: %s>", fun->name->chars);
            }
            break;
        }
        case OBJ_UPVALUE:
            *len = asprintf(&buffer, "<upvalue>");
            break;
        case OBJ_CLASS: {
            Class *class = as_class(value);
            *len = asprintf(&buffer, "<cls: %s>", class->name->chars);
            break;
        }
        case OBJ_INSTANCE: {
            Instance *instance = as_instance(value);
            if (instance->class != NULL) {
                *len = asprintf(&buffer, "<obj: %s>", instance->class->name->chars);
            }
            break;
        }
        case OBJ_METHOD: {
            Method *method = as_method(value);
            *len = asprintf(&buffer, "<mthd: %s>", method->closure->function->name->chars);
            break;
        }
        case OBJ_ARRAY: {
            Array *array = as_array(value);
            *len = asprintf(&buffer, "<array: %d>", array->length);
            break;
        }
        case OBJ_MODULE: {
            char *filename = get_filename(as_module(value)->path->chars);
            *len = asprintf(&buffer, "<mod: %s>", filename);
            break;
        }
        default:
            printf("error: encountering a value with unknown type: %d\n", type);
            break;
    }
    return buffer;
}

/**
 * 打印一个 value
 * @param value 想要打印的 value
 */
void print_value(Value value) {
    char *str = value_to_chars(value, NULL);
    printf("%s", str);
    free(str);
}

/**
 * 获取目标 value 的char*表达。调用者需要自己 free 之。
 * 如果传入的len不为NULL，则将生成的字符串长度储存在其中
 * */
char *value_to_chars(Value value, int *len) {
    char *buffer;
    int dummy;
    if (len == NULL) {
        len = &dummy;
    }

    switch (value.type) {
        case VAL_FLOAT: {
            double decimal = as_float(value);
            if (decimal == (int) decimal) {
                *len = asprintf(&buffer, "%.1f", decimal);
            } else {
                *len = asprintf(&buffer, "%.10g", as_float(value));
            }
            break;
        }
        case VAL_INT: {
            *len = asprintf(&buffer, "%d", as_int(value));
            break;
        }
        case VAL_BOOL: {
            if (as_bool(value)) {
                buffer = malloc(5);
                memcpy(buffer, "true", 5);
                *len = 4;
            } else {
                buffer = malloc(6);
                memcpy(buffer, "false", 6);
                *len = 5;
            }
            break;
        }
        case VAL_NIL: {
            buffer = malloc(4);
            memcpy(buffer, "nil", 4);
            *len = 3;
            break;
        }
        case VAL_REF: {
            buffer = ref_to_chars(value, len);
            break;
        }
        case VAL_ABSENCE: {
            buffer = malloc(8);
            memcpy(buffer, "absence", 8);
            *len = 8;
            break;
        }
        default:
            printf("error: encountering a value with unknown type: %d\n", value.type);
            buffer = NULL;
            break;
    }
    return buffer;
}
