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
char *GRAY = "\033[0;90m";
char *MAGENTA = "\033[35m";
char *BLUE = "\033[38;5;32m";
char *YELLOW = "\033[1;33m";

inline void start_color(char *color) {
    printf("%s", color);
}

inline void end_color() {
    printf("\033[0m");
}

void print_value_with_color(Value value) {
    char *str = to_print_chars(value);
    switch (value.type) {
        case VAL_INT:
        case VAL_FLOAT:
        case VAL_NIL:
        case VAL_BOOL:
            start_color(YELLOW);
            break;
        case VAL_REF: {
            switch (as_ref(value)->type) {
                case OBJ_STRING:
                    start_color(MAGENTA);
                    break;
                case OBJ_CLOSURE:
                case OBJ_NATIVE:
                case OBJ_FUNCTION:
                    start_color(BLUE);
                    break;
                case OBJ_UPVALUE:
                    break;
            }
            break;
        }
    }
    printf("%s", str);
    end_color();
    free(str);
}

static char *to_print_ref(Value value);

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

inline bool is_bool(Value value) {
    return value.type == VAL_BOOL;
}

inline bool is_float(Value value) {
    return value.type == VAL_FLOAT;
}

inline bool is_int(Value value) {
    return value.type == VAL_INT;
}

/**
 * int 或者 float
 * */
inline bool is_number(Value value) {
    return is_int(value) || is_float(value);
}

inline bool is_nil(Value value) {
    return value.type == VAL_NIL;
}

inline bool is_ref(Value value) {
    return value.type == VAL_REF;
}

inline double as_float(Value value) {
    return value.as.decimal;
}

inline int as_int(Value value) {
    return value.as.integer;
}

inline bool as_bool(Value value) {
    return value.as.boolean;
}

inline Object *as_ref(Value value) {
    return value.as.reference;
}

inline Value bool_value(bool value) {
    return (Value) {.type = VAL_BOOL, .as = {.boolean = value}};
}

inline Value float_value(double value) {
    return (Value) {.type = VAL_FLOAT, .as = {.decimal = value}};
}

inline Value int_value(int value) {
    return (Value) {.type = VAL_INT, .as = {.integer = value}};
}

inline Value nil_value() {
    return (Value) {.type = VAL_NIL, .as = {}};
}

inline Value ref_value(Object *value) {
    return (Value) {.type = VAL_REF, .as = {.reference = value}};
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

static char *to_print_ref(Value value) {
    char *buffer;
    ObjectType type = as_ref(value)->type;
    switch (type) {
        case OBJ_STRING:
            asprintf(&buffer, "%s", as_string(value)->chars);
            break;
        case OBJ_CLOSURE: {
            LoxFunction *fun = as_closure(value)->function;
            if (fun->type == TYPE_MAIN) {
                asprintf(&buffer, "<main>");
            } else if (fun->type == TYPE_LAMBDA){
                asprintf(&buffer, "<lambda>");
            } else {
                asprintf(&buffer, "<fn: %s>", as_closure(value)->function->name->chars);
            }
            break;
        }
        case OBJ_NATIVE:
            asprintf(&buffer, "<native: %s>", as_native(value)->name->chars);
            break;
        case OBJ_FUNCTION: {
            LoxFunction *fun = as_function(value);
            if (fun->type == TYPE_MAIN) {
                asprintf(&buffer, "<proto: main>");
            } else if (fun->type == TYPE_LAMBDA){
                asprintf(&buffer, "<proto: lambda>");
            } else {
                asprintf(&buffer, "<proto: %s>", fun->name->chars);
            }
            break;
        }
        case OBJ_UPVALUE:
            asprintf(&buffer, "<upvalue>");
            break;
        case OBJ_CLASS: {
            Class *class = as_class(value) ;
            asprintf(&buffer, "<class: %s>", class->name->chars);
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
    char *str = to_print_chars(value);
    printf("%s", str);
    free(str);
}

/**
 * 获取目标 value 的char*表达。调用者需要自己 free 之
 * */
char *to_print_chars(Value value) {
    char *buffer;
    if (is_float(value)) {
        double decimal = as_float(value);
        if (decimal == (int ) decimal) {
            asprintf(&buffer, "%.1f", decimal);
        } else {
            asprintf(&buffer, "%.10g", as_float(value));
        }
    } else if (is_int(value)) {
        asprintf(&buffer, "%d", as_int(value));
    } else if (is_bool(value)) {
        if (as_bool(value)) {
            asprintf(&buffer, "true");
        } else {
            asprintf(&buffer, "false");
        }
    } else if (is_nil(value)) {
        asprintf(&buffer, "nil");
    } else if (is_ref(value)) {
        buffer = to_print_ref(value);
    } else {
        printf("error: encountering a value with unknown type: %d\n", value.type);
        buffer = NULL;
    }
    return buffer;
}
