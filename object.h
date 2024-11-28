#ifndef CLOX_OBJECT_H

#define CLOX_OBJECT_H

#include "value.h"
#include "chunk.h"

typedef enum {
  OBJ_STRING,
  OBJ_FUNCTION,
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

typedef struct LoxFunction {
    Object object;
    int arity;
    Chunk chunk;
    String *name;
} LoxFunction;


bool is_ref_of(Value value, ObjectType type);
String *as_string(Value value);
String *string_copy(const char *src, int length);
String *string_allocate(char *chars, int length);
String *string_concat(Value a, Value b);

Object *allocate_object(size_t size, ObjectType type);
LoxFunction *new_function();
LoxFunction *as_function(Value value);

#endif