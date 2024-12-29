#ifndef CLOX_OBJECT_H

#define CLOX_OBJECT_H
#define NATIVE_OBJECT_VALUE_SIZE 4

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
  OBJ_NATIVE_OBJECT,
  OBJ_MAP,
} ObjectType;

typedef enum NativeObjectType {
    NativeRangeIter,
    NativeArrayIter,
    NativeMapIter
} NativeObjectType;

typedef enum NativeInterface {
    INTER_NULL,
    INTER_HAS_NEXT,
    INTER_NEXT,
    INTER_ITERATOR,
    INTER_HASH,
    INTER_EQUAL,
} NativeInterface;

typedef struct Object{
    ObjectType type;
    Object *next;
    bool is_marked;
} Object;

typedef struct Module {
    Object object;
    Table globals;
    String *path;
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
    char *chars;
    uint32_t hash;
} String;

typedef struct LoxFunction {
    Object object;
    Chunk chunk;
    String *name;
    int upvalue_count;
    FunctionType type;
    int fixed_arg_count; // 固定参数的数量。
    short optional_arg_count; // 可选参数的数量
    bool var_arg; // 是否接受可变数量的参数。arity=3, var_arg=true意味着至少三个参数，实际调用时会将所有额外参数作为一个数组传入
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

typedef struct MapEntry {
    Value key;
    Value value;
    int hash;
} MapEntry;

typedef struct Map {
    Object object;
    int count;
    int capacity;
    MapEntry *backing;
} Map;

typedef Value (*NativeImplementation)(int count, Value *values);

typedef struct NativeFunction {
    Object object;
    NativeImplementation impl;
    String *name;
    int arity; // -1 means variable amount of arguments
} NativeFunction;

typedef struct NativeObject {
    Object object;
    Value values[NATIVE_OBJECT_VALUE_SIZE];
    NativeObjectType native_type;
} NativeObject;

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
#define as_native_object(v) ((NativeObject *)as_ref(v) )
#define as_map(v) ((Map *)(as_ref(v)) )

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
Module *new_module(String *path);
Map *new_map();

NativeObject *new_native_object(NativeObjectType type, int num_used);
uint32_t chars_hash(const char *key, int length);

bool map_empty_entry(MapEntry *entry);
bool map_del_mark(MapEntry *entry);
bool map_need_resize(Map *map);


#endif