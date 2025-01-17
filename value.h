//
// Created by Yue Xue  on 11/14/24.
//

#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

typedef struct Object Object;

typedef enum {
    VAL_NIL,
    VAL_FLOAT,
    VAL_BOOL,
    VAL_INT,
    VAL_ABSENCE,
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
char *value_to_chars(Value value, int *len);

#define is_bool(value) ((value).type == VAL_BOOL)
#define is_float(value) ((value).type == VAL_FLOAT)
#define is_number(value) ((value).type == VAL_FLOAT || (value).type == VAL_INT )
#define is_int(value) ((value).type == VAL_INT)
#define is_nil(value) ((value).type == VAL_NIL)
#define is_ref(value) ((value).type == VAL_REF)
#define is_absence(value) ((value).type == VAL_ABSENCE)

#define as_float(v) ((v).as.decimal)
#define as_int(v) ((v).as.integer)
#define as_bool(v) ((v).as.boolean)
#define as_ref(v) ((v).as.reference)
#define AS_NUMBER(value) \
    (is_int(value) ? as_int(value) : as_float(value))

#define bool_value(v) ((Value) {.type = VAL_BOOL, .as = {.boolean = (v)}})
#define int_value(v) ((Value) {.type = VAL_INT, .as = {.integer = (v)}})
#define float_value(v) ((Value) {.type = VAL_FLOAT, .as = {.decimal = (v)}})
#define nil_value() ((Value) {.type = VAL_NIL, .as = {}})
#define ref_value(v) ((Value) {.type = VAL_REF, .as = {.reference = (v)}})
#define ref_value_cast(v) ref_value((Object *)(v))
#define absence_value() ((Value) {.type = VAL_ABSENCE, .as = {}})

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
extern char *BOLD_BLUE;
extern char *BOLD_MAGENTA;

void start_color(char *color);
void end_color();
void print_value_with_color(Value value);

#endif //CLOX_VALUE_H
