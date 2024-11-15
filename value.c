//
// Created by Yue Xue  on 11/14/24.
//

#include "value.h"
#include "memory.h"
#include "stdio.h"

void init_ValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void write_ValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = array->capacity < 8 ? 8 : 2 * array->capacity;
        array->values = GROW_ARRAY(Value, array->values,
                                   oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void free_ValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    init_ValueArray(array);
}

void print_value(Value value) {
    printf("%g", value);
}


