//
// Created by Yue Xue  on 11/14/24.
//

#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

typedef struct Object Object;

typedef enum {
    VAL_FLOAT,
    VAL_BOOL,
    VAL_INT,
    VAL_NIL,
    VAL_REF,
} ValueType;

typedef struct {
    ValueType type;
    union {
        double decimal;
        int integer;
        bool boolean;
        Object *reference;
    } as;
} Value;

typedef struct {
    int capacity;
    int count;
    Value *values;
} ValueArray;

void init_ValueArray(ValueArray *array);

void write_ValueArray(ValueArray *array, Value value);

void free_ValueArray(ValueArray *array);

void print_value(Value value);
// void print_value_with_type(Value value);
// void print_ref(Value value);
char *to_print_chars(Value value);

bool is_bool(Value value);
bool is_float(Value value);
bool is_number(Value value);
bool is_int(Value value);
bool is_nil(Value value);
bool is_ref(Value value);

double as_float(Value value);
int as_int(Value value);
bool as_bool(Value value);
Object *as_ref(Value value);

Value bool_value(bool value);
Value float_value(double value);
Value int_value(int value);
Value nil_value();
Value ref_value(Object *value);

bool value_equal(Value a, Value b);
bool object_equal(Object *a, Object *b);

#endif //CLOX_VALUE_H
