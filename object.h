#ifndef CLOX_OBJECT_H

#define CLOX_OBJECT_H

#include "value.h"
#include "chunk.h"
#include "table.h"

typedef enum {
  OBJ_STRING,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_CLOSURE,
  OBJ_UPVALUE,
  OBJ_CLASS,
  OBJ_INSTANCE,
  OBJ_METHOD,
} ObjectType;

typedef struct Object{
    ObjectType type;
    Object *next;
    bool is_marked;
} Object;

typedef enum FunctionType {
    TYPE_FUNCTION,
    TYPE_METHOD,
    TYPE_MAIN,
    TYPE_LAMBDA,
    TYPE_INITIALIZER,
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

typedef struct Method {
    Object object;
    Closure *closure;
    Value receiver;
} Method;

typedef struct Class {
    Object object;
    String *name;
    Table methods;
} Class;

typedef struct Instance {
    Object object;
    Class *class;
    Table fields;
} Instance;

typedef Value (*NativeImplementation)(int count, Value *values);

typedef struct NativeFunction {
    Object object;
    NativeImplementation impl;
    String *name;
    int arity; // -1 means variable amount of arguments
} NativeFunction;

//bool is_ref_of(Value value, ObjectType type);

//inline bool is_ref_of(Value value, ObjectType type) {
//    return is_ref(value) && as_ref(value)->type == type;
//}

#define is_ref_of(value, ref_type) (is_ref(value) && as_ref(value)->type == (ref_type))

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

Instance *new_instance(Class *class);
Instance *as_instance(Value value);

Method *new_method(Closure *closure, Value value);
Method *as_method(Value value);
#endif