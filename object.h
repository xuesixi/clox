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
  OBJ_ARRAY,
  OBJ_MODULE,
} ObjectType;

typedef struct Object{
    ObjectType type;
    Object *next;
    bool is_marked;
} Object;

typedef struct Module {
    Object object;
    Table globals;
} Module;

typedef struct Array {
    Object object;
    int length;
    Value *values;
} Array;

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
    Module *module;
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
    Table static_fields;
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

#define is_ref_of(value, ref_type) (is_ref(value) && as_ref(value)->type == (ref_type))

#define as_string(v) ((String *)(as_ref(v)))
#define as_function(v) ((LoxFunction *)(as_ref(v)))
#define as_native(v) ((NativeFunction *)(as_ref(v)))
#define as_closure(v) ((Closure *)(as_ref(v)))
#define as_class(v) ((Class *)(as_ref(v)))
#define as_instance(v) ((Instance *)(as_ref(v)))
#define as_method(v) ((Method *)(as_ref(v)))
#define as_array(v) ((Array *)(as_ref(v)))
#define as_module(v) ((Module *)(as_ref(v)))


String *string_copy(const char *src, int length);
String *string_allocate(char *chars, int length);
String *string_concat(Value a, Value b);

Object *allocate_object(size_t size, ObjectType type);

LoxFunction *new_function(FunctionType type);
NativeFunction *new_native(NativeImplementation impl, String *name, int arity);
Closure *new_closure(LoxFunction *function);
UpValue *new_upvalue(Value *position);
Class *new_class(String *name);
Instance *new_instance(Class *class);
Method *new_method(Closure *closure, Value value);
Array *new_array(int length);
Module *new_module();

#endif