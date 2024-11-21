#ifndef CLOX_OBJECT_H

#define CLOX_OBJECT_H

#include "value.h"

typedef enum {
  OBJ_STRING,
} ObjectType;

struct Object{
    ObjectType type;
}; 

typedef struct {
    Object object;
    int length;
    char *chars;
} String;

bool is_ref_of(Value value, ObjectType type);
String *as_string(Value value);
String *string_copy(const char *src, int length);
String *string_allocate(char *chars, int length);
String *string_concat(Value a, Value b);

#endif