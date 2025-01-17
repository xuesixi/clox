#include "object.h"

#include <string.h>

#include "memory.h"
#include "vm.h"

/**
 * 使用指定的 src 产生一个 String。
 * 如果同值的string已存在，那么直接返回那个对象。
 * 否则，分配新的内存空间以复制那些字符，并创建一个string。
 * 原char*不会被引用，也不会被修改。
 * */
String *string_copy(const char *src, int length) {

    uint32_t hash = chars_hash(src, length);

    String *interned = table_find_string(&vm.string_table, src, length, hash);

    if (interned != NULL) {
        return interned;
    }

    String *str = (String *) allocate_object(sizeof(String), OBJ_STRING);
    stack_push(ref_value((Object *) str));
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, src, length);
    chars[length] = '\0';
    str->chars = chars;
    str->length = length;
    str->hash = hash;
    table_add_new(&vm.string_table, str, nil_value(), true, false);
    stack_pop();
    return str;
}

String *auto_length_string_copy(const char *name) {
    int len = strlen(name); // NOLINT
    return string_copy(name, len);
}

/**
 * 如果同值的String不存在，使用（占据）给定的 char* 来产生一个 String。
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
    table_add_new(&vm.string_table, str, nil_value(), true, false); // this may cause gc

    stack_pop();

    return str;
}

/**
 * 将a 和 b 的字符串表达拼接在一起，产生一个 String
 * */
String *string_concat(Value a, Value b) {
    int len_a, len_b;
    char *a_str = value_to_chars(a, &len_a);
    char *b_str = value_to_chars(b, &len_b);
    char *buffer = malloc(len_a + len_b + 1);
    memcpy(buffer, a_str, len_a);
    memcpy(buffer + len_a, b_str, len_b + 1);
    free(a_str);
    free(b_str);
    return string_allocate(buffer, len_a + len_b);
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
#ifdef DEBUG_LOG_GC_ALLOCATE
    printf("%p is allocated with size %zu for type %d, now allocated size: %zu, next gc: %zu\n", obj, size, type, vm.allocated_size, vm.next_gc);
#endif
    return obj;
}

uint32_t chars_hash(const char *key, int length) {
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
    function->fixed_arg_count = 0;
    function->optional_arg_count = 0;
    function->var_arg = false;
    function->type = type;
    function->upvalue_count = 0;
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
    closure->module_of_define = NULL;

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
    class->super_class = NULL;
    init_table(&class->methods);
    init_table(&class->static_fields);
    return class;
}

Instance *new_instance(Class *class) {
    Instance *instance = (Instance *) allocate_object(sizeof(Instance), OBJ_INSTANCE);
    instance->class = class;
    init_table(&instance->fields);
    return instance;
}

Method *new_method(Closure *closure, Value value) {
    Method *method = (Method *) allocate_object(sizeof(Method), OBJ_METHOD);
    method->closure = closure;
    method->receiver = value;
    return method;
}

NativeMethod *new_native_method(NativeFunction *fun, Value receiver) {
    NativeMethod * method = (NativeMethod *) allocate_object(sizeof(NativeMethod), OBJ_NATIVE_METHOD);
    method->fun = fun;
    method->receiver = receiver;
    return method;
}

Array *new_array(int length, bool init_with_nil) {
    Value *values = ALLOCATE(Value, length);
    Array *array = (Array *) allocate_object(sizeof(Array), OBJ_ARRAY);
    array->length = length;
    array->values = values;
    if (init_with_nil) {
        memset(array->values, 0, sizeof(Value) * length);
    }
    return array;
}

Module *new_module(String *path) {
    Module *module = (Module *) allocate_object(sizeof(Module), OBJ_MODULE);
    module->path = path;
    init_table(&module->globals);
    return module;
}

/**
 * 创建一个新的native object。
 * value_used 表示将要使用的数据的个数，该函数会根据之来将没有使用的值初始化为nil。
 */
NativeObject *new_native_object(NativeObjectType type, int value_used) {
    NativeObject *native_obj = (NativeObject *) allocate_object(sizeof(NativeObject), OBJ_NATIVE_OBJECT);
    native_obj->native_type = type;
    for (int i = value_used; i < NATIVE_OBJECT_VALUE_SIZE; ++i) {
        native_obj->values[i] = nil_value();
    }
    return native_obj;
}

Map *new_map() {
    Map *map = (Map *) allocate_object(sizeof(Map), OBJ_MAP);
    map->backing = NULL;
    map->capacity = 0;
    map->active_count = 0;
    map->del_count = 0;
    return map;
}


