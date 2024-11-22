#ifndef CLOX_OBJECT_H

#define CLOX_OBJECT_H

#include "value.h"

typedef enum {
  OBJ_STRING,
} ObjectType;

typedef struct Object{
    ObjectType type;
    Object *next;
} Object; 

typedef struct String{
    Object object;
    int length;
    const char *chars;
    uint32_t hash;
} String;

bool is_ref_of(Value value, ObjectType type);
String *as_string(Value value);
String *string_copy(const char *src, int length);
String *string_allocate(char *chars, int length);
String *string_concat(Value a, Value b);

Object *allocate_object(size_t size, ObjectType type);

#endif