//
// Created by Yue Xue  on 11/14/24.
//

#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

#define AS_NUMBER(value) \
    (is_int(value) ? as_int(value) : as_float(value))


typedef struct Object Object;

typedef enum {
    VAL_FLOAT,
    VAL_BOOL,
    VAL_INT,
    VAL_NIL,
    VAL_REF,
} ValueType;

typedef struct Value{
    union {
        double decimal;
        int integer;
        bool boolean;
        Object *reference;
    } as;
    ValueType type;
} Value;

typedef struct ValueArray{
    int capacity;
    int count;
    Value *values;
} ValueArray;

void init_ValueArray(ValueArray *array);

void append_ValueArray(ValueArray *array, Value value);

void free_ValueArray(ValueArray *array);

void print_value(Value value);
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

extern char *RED;
extern char *BOLD_RED;
extern char *GREEN;
extern char *BOLD_GREEN;
extern char *CYAN;
extern char *BOLD_CYAN;
extern char *GRAY;
extern char *MAGENTA;
extern char *BLUE;
extern char *YELLOW;

void start_color(char *color);
void end_color();
void print_value_with_color(Value value);

#endif //CLOX_VALUE_H
