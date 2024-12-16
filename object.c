#include "object.h"

#include <string.h>

#include "memory.h"
#include "vm.h"

static uint32_t chars_hash(const char *key, int length);

inline bool is_ref_of(Value value, ObjectType type) {
    return is_ref(value) && as_ref(value)->type == type;
}

inline String *as_string(Value value) {
    return (String *) as_ref(value);
}

inline LoxFunction *as_function(Value value) {
    return (LoxFunction *) as_ref(value);
}

inline NativeFunction *as_native(Value value) {
    return (NativeFunction *) as_ref(value);
}

inline Closure *as_closure(Value value) {
    return (Closure *) as_ref(value);
}

/**
 * 从指定的 src 处产生一个新的 String。原 char*不会被修改。
 * */
String *string_copy(const char *src, int length) {
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, src, length);
    chars[length] = '\0';
    return string_allocate(chars, length);
}

/**
 * 如果同值的String不存在，使用给定的 char* 来产生一个 String。
 * 如果同值的String已经存在，那么不产生新的，而是直接返回旧有的，并free给定的chars
 * */
String *string_allocate(char *chars, int length) {

    uint32_t hash = chars_hash(chars, length);

    String *interned = table_find_string(&vm.string_table, chars, length, hash);

    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    String *str = (String *) allocate_object(sizeof(String), OBJ_STRING);

    stack_push(ref_value((Object *) str)); // prevent being gc

    str->object.type = OBJ_STRING;
    str->chars = chars;
    str->length = length;
    str->hash = hash;
    table_set(&vm.string_table, str, nil_value()); // this may cause gc

    stack_pop();

    return str;
}

/**
 * 将a 和 b 的字符串表达拼接在一起，产生一个 String
 * */
String *string_concat(Value a, Value b) {
    char *a_str = to_print_chars(a);
    char *b_str = to_print_chars(b);
    char *buffer;
    int length = asprintf(&buffer, "%s%s", a_str, b_str);
    free(a_str);
    free(b_str);
    return string_allocate(buffer, length);
}

/**
 * 分配指定字节大小的 Object。所有引用类型的对象都应该由此产生。
 * 例如：`String *str = allocate_object(sizeof(String), OBJ_STRING);`
 * */
Object *allocate_object(size_t size, ObjectType type) {
    Object *obj = re_allocate(NULL, 0, size);
    obj->type = type;
    obj->next = vm.objects;
    obj->is_marked = false;
    vm.objects = obj;
#ifdef DEBUG_LOG_GC
    printf("%p is allocated with size %zu for type %d\n", obj, size, type);
#endif
    return obj;
}

static uint32_t chars_hash(const char *key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t) key[i];
        hash *= 16777619;
    }
    return hash;
}

LoxFunction *new_function(FunctionType type) {
    LoxFunction *function = (LoxFunction *) allocate_object(sizeof(LoxFunction), OBJ_FUNCTION);
    function->name = NULL;
    function->arity = 0;
    function->type = type;
    function->upvalue_count = 0;
    init_chunk(&function->chunk);
    return function;
}

Closure *new_closure(LoxFunction *function) {
    Closure *closure = (Closure *) allocate_object(sizeof(Closure), OBJ_CLOSURE);

    stack_push(ref_value((Object *) closure));

    closure->function = function;

    UpValue **upvalues = (UpValue **) ALLOCATE(UpValue*, function->upvalue_count);
    for (int i = 0; i < function->upvalue_count; ++i) {
        upvalues[i] = NULL;
    }

    closure->upvalue_count = function->upvalue_count;
    closure->upvalues = upvalues;

    stack_pop();

    return closure;
}

UpValue *new_upvalue(Value *position) {
    UpValue *upValueObject = (UpValue *) allocate_object(sizeof(UpValue), OBJ_UPVALUE);
    upValueObject->position = position;
    upValueObject->next = NULL;
    upValueObject->closed = nil_value();
    return upValueObject;
}

NativeFunction *new_native(NativeImplementation impl, String *name, int arity) {
    NativeFunction *native = (NativeFunction *) allocate_object(sizeof(NativeFunction), OBJ_NATIVE);
    native->impl = impl;
    native->name = name;
    native->arity = arity;
    return native;
}

Class *new_class(String *name) {
    Class *class = (Class *) allocate_object(sizeof(Class), OBJ_CLASS);
    class->name = name;
    init_table(&class->methods);
    return class;
}

inline Class *as_class(Value value) {
    return (Class *) as_ref(value);
}

Instance *new_instance(Class *class) {
    Instance *instance = (Instance *) allocate_object(sizeof(Instance), OBJ_INSTANCE);
    instance->class = class;
    init_table(&instance->fields);
    return instance;
}

Instance *as_instance(Value value) {
    return (Instance *) as_ref(value);
}

Method *new_method(Closure *closure, Value value) {
    Method *method = (Method *) allocate_object(sizeof(Method), OBJ_METHOD);
    method->closure = closure;
    method->receiver = value;
    return method;
}

inline Method *as_method(Value value) {
    return (Method *) as_ref(value);
}



