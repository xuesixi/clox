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
  OBJ_CLASS,
  OBJ_INSTANCE,
} ObjectType;

typedef struct Object{
    ObjectType type;
    Object *next;
    bool is_marked;
} Object;

typedef enum FunctionType {
    TYPE_FUNCTION,
    TYPE_MAIN,
    TYPE_LAMBDA,
} FunctionType;

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
    FunctionType type;
} LoxFunction;

typedef struct UpValue {
    Object object;
    Value *position;
    Value closed;
    struct UpValue *next;
} UpValue;

typedef struct Closure {
    Object object;
    LoxFunction *function;
    UpValue **upvalues; // array of pointers to UpValueObject，之所以有额外一层pointer，是因为多个closure可以共享同一个UpValueObject本体。
    int upvalue_count;
} Closure;

typedef struct Class {
    Object object;
    String *name;
} Class;

typedef struct Instance {
    Object object;
    Class *class;
} Instance;

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

LoxFunction *new_function(FunctionType type);
LoxFunction *as_function(Value value);

NativeFunction *new_native(NativeImplementation impl, String *name, int arity);
NativeFunction *as_native(Value value);

Closure *as_closure(Value value);
Closure *new_closure(LoxFunction *function);

UpValue *new_upvalue(Value *position);

Class *new_class(String *name);
Class *as_class(Value value);

#endif