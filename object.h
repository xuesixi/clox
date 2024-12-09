#ifndef CLOX_OBJECT_H

#define CLOX_OBJECT_H

#include "value.h"
#include "chunk.h"

typedef enum {
  OBJ_STRING,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_CLOSURE,
  OBJ_UPVALUE,
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
    int upvalue_count;
} LoxFunction;

typedef struct UpValueObject {
    Object object;
    Value *position;
    Value closed;
    struct UpValueObject *next;
} UpValueObject;

typedef struct Closure {
    Object object;
    LoxFunction *function;
    UpValueObject **upvalues; // 意思是array of pointers to UpValueObject，之所以有额外一层pointer，是因为我们多个closure可以共享同一个UpVlaueObject本体。
    int upvalue_count;
} Closure;

typedef Value (*NativeImplementation)(int count, Value *values);

typedef struct NativeFunction {
    Object object;
    NativeImplementation impl;
    String *name;
    int arity; // -1 means variable amount of arguments
} NativeFunction;

bool is_ref_of(Value value, ObjectType type);
String *as_string(Value value);
String *string_copy(const char *src, int length);
String *string_allocate(char *chars, int length);
String *string_concat(Value a, Value b);

Object *allocate_object(size_t size, ObjectType type);

LoxFunction *new_function();
LoxFunction *as_function(Value value);

NativeFunction *new_native(NativeImplementation impl, String *name, int arity);
NativeFunction *as_native(Value value);

Closure *as_closure(Value value);
Closure *new_closure(LoxFunction *function);

UpValueObject *new_upvalue(Value *position);

#endif