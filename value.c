//
// Created by Yue Xue  on 11/14/24.
//

#include "value.h"
#include "memory.h"
#include "stdio.h"

void init_ValueArray(ValueArray *array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void write_ValueArray(ValueArray *array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = array->capacity < 8 ? 8 : 2 * array->capacity;
        array->values = GROW_ARRAY(Value, array->values,
                                   oldCapacity, array->capacity);
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

inline bool is_number(Value value) {
    return is_int(value) || is_float(value);
}

inline bool is_nil(Value value) {
    return value.type == VAL_NIL;
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

inline Value bool_value(bool value) {
    return (Value) {
            .type = VAL_BOOL,
            .as = {.boolean = value}
    };
}

inline Value float_value(double value) {
    return (Value) {
            .type = VAL_FLOAT,
            .as = {.decimal = value}
    };
}

inline Value int_value(int value) {
    return (Value) {
            .type = VAL_INT,
            .as = {.integer = value}
    };
}

inline Value nil_value() {
    return (Value) {
            .type = VAL_NIL,
            .as = {}
    };
}

inline bool equal_value(Value a, Value b) {
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
        default:
            return true;
    }
}

/**
 * 打印一个 value
 * @param value 想要打印的 value
 */
void print_value(Value value) {
    if (is_float(value)) {
        printf("%g", as_float(value));
    } else if (is_int(value)) {
        printf("%d", as_int(value));
    } else if (is_bool(value)) {
        if (as_bool(value)) {
            printf("true");
        } else {
            printf("false");
        }
    } else if (is_nil(value)) {
        printf("nil");
    } else{
        printf("error: encountering a value with unknown type: %d\n", value.type);
    }
}

void print_value_with_type(Value value) {
    if (is_float(value)) {
        printf("%g: float", as_float(value));
    } else if (is_int(value)) {
        printf("%d: int", as_int(value));
    } else if (is_bool(value)) {
        if (as_bool(value)) {
            printf("true");
        } else {
            printf("false");
        }
    } else if (is_nil(value)) {
        printf("nil");
    } else{
        printf("error: encountering a value with unknown type: %d\n", value.type);
    }
}


