//
// Created by Yue Xue  on 11/16/24.
//

#include "vm.h"
#include "libgen.h"
#include "native.h"

#include "compiler.h"
#include "io.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "time.h"
#include "math.h"

#include "stdarg.h"
#include "setjmp.h"

VM vm;
Module *repl_module = NULL;

jmp_buf error_buf;

static CallFrame *curr_frame;
static Table *curr_closure_global;
static Value *curr_const_pool;

static inline uint8_t read_byte();

static inline uint16_t read_uint16();

static inline bool is_falsy(Value value);

static void throw_value(Value value);

static inline void reset_stack();

static InterpretResult run_frame_until(int end_when);

static void invoke_property(String *name, int arg_count);

static inline bool if_read_byte(OpCode op);

static inline Value read_constant16();

static void import(const char *src, String *path);

static void show_stack();

static void build_array(int length);

static void call_native(NativeFunction *native , int arg_count);

static Value stack_peek(int distance);

static void binary_number_op(Value a, Value b, char operator);

static void call_value(Value value, int arg_count);

static void invoke_from_class(Class *class, String *name, int arg_count);

static void call_closure(Closure *closure, int arg_count);

static Value multi_dimension_array(int dimension, Value *lens);

static void warmup(LoxFunction *function, const char *path_chars, String *path_string, bool care_repl);

static Value bind_method(Class *class, String *name, Value receiver);

static void close_upvalue(Value *position);

static void invoke_native_object(int arg_count, String *name, NativeObject *native_object);


/**
 * 根据vm.frame_count来更新curr_frame, 然后根据curr_frame来更新curr_const_pool以及curr_closure_global
 */
static inline void sync_frame_cache() {
    curr_frame = vm.frames + vm.frame_count - 1;
    curr_const_pool = curr_frame->closure->function->chunk.constants.values;
    curr_closure_global = &curr_frame->closure->module_of_define->globals;
}

static inline void stack_set(int n, Value value) {
    vm.stack_top[-1 - n] = value;
}

static inline int positive_mod(int a, int b) {
    int result = a % b;
    return result < 0 ? result + b : result;
}

Class *value_class(Value value) {
    switch (value.type) {
        case VAL_INT:
            return int_class;
        case VAL_FLOAT:
            return float_class;
        case VAL_BOOL:
            return bool_class;
        case VAL_NIL:
            return nil_class;
        case VAL_REF: {
            switch (as_ref(value)->type) {
                case OBJ_STRING:
                    return string_class;
                case OBJ_FUNCTION:
                    return function_class;
                case OBJ_CLOSURE:
                    return closure_class;
                case OBJ_CLASS:
                    return class_class;
                case OBJ_MAP:
                    return map_class;
                case OBJ_ARRAY:
                    return array_class;
                case OBJ_NATIVE:
                    return native_class;
                case OBJ_MODULE:
                    return module_class;
                case OBJ_METHOD:
                    return method_class;
                case OBJ_INSTANCE:
                    return as_instance(value)->class;
                case OBJ_NATIVE_OBJECT:
                    return native_object_class;
                case OBJ_NATIVE_METHOD:
                    return native_method_class;
                case OBJ_UPVALUE:
                    return NULL;
                    IMPLEMENTATION_ERROR("bad");
            }
            break;
        }
        default:
            IMPLEMENTATION_ERROR("bad");
            return NULL;
    }
    return NULL;
}

/**
 * 将value置于stack_top处，然后自增stack_top
 * @param value 想要添加的value
 */
inline void stack_push(Value value) {
    *vm.stack_top = value;
    vm.stack_top++;
}

inline Value stack_pop() {
    vm.stack_top--;
    return *(vm.stack_top);
}

/**
 * 将栈顶值和stack_peek(n)交换
 */
static inline void stack_swap(int n) {
    Value temp = stack_peek(n);
    stack_set(n, stack_peek(0));
    stack_set(0, temp);
}

static inline void invoke_and_wait(String *name, int arg_count) {
    int count = vm.frame_count;
    invoke_property(name, arg_count);
    run_frame_until(count);
}

static void string_indexing_get() {
    // [str, index]
    Value index_value = stack_pop();
    assert_value_type(index_value, VAL_INT, "int");
    Value str_value = stack_pop();
    String *str = as_string(str_value);
    int index = as_int(index_value);
    if (index < 0 || index >= str->length) {
        throw_user_level_runtime_error(Error_IndexError, "IndexError: index %d is out of bound: [0, %d]", index,
                                       str->length - 1);
    } else {
        String *char_at_index = string_copy(str->chars + index, 1);
        stack_push(ref_value_cast(char_at_index));
    }
}

static void array_indexing_get() {
    // [arr, index]
    Value index_value = stack_pop();
    assert_value_type(index_value, VAL_INT, "int");
    Value arr_value = stack_pop();
    Array *array = as_array(arr_value);
    int index = as_int(index_value);
    if (index < 0 || index >= array->length) {
        throw_user_level_runtime_error(Error_IndexError, "IndexError: index %d is out of bound: [0, %d]", index,
                                       array->length - 1);
    } else {
        stack_push(array->values[index]);
    }
}

static void array_indexing_set() {
    // op: [array, index, value]
    Value value = stack_pop();
    Value index_value = stack_pop();
    assert_value_type(index_value, VAL_INT, "int");
    Value arr_value = stack_pop();
    Array *array = as_array(arr_value);
    int index = as_int(index_value);
    if (index < 0 || index >= array->length) {
        throw_user_level_runtime_error(Error_IndexError, "IndexError: index %d is out of bound: [0, %d]", index,
                                       array->length - 1);
    } else {
        array->values[index] = value;
        stack_push(value);
    }
}

/**
 * [map, key] -> [value]。
 * 如果没有找到，将使用throw_value()抛出IndexError（用户级别）。
 */
static void map_indexing_get() {
    // [map, key0]
    Map *map = as_map(stack_peek(1));
    if (map->active_count == 0) {
        stack_pop();
        stack_pop();
        throw_user_level_runtime_error(Error_IndexError, "IndexError: the key does not exist");
        return;
    }
    stack_push(stack_peek(0)); // [map, key0, key0]
    invoke_and_wait(HASH, 0);
    Value hash_result = stack_pop();
    assert_value_type(hash_result, VAL_INT, "int");
    int hash = as_int(hash_result); // [map, key0, hash]
    for (int i = 0; i < map->capacity; ++i) {
        int curr = MODULO(hash + i, map->capacity);
        MapEntry *entry = map->backing + curr;
        if (map_empty_entry(entry)) {
            stack_pop();
            stack_pop();
            throw_user_level_runtime_error(Error_IndexError, "IndexError: the key does not exist");
            return;
        } else if (!is_absence(entry->key) && entry->hash == hash) {
            stack_push(entry->key); // map, key0, key1
            stack_push(stack_peek(1)); // map, key0, key1, key0
            invoke_and_wait(EQUAL, 1); // map, key0, bool
            Value cmp_result = stack_pop(); // map, key0
            assert_value_type(cmp_result, VAL_BOOL, "bool");
            bool eq = as_bool(cmp_result);
            if (eq) {
                stack_pop();
                stack_pop();
                stack_push(entry->value);
                return;
            }
        }
    }
    // not found
    stack_pop();
    stack_pop();
    throw_user_level_runtime_error(Error_IndexError, "IndexError: the key does not exist");
}

/**
 * [map, key0, value0, hash] -> [map | value]
 * @param keep_map true: keep the map on the top of the stack, false: keep value on top
 */
static void map_indexing_set_with_hash(bool keep_map) {

    Map *map = as_map(stack_peek(3));
    Value hash_result = stack_pop(); // map, key0, value
    assert_value_type(hash_result, VAL_INT, "int");
    MapEntry *del_mark = NULL;

    int hash = as_int(hash_result); // map, key0, value
    for (int i = 0; i < map->capacity; ++i) {
        int curr = MODULO(hash + i, map->capacity);
        MapEntry *entry = map->backing + curr;
        if (map_empty_entry(entry)) {
            if (del_mark == NULL) {
                entry->value = stack_pop(); // -> map, key0
                entry->key = stack_pop(); //  -> [map]
                entry->hash = hash;
                map->active_count++;
                if (!keep_map) {
                    stack_pop(); // -> [ ]
                    stack_push(entry->value); // -> [value]
                }
                return;
            } else {
                del_mark->value = stack_pop();
                del_mark->key = stack_pop(); // -> [map]
                del_mark->hash = hash;
                if (!keep_map) {
                    stack_pop();
                    stack_push(del_mark->value); // -> [value]
                }
                map->del_count --;
                return;
            }
        } else if (del_mark == NULL && map_del_mark(entry)) {
            del_mark = entry;
        } else if (entry->hash == hash) {
            // [map, key0, value]
            stack_push(entry->key); // map, key0, value, key1
            stack_push(stack_peek(2)); // map, k0, v, k1, k0
            invoke_and_wait(EQUAL, 1);
            // map, k0, v, bool
            Value eq_result = stack_pop(); // map, k0, v
            assert_value_type(eq_result, VAL_BOOL, "bool");
            bool eq = as_bool(eq_result);
            if (eq) {
                entry->value = stack_pop(); // map, k0
                stack_pop();  // map
                if (!keep_map) {
                    stack_pop();
                    stack_push(entry->value); // -> [value]
                }
                return;
            }
        }
    }
    runtime_error("map cannot find empty spot, this is an implementation error!");
}

/**
 * [map, key0, value] -> [map | value]
 */
static void map_indexing_set(bool keep_map) {
    // map, key0, value
    Map *map = as_map(stack_peek(2));

    if (map_need_resize(map)) {
        int new_capacity = map->capacity < 8 ? 8 : map->capacity * 2;
        MapEntry *new_backing = ALLOCATE(MapEntry, new_capacity);
        for (int i = 0; i < new_capacity; ++i) {
            new_backing[i].key = absence_value();
            new_backing[i].value = absence_value();
        }
        Map *m2 = new_map();
        m2->backing = new_backing;
        m2->capacity = new_capacity;
        stack_push(ref_value((Object *) m2)); // map, k0, v, m2
        for (int i = 0; i < map->capacity; ++i) {
            MapEntry *entry = map->backing + i;
            if (!is_absence(entry->key)) {
                stack_push(entry->key);
                stack_push(entry->value);
                stack_push(int_value(entry->hash));

                // map, k0, v, m2, k1, v1, hash1
                map_indexing_set_with_hash(true);
                // map, k0, v, m2
            }
        }
        // map, k0, v, m2
        FREE_ARRAY(MapEntry, map->backing, map->capacity);
        map->backing = new_backing;
        map->capacity = new_capacity;
        map->active_count = m2->active_count;
        map->del_count = 0;
        m2->backing = NULL;
        m2->capacity = 0;
        m2->active_count = 0;
        stack_pop(); // map, k0, v
    }

    stack_push(stack_peek(1)); // map, key0, value, key0
    invoke_and_wait(HASH, 0);
    // map, key0, value, hash

    map_indexing_set_with_hash(keep_map);
    // map | value
}

void map_delete() {
    // [map, key] -> [map, key, value]
    Map *map = as_map(stack_peek(1));
    stack_push(stack_peek(0)); // [map, key, key]
    invoke_and_wait(HASH, 0); // [map, key, hash]
    Value hash_result = stack_pop(); // map, key0, value
    assert_value_type(hash_result, VAL_INT, "int");
    int hash = as_int(hash_result); // map, key0, value
    int index = MODULO(hash, map->capacity);
    for (int i = 0; i < map->capacity; ++i) {
        int curr = MODULO(index + i, map->capacity);
        MapEntry *entry = map->backing + curr;
        if (map_empty_entry(entry)) {
            throw_new_runtime_error(Error_IndexError, "IndexError: the key does not exist");
            return;
        } else if (!map_del_mark(entry) && entry->hash == hash) {
            stack_push(entry->key); // map, key0, key1
            stack_push(stack_peek(1)); // map, k0, k1, k0
            invoke_and_wait(EQUAL, 1);
            // map, k0, bool
            Value eq_result = stack_pop(); // map, k0
            assert_value_type(eq_result, VAL_BOOL, "bool");
            bool eq = as_bool(eq_result);
            if (eq) {
                stack_push(entry->value); // [map, key, value]
                map->active_count --;
                map->del_count ++;
                entry->key = absence_value();
                return;
            }
        }
    }
    throw_new_runtime_error(Error_IndexError, "IndexError: the key does not exist");
}

/**
 * 获取target的指定属性，并将之置于栈顶。
 */
static void get_property(Value target, String *property_name) {
    if (is_ref(target)) {
        switch (as_ref(target)->type) {
            case OBJ_INSTANCE: {
                Instance *instance = as_instance(target);
                Value result;
                bool found = table_get(&instance->fields, property_name, &result);
                if (found == false) {
                    result = bind_method(instance->class, property_name, target);
                }
                stack_push(result);
                break;
            }
            case OBJ_ARRAY: {
                if (property_name == LENGTH) {
                    Array *array = as_array(target);
                    stack_push(int_value(array->length));
                } else {
                    Value res = bind_method(array_class, property_name, target);
                    stack_push(res);
                }
                break;
            }
            case OBJ_STRING: {
                if (property_name == LENGTH) {
                    String *string = as_string(target);
                    stack_push(int_value(string->length));
                } else {
                    Value res = bind_method(string_class, property_name, target);
                    stack_push(res);
                }
                break;
            }
            case OBJ_MAP: {
                if (property_name == LENGTH) {
                    Map *map = as_map(target);
                    stack_push(int_value(map->active_count));
                } else {
                    stack_push(bind_method(map_class, property_name, target));
                }
                break;
            }
            case OBJ_CLASS: {
                Class *class = as_class(target);
                Value static_field;
                if (table_get(&class->static_fields, property_name, &static_field)) {
                    stack_push(static_field);
                } else {
                    stack_push(bind_method(class_class, property_name, target));
                }
                break;
            }
            case OBJ_MODULE: {
                Module *module = as_module(target);
                Value property;
                if (!table_conditional_get(&module->globals, property_name, &property, true, false)) {
                    throw_new_runtime_error(Error_PropertyError, "PropertyError: no such public property: %s", property_name->chars);
                } else {
                    stack_push(property);
                }
                break;
            }
            case OBJ_NATIVE:
                stack_push(bind_method(native_class, property_name, target));
                break;
            case OBJ_CLOSURE:
                stack_push(bind_method(closure_class, property_name, target));
                break;
            case OBJ_METHOD:
                stack_push(bind_method(method_class, property_name, target));
                break;
            case OBJ_FUNCTION:
                stack_push(bind_method(function_class, property_name, target));
                break;
            case OBJ_NATIVE_OBJECT:
                stack_push(bind_method(native_class, property_name, target));
                break;
            case OBJ_NATIVE_METHOD:
                stack_push(bind_method(NULL, property_name, target));
                break;
            default:
                throw_new_runtime_error(Error_PropertyError, "PropertyError: no such public property: %s", property_name->chars);
        }
    } else {
        switch (target.type) {
            case VAL_INT:
            case VAL_BOOL:
            case VAL_FLOAT:
            case VAL_NIL: {
                stack_push(bind_method(value_class(target), property_name, target));
                break;
            }
            default:
                IMPLEMENTATION_ERROR("bad");
                return;
        }
    }
}

inline void assert_ref_type(Value value, ObjectType type, const char *expected_type) {
    if (!is_ref_of(value, type)) {
        throw_new_runtime_error(Error_TypeError, "TypeError: expect value of type: %s", expected_type );
    }
}

inline void assert_value_type(Value value, ValueType type, const char *expected_type) {
    if (value.type != type) {
        throw_new_runtime_error(Error_TypeError, "TypeError: expect value of type: %s", expected_type );
    }
}

/**
 * 根据interface，执行native_object的对应行为。如果不匹配，该函数会导致runtime error
 * @param arg_count 参数个数。stack_peek(ar_count)就是native_object
 * @param interface 接口（预期行为）
 * @param native_object
 */
static void invoke_native_object(int arg_count, String *name, NativeObject *native_object) {
    (void ) arg_count;
    if (name == ITERATOR) {
        switch (native_object->native_type) {
            case NativeRangeIter: {
                // do nothing, it is by itself an iterator
                return;
            }
            default:
                goto error;
        }
    } else if (name == HAS_NEXT) {
        switch (native_object->native_type) {
            case NativeRangeIter: {
                // curr, limit, step
                stack_pop();
                bool has_next = as_int(native_object->values[0]) < as_int(native_object->values[1]);
                stack_push(bool_value(has_next));
                return;
            }
            case NativeArrayIter: {
                // curr, array
                stack_pop();
                bool has_next = as_int(native_object->values[0]) < as_int(native_object->values[2]);
                stack_push(bool_value(has_next));
                return;
            }
            case NativeMapIter: {
                // curr, map
                stack_pop();
                Map *map = as_map(native_object->values[1]);
                int curr = as_int(native_object->values[0]);
                bool has_next = false;
                for (int i = curr; i < map->capacity; ++i) {
                    if (!is_absence(map->backing[i].key)) {
                        has_next = true;
                        native_object->values[0] = int_value(i);
                        break;
                    }
                }
                stack_push(bool_value(has_next));
                return;
            }
            default:
                goto error;
        }
    } else if (name == NEXT) {
        switch (native_object->native_type) {
            case NativeRangeIter: {
                stack_pop();
                int result = as_int(native_object->values[0]) += as_int(native_object->values[2]);
                stack_push(int_value(result));
                return;
            }
            case NativeArrayIter: {
                stack_pop();
                Value result = as_array(native_object->values[1])->values[as_int(native_object->values[0])++];
                stack_push(result);
                return;
            }
            case NativeMapIter: {
                stack_pop();
                MapEntry entry = as_map(native_object->values[1])->backing[as_int(native_object->values[0])];
                as_int(native_object->values[0])++;
                Array *tuple = new_array(2, false);
                tuple->values[0] = entry.key;
                tuple->values[1] = entry.value;
                stack_push(ref_value((Object *)tuple));
                return;
            }
            default:
                goto error;
        }
    } else {
        error:
        throw_new_runtime_error(Error_TypeError, "TypeError: target does not support such operation");
    }
}

/**
 * 调用一个property。receiver是stack_peek(arg_count). 如果没有找到那个属性，该函数会触发runtime error
 * @param name 名字
 * @param arg_count 参数个数
 */
static void invoke_property(String *name, int arg_count) {
    Value receiver = stack_peek(arg_count);
    if (is_ref(receiver)){
        switch (receiver.as.reference->type) {
            case OBJ_INSTANCE: {
                Instance *instance = as_instance(receiver);
                Value closure_value;
                if (table_get(&instance->fields, name, &closure_value)) {
                    call_value(closure_value, arg_count);
                } else {
                    invoke_from_class(instance->class, name, arg_count);
                }
                break;
            }
            case OBJ_CLASS: {
                Class *class = as_class(receiver);
                Value static_field;
                if (table_get(&class->static_fields, name, &static_field)) {
                    call_value(static_field, arg_count);
                } else {
                    invoke_from_class(class_class, name, arg_count);
                }
                break;
            }
            case OBJ_MODULE: {
                Module *module = as_module(receiver);
                Value property;
                if (table_conditional_get(&module->globals, name, &property, true, false)) {
                    call_value(property, arg_count);
                } else {
                    invoke_from_class(module_class, name, arg_count);
                }
                break;
            }
            case OBJ_ARRAY:
                invoke_from_class(array_class, name, arg_count);
                break;
            case OBJ_STRING:
                invoke_from_class(string_class, name, arg_count);
                break;
            case OBJ_MAP:
                invoke_from_class(map_class, name, arg_count);
                break;
            case OBJ_NATIVE_OBJECT: {
                NativeObject *native_object = as_native_object(receiver);
                invoke_native_object(arg_count, name, native_object);
                break;
            }
            case OBJ_CLOSURE:
                invoke_from_class(closure_class, name, arg_count);
                break;
            case OBJ_METHOD:
                invoke_from_class(method_class, name, arg_count);
                break;
            case OBJ_FUNCTION:
                invoke_from_class(function_class, name, arg_count);
                break;
            case OBJ_NATIVE:
                invoke_from_class(native_class, name, arg_count);
                break;
            case OBJ_NATIVE_METHOD:
                invoke_from_class(native_method_class, name, arg_count);
                break;
            default:
                throw_new_runtime_error(Error_PropertyError, "PropertyError: no such property: %s", name->chars);
        }
    } else {
        switch (receiver.type) {
            case VAL_INT:
            case VAL_BOOL:
            case VAL_FLOAT:
            case VAL_NIL:
                invoke_from_class(value_class(receiver), name, arg_count);
                break;
            default:
                IMPLEMENTATION_ERROR("bad");
                throw_new_runtime_error(Error_FatalError, "Implementation error");
        }
    }
}


static Value multi_dimension_array(int dimension, Value *lens) {
    Value len_value = *lens;
    assert_value_type(len_value, VAL_INT, "int");
    int len = as_int(*lens);
    Array *arr;
    if (dimension == 1) {
        arr = new_array(len, true);
        return ref_value((Object *) arr);
    } else {
        arr = new_array(len, true);
    }
    stack_push(ref_value((Object *) arr));
    for (int i = 0; i < len; ++i) {
        arr->values[i] = multi_dimension_array(dimension - 1, lens + 1);
    }
    stack_pop();
    return ref_value((Object *) arr);

}

/**
 * 如果遇到异常，调用throw_user_level_runtime_error()
 */
static void binary_number_op(Value a, Value b, char operator) {
    if (is_int(a) && is_int(b)) {
        int a_v = as_int(a);
        int b_v = as_int(b);
        switch (operator) {
            case '+':
                stack_push(int_value(a_v + b_v));
                break;
            case '-':
                stack_push(int_value(a_v - b_v));
                break;
            case '*':
                stack_push(int_value(a_v * b_v));
                break;
            case '/':
                stack_push(int_value(a_v / b_v));
                break;
            case '%':
                stack_push(int_value(positive_mod(a_v, b_v)));
                break;
            case '>':
                stack_push(bool_value(a_v > b_v));
                break;
            case '<':
                stack_push(bool_value(a_v < b_v));
                break;
            default:
                IMPLEMENTATION_ERROR("invalid binary operator");
                return;
        }
    } else if (is_int(a) && is_float(b)) {
        int a_v = as_int(a);
        double b_v = as_float(b);
        switch (operator) {
            case '+':
                stack_push(float_value(a_v + b_v));
                break;
            case '-':
                stack_push(float_value(a_v - b_v));
                break;
            case '*':
                stack_push(float_value(a_v * b_v));
                break;
            case '/':
                stack_push(float_value(a_v / b_v));
                break;
            case '>':
                stack_push(bool_value(a_v > b_v));
                break;
            case '<':
                stack_push(bool_value(a_v < b_v));
                break;
            default:
                throw_user_level_runtime_error(Error_TypeError, "TypeError: operands do no support such operation");
                return;
        }
    } else if (is_float(a) && is_int(b)) {
        double a_v = as_float(a);
        int b_v = as_int(b);
        switch (operator) {
            case '+':
                stack_push(float_value(a_v + b_v));
                break;
            case '-':
                stack_push(float_value(a_v - b_v));
                break;
            case '*':
                stack_push(float_value(a_v * b_v));
                break;
            case '/':
                stack_push(float_value(a_v / b_v));
                break;
            case '>':
                stack_push(bool_value(a_v > b_v));
                break;
            case '<':
                stack_push(bool_value(a_v < b_v));
                break;
            default:
                throw_user_level_runtime_error(Error_TypeError, "TypeError: operands do no support such operation");
                return;
        }
    } else if (is_float(a) && is_float(b)) {
        double a_v = as_float(a);
        double b_v = as_float(b);
        switch (operator) {
            case '+':
                stack_push(float_value(a_v + b_v));
                break;
            case '-':
                stack_push(float_value(a_v - b_v));
                break;
            case '*':
                stack_push(float_value(a_v * b_v));
                break;
            case '/':
                stack_push(float_value(a_v / b_v));
                break;
            case '>':
                stack_push(bool_value(a_v > b_v));
                break;
            case '<':
                stack_push(bool_value(a_v < b_v));
                break;
            default:
                throw_user_level_runtime_error(Error_TypeError, "TypeError: operands do no support such operation");
                return;
        }
    } else if (operator == '+' && (is_ref_of(a, OBJ_STRING) || is_ref_of(b, OBJ_STRING))) {
        stack_push(a);
        stack_push(b);
        String *str = string_concat(a, b);
        stack_pop();
        stack_pop();
        stack_push(ref_value(&str->object));
        return;
    } else {
        throw_user_level_runtime_error(Error_TypeError, "TypeError: the operands do no support the operation: %c", operator);
        return;
    }
}

static void show_stack() {
    printf(" ");
    for (Value *i = vm.stack; i < vm.stack_top; i++) {
        if (i == curr_frame->FP) {
            start_color(BOLD_RED);
            printf("@");
            end_color();
        } else {
            printf(" ");
        }
        printf("[");
        print_value_with_color(*i);
        printf("]");
    }
    NEW_LINE();

}

static inline bool is_falsy(Value value) {
    if (is_bool(value)) {
        return as_bool(value) == false;
    }
    if (is_nil(value)) {
        return true;
    }
    return false;
}

static inline void reset_stack() {
    vm.stack_top = vm.stack;
    vm.frame_count = 0;
    curr_frame = NULL;
    vm.open_upvalues = NULL;
}

static inline Value stack_peek(int distance) {
    return vm.stack_top[-1 - distance];
}

static void print_error(Value value) {
    Class *error_class = value_class(value);
    if (is_ref_of(value, OBJ_INSTANCE) && is_subclass(error_class, Error)) {
        Value temp;
        Instance *err = as_instance(value);
        table_get(&err->fields, MESSAGE, &temp);
        String *message = as_string(temp);
        table_get(&err->fields, POSITION, &temp);
        String *position = as_string(temp);
        printf("%s\n%s", message->chars, position->chars);
    } else {
        char *str = value_to_chars(value, NULL);
        printf("A non-Error value is thrown: %s\n", str);
        free(str);
    }
}

/**
 * 将虚拟机的状态复原为上一次try时所设置的TrySavePoint。
 * 如果不存在try，使用longjmp()传递INTERPRET_RUNTIME_ERROR。
 * @param value 被抛出的值。重置虚拟机后，将之置于栈顶。如果该值是一个Error, 那么在这里设置它的backtrace
 */
static void throw_value(Value value) {

    stack_push(value); // prevent gc

    if (is_ref_of(value, OBJ_INSTANCE)) {
        if (is_subclass(value_class(value), Error)) {
            Instance *err = as_instance(value);
            Value bt = native_backtrace(0, NULL);
            table_add_new(&err->fields, POSITION, bt, true, false);
        }
    }

    TrySavePoint *last_save = vm.last_save;
    if (last_save == NULL) {
        print_error(value);
        longjmp(error_buf, INTERPRET_RUNTIME_ERROR);
        return;
    }

    close_upvalue(last_save->stack_top);
    vm.frame_count = last_save->frame_count;
    vm.stack_top = last_save->stack_top;
    sync_frame_cache();
    curr_frame->PC = last_save->PC;
    stack_push(value);

    vm.last_save = last_save->next; // clean
    free(last_save);
}

/**
 * 和throw_new_runtime_error()的唯一区别在于，该函数认为生成的异常可以被虚拟机自行解决，因此不会自动调用catch()。
 */
void throw_user_level_runtime_error(ErrorType type, const char *format, ...) {
    static char buf[RUNTIME_ERROR_VA_BUF_LEN];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, RUNTIME_ERROR_VA_BUF_LEN, format, args);
    va_end(args);

    new_error(type, buf);
    Value err = stack_pop();
    throw_value(err); // 如果没有try—savepoint，那么这个函数会直接导致虚拟机结束，而不会运行下一行的catch
}

/**
 * 抛出的异常可以有两种：
 * 1. 用户通过throw语句抛出的异常，throw_value()可以正确地调整虚拟机状态，处理这种情况。此时虚拟机只需要继续向下执行即可。
 * 2. 虚拟机实现层面出现的异常（比如访问不存在的变量），这些操作常常嵌套在其他的虚拟机逻辑内部，throw_new_runtime_error()
 * 被设计来处理这种情况，会调用catch来返回到上级，然后让上级重新调用run_frame_until()
 */
void throw_new_runtime_error(ErrorType type, const char *format, ...) {
    static char buf[RUNTIME_ERROR_VA_BUF_LEN];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, RUNTIME_ERROR_VA_BUF_LEN, format, args);
    va_end(args);

    new_error(type, buf);
    Value err = stack_pop();
    throw_value(err); // 如果没有try—savepoint，那么这个函数会直接导致虚拟机结束，而不会运行下一行的catch
    catch(INTERPRET_ERROR_CAUGHT);
}

/**
 * 输出错误消息（会自动添加换行符）
 * 打印调用栈消息。清空栈。
 */
void runtime_error(const char *format, ...) {
    fputs("\nRuntime Error: ", stderr);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frame_count - 1; i >= 0; i--) {
        CallFrame *frame = vm.frames + i;
        LoxFunction *function = frame->closure->function;
        size_t index = frame->PC - frame->closure->function->chunk.code - 1;
        int line = function->chunk.lines[index];
        fprintf(stderr, "at [line %d] in ", line);
        char *name = value_to_chars(ref_value((Object *) frame->closure), NULL);
        fprintf(stderr, "%s\n", name);
        free(name);
    }

    reset_stack();
}

/**
 * 跳转到错误处理处。
 * 该函数只应该用在runtime_error()后面。
 */
inline void catch(InterpretResult result) {
    longjmp(error_buf, result);
}
/**
 * 读取curr_frame的下一个字节，然后移动PC
 * @return 下一个字节
 */
static inline uint8_t read_byte() {
    return *curr_frame->PC++;
}

static inline bool if_read_byte(OpCode op) {
    if (*curr_frame->PC == op) {
        curr_frame->PC++;
        return true;
    } else {
        return false;
    }
}

static inline uint16_t read_uint16() {
    uint8_t i0 = read_byte();
    uint8_t i1 = read_byte();
    return u8_to_u16(i0, i1);
}

static inline Value read_constant16() {
    return curr_const_pool[read_uint16()];
}

/**
 * 将下一个u16解释为字符串常数索引，返回其对应的String
 * @return 下一个字节代表的String
 */
static inline String *read_constant_string() {
    Value value = read_constant16();
#ifdef IMPLEMENTATION_CHECK
    if (!is_ref_of(value, OBJ_STRING)) {
        IMPLEMENTATION_ERROR("trying to read a constant string, but the value is not a String");
        return NULL;
    }
#endif
    return (String *) (as_ref(value));
}

/**
 * 返回一个捕获了当前处于value位置上的那个值的UpValueObject对象。
 * 如果那个只是第一次被捕获，新建UpValueObject，将其添加入链表中，然后返回之，否则沿用旧有的。
 */
static UpValue *capture_upvalue(Value *value) {
    // vm.open_upvalues 是有序的。第一个元素的position值是最大的（处于stack中更高的位置）
    UpValue *curr = vm.open_upvalues;
    UpValue *pre = NULL;
    while (curr != NULL && curr->position > value) {
        pre = curr;
        curr = curr->next;
    }
    // 三种情况：curr为NULL；curr->position == value; curr->position < value

    if (curr != NULL && curr->position == value) {
        return curr;
    }

    UpValue *new_capture = new_upvalue(value);
    new_capture->next = curr;
    if (pre == NULL) {
        vm.open_upvalues = new_capture;
    } else {
        pre->next = curr;
    }

    return new_capture;
}


/**
 * 将vm.open_upvalues中所有位置处于position以及其上的upvalue的值进行“close”，
 * 也就是说，将对应的栈上的值保存到UpValueObject其自身中。
 */
static void close_upvalue(Value *position) {
    UpValue *curr = vm.open_upvalues;
    while (curr != NULL && curr->position >= position) {
        curr->closed = *curr->position; // 把栈上的值保存入closed中
        curr->position = &curr->closed; // 重新设置position。如此一来upvalue的get，set指令可以正常运行
        curr = curr->next;
    }
    vm.open_upvalues = curr;
}


/**
 * 在指定的class的methods中寻找某个closure，然后将其包装成一个Method.
 * 如果没找到，该函数自身会调用runtime error
 */
static Value bind_method(Class *class, String *name, Value receiver) {
    Value value;
    if (class == NULL || table_get(&class->methods, name, &value) == false) {
        throw_new_runtime_error(Error_PropertyError, "PropertyError: no such property: %s", name->chars);
        return nil_value();
    } else if (is_ref_of(value, OBJ_CLOSURE)){
        Method *method = new_method(as_closure(value), receiver);
        return ref_value_cast(method);
    } else if (is_ref_of(value, OBJ_NATIVE)) {
        NativeMethod *method = new_native_method(as_native(value), receiver);
        return ref_value_cast(method);
    } else {
        throw_new_runtime_error(Error_PropertyError, "PropertyError: no such property: %s", name->chars);
        return nil_value();
    }
}


/**
 * 在指定的class中寻找对应method，然后调用之。
 * 在调用该函数时，请确保stack_peek(arg_count)为method的receiver。
 * 如果没有找到对应的method，该函数自己会发出runtime error
 */
static void invoke_from_class(Class *class, String *name, int arg_count) {

    // stack: obj, arg1, arg2, top
    // code: op, name_index, arg_count
    Value callable;
    if (!table_get(&class->methods, name, &callable)) {
        throw_new_runtime_error(Error_PropertyError, "PropertyError: no such property: %s", name->chars);
    }
    if (is_ref_of(callable, OBJ_CLOSURE)) {
        call_closure(as_closure(callable), arg_count);
    } else if (is_ref_of(callable, OBJ_NATIVE)) {
        call_native(as_native(callable), arg_count);
    }
}

static void call_closure(Closure *closure, int arg_count) {
    LoxFunction *function = closure->function;
    int fixed_arg_count = function->fixed_arg_count;
    int optional_arg_count = function->optional_arg_count;
    if (arg_count >= fixed_arg_count) {
        // fixed: 2, option: 1, got: 5 -> num_absence = -2, array_len = 2
        // fixed: 1, option: 1, va, got: 1 -> num_absence = 1
        int num_absence = fixed_arg_count + optional_arg_count - arg_count;
        if (num_absence >= 0) { // some optional args are missing
            for (int i = 0; i < num_absence; ++i) {
                stack_push(absence_value());
            }
            arg_count = fixed_arg_count + optional_arg_count;
            if (function->var_arg) {
                stack_push(ref_value((Object *) new_array(0, false)));
                arg_count++;
            }
        } else if (function->var_arg) { // more than expect
            arg_count = fixed_arg_count + optional_arg_count + 1;
            if (if_read_byte(OP_ARR_AS_VAR_ARG)) {
                if (num_absence == -1) {
                    Value top = stack_peek(0);
                    if (!is_ref_of(top, OBJ_ARRAY)) {
                        throw_new_runtime_error(Error_ArgError, "ArgError: cannot use a non-array value as var arg");
                    }
                } else {
                    throw_new_runtime_error(Error_ArgError, "ArgError: too many arguments when using an array as the var arg");
                }
            } else {
                build_array(-num_absence);
            }
        } else {
            throw_new_runtime_error(Error_ArgError, "ArgError: %s expects at most %d arguments, but got %d", function->name->chars,
                                    fixed_arg_count + optional_arg_count, arg_count);
        }
    } else {
        throw_new_runtime_error(Error_ArgError, "ArgError: %s expects at least %d arguments, but got %d", function->name->chars, fixed_arg_count,
                                arg_count);
    }
    if (vm.frame_count == FRAME_MAX) {
        throw_new_runtime_error(Error_FatalError, "FatalError: Stack overflow");
    }
    vm.frame_count++;
    curr_frame = vm.frames + vm.frame_count - 1;
    CallFrame *frame = curr_frame;
    frame->FP = vm.stack_top - arg_count - 1;
    frame->PC = function->chunk.code;
    frame->closure = closure;
    sync_frame_cache();
}

/**
 * 注意，如果该native是作为method调用的，那么其receiver处于values[-1]的位置
 */
static void call_native(NativeFunction *native , int arg_count) {
    if (native->arity != arg_count && native->arity != -1) {
        throw_new_runtime_error(Error_ArgError, "ArgError: %s expects %d arguments, but got %d", native->name->chars, native->arity,
                                arg_count);
    }
    Value result = native->impl(arg_count, vm.stack_top - arg_count); //
    vm.stack_top -= arg_count + 1;
    stack_push(result);
}

/**
 * 创建一个call frame。
 * 如果value不可调用，或者参数数量不匹配，则出现runtime错误
 * @param value 要调用的closure、method、native、class
 * @param arg_count 传入的参数的个数
 */
static void call_value(Value value, int arg_count) {
    if (!is_ref(value)) {
        throw_new_runtime_error(Error_TypeError, "TypeError: Calling a non-callable value");
    }
    switch (as_ref(value)->type) {
        case OBJ_CLOSURE: {
            call_closure(as_closure(value), arg_count);
            break;
        }
        case OBJ_NATIVE: {
            NativeFunction *native = as_native(value);
            call_native(native, arg_count);
            break;
        }
        case OBJ_CLASS: {
            Class *class = as_class(value);
            Instance *instance = new_instance(class);
            Value init_closure;
            if (table_get(&class->methods, INIT, &init_closure)) {
                Method *initializer = new_method(as_closure(init_closure), ref_value((Object *) instance));
                call_value(ref_value((Object *) initializer), arg_count);
            } else if (arg_count != 0) {
                throw_new_runtime_error(Error_ArgError, "ArgError: %s does not define init() but got %d arguments", class->name->chars,
                                        arg_count);
            } else {
                stack_pop();
                stack_push(ref_value((Object *) instance));
            }
            break;
        }
        case OBJ_METHOD: {
            Method *method = as_method(value);
            vm.stack_top[-arg_count - 1] = method->receiver;
            call_closure(method->closure, arg_count);
            break;
        }
        case OBJ_NATIVE_METHOD: {
            NativeMethod *method = as_native_method(value);
            vm.stack_top[-arg_count - 1] = method->receiver;
            call_native(method->fun, arg_count);
            break;
        }
        default: {
            throw_new_runtime_error(Error_TypeError, "TypeError: Calling a non-callable value");
        }
    }
}

void init_VM() {
    reset_stack();
    vm.objects = NULL;
    vm.open_upvalues = NULL;
    vm.gray_count = 0;
    vm.gray_capacity = 0;
    vm.gray_stack = NULL;
    vm.allocated_size = 0;
    vm.next_gc = INITIAL_GC_SIZE;
    vm.last_save = NULL;
    srand(time(NULL)); // NOLINT(*-msc51-cpp)

    init_table(&vm.builtin);
    init_table(&vm.string_table);
    init_static_strings();

    init_vm_native();
}

/**
 * 清除下列数据
 * @par objects
 * @par string_table
 * @par globals
 * @par const_table
 */
void free_VM() {
    free_all_objects();
    free_table(&vm.builtin);
    free_table(&vm.string_table);
//    free_table(&vm.globals);
    free(vm.gray_stack);
}

/**
 * 调用 compile() 将源代码编译成一个function，构造一个closure，设置栈帧，
 * 将closure置入栈中，然后用run()运行代码，返回其结果。
 * 在repl中，每一次输入都将执行该函数。
 * 如果是repl，那么使用repl_module为初始module；否则，创建新的module
 *
 * @param src 要执行的代码
 * @parem path 该脚本的路径。如果是REPL模式，不会用到该值。
 * @return 执行结果（是否出错等）
 */
InterpretResult interpret(const char *src, const char *path) {

    LoxFunction *function = compile(src);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    warmup(function, path, NULL, true);

    InterpretResult error;

    error = setjmp(error_buf);
    if (error == INTERPRET_0 || error == INTERPRET_ERROR_CAUGHT) {
        error = run_frame_until(0);
    }

    return error;
}

/**
 * 根据给定的源代码，编译一个函数，然后将其设置为接下来要执行的closure
 */
static void import(const char *src, String *path) {

    LoxFunction *function = compile(src);

    if (function == NULL) {
        throw_new_runtime_error(Error_CompileError, "CompileError: the module fails to compile");
        return;
    }

    warmup(function, NULL, path, false);

}

/**
 * 编译源代码，输出字节码
 */
InterpretResult produce(const char *src, const char *path) {
    LoxFunction *function = compile(src);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        printf("Error when opening the file: %s\n", path);
        return INTERPRET_BYTECODE_WRITE_ERROR;
    }
    write_function(file, function);
    fclose(file);
    return INTERPRET_PRODUCE_OK;
}

InterpretResult read_run_bytecode(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        printf("Error when opening the file: %s\n", path);
        return INTERPRET_BYTECODE_READ_ERROR;
    }

    DISABLE_GC;
    LoxFunction *function = read_function(file);
    ENABLE_GC;

    fclose(file);
    warmup(function, path, NULL, false);

    InterpretResult error;
    error = setjmp(error_buf);
    if (error == INTERPRET_0 || error == INTERPRET_ERROR_CAUGHT) {
        error = run_frame_until(0);
    }
    return error;
}

InterpretResult disassemble_byte_code(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        printf("Error when opening the file: %s\n", path);
        return INTERPRET_BYTECODE_READ_ERROR;
    }

    DISABLE_GC;
    LoxFunction *function = read_function(file);
    ENABLE_GC;

    int result = disassemble_chunk(&function->chunk, path);
    if (result == -1) {
        return INTERPRET_BYTECODE_DISASSEMBLE_ERROR;
    } else {
        return INTERPRET_BYTECODE_DISASSEMBLE_OK;
    }
}

/**
 * 用fmemopen()从目标字节数组处打开一个流，创建一个新的全局命名空间，然后读取并执行字节码。运行后，将全局命名空间中的public元素添加到builtin命名空间中
 */
InterpretResult load_bytes_into_builtin(unsigned char *bytes, size_t len, const char *path) {
    FILE *file = fmemopen(bytes, len, "rb");

    DISABLE_GC;
    LoxFunction *function = read_function(file);
    fclose(file);
    ENABLE_GC;

    warmup(function, path, NULL, false);

    InterpretResult error;

    error = setjmp(error_buf);
    if (error == INTERPRET_0 || error == INTERPRET_ERROR_CAUGHT) {
        error = run_frame_until(0);
    }
    vm.frame_count = 1; // so that the module at frames[0] won't be gc when doing table_add_all()
    if (error == INTERPRET_EXECUTE_OK) {
        table_add_all(&vm.frames[0].module->globals, &vm.builtin, true);
    }
    vm.frame_count = 0;

    return error;
}


/**
 * 使用给定的函数，构造一个closure，设置栈帧，将closure置入栈中。
 * 设置vm.current_module, curr_frame, curr_closure_global, curr_const_pool等值。
 * @param function 要执行的main函数
 * @param path_chars 该模块的路径。如果为NULL，则改为使用path_string
 * @param path_string 如果path_chars为NULL，那么该模块的路径由该值决定
 * @param care_repl 如果 care_repl && REPL，则不创建新的模块，而是使用先前保存的repl_module
 */
static void warmup(LoxFunction *function, const char *path_chars, String *path_string, bool care_repl) {

    DISABLE_GC;

    vm.frame_count++;

    curr_frame = vm.frames + vm.frame_count - 1;

    Closure *closure = new_closure(function);
    curr_frame->closure = closure;
    curr_frame->FP = vm.stack_top;
    curr_frame->PC = function->chunk.code;

    Module *module;
    if (care_repl && REPL) {
        module = repl_module;
    } else if (path_chars != NULL) {
        module = new_module(auto_length_string_copy(path_chars));
    } else {
        module = new_module(path_string);
    }

    closure->module_of_define = module;
    curr_frame->module = module;
    sync_frame_cache();

    stack_push(ref_value((Object *) closure));

    ENABLE_GC;
}

/**
 * 消耗栈顶的length个数组，将它们组成一个数组，然后置于栈顶
 * @param length
 */
static void build_array(int length) {
    Array *array = new_array(length, false);
    for (int i = length - 1; i >= 0; i--) {
        array->values[i] = stack_pop();
    }
    stack_push(ref_value((Object *) array));
}

/**
 * 遍历虚拟机的 curr_frame 的 function 的 chunk 中的每一个指令，执行之. 运行直到vm.frame_count == end_when.
 * 如果发生了runtime error，该函数将立刻返回。
 * @return 执行的结果
 */
static InterpretResult run_frame_until(int end_when) {
    if (vm.frame_count == end_when) {
        return INTERPRET_EXECUTE_OK;
    }

    while (true) {

        if (TRACE_EXECUTION && TRACE_SKIP == -1 && preload_finished) {
            show_stack();
            if (getchar() == 'o') {
                TRACE_SKIP = vm.frame_count - 1; // skip until the frame_count is equal to TRACE_SKIP
                while (getchar() != '\n');
            } else {
                CallFrame *frame = curr_frame;
                disassemble_instruction(&frame->closure->function->chunk,
                                        (int) (frame->PC - frame->closure->function->chunk.code), false);
            }
        }

        uint8_t instruction = read_byte();
        switch (instruction) {
            case OP_RETURN: {
                Value result = stack_pop(); // 返回值
                vm.stack_top = curr_frame->FP;
                close_upvalue(vm.stack_top);
                vm.frame_count--;
                if (vm.frame_count == TRACE_SKIP) {
                    TRACE_SKIP = -1; // no skip. Step by step
                }
                if (vm.frame_count != 0) {
                    sync_frame_cache();
                    stack_push(result);
                }
                if (vm.frame_count == end_when) {
                    return INTERPRET_EXECUTE_OK;
                }
                break;
            }
            case OP_LOAD_CONSTANT: {
                Value value = read_constant16();
                stack_push(value);
                break;
            }
            case OP_NEGATE: {
                Value value = stack_peek(0);
                if (is_int(value)) {
                    stack_push(int_value(-as_int(stack_pop())));
                    break;
                } else if (is_float(value)) {
                    stack_push(float_value(-as_float(stack_pop())));
                    break;
                } else {
                    throw_new_runtime_error(Error_TypeError, "TypeError: the value of cannot be negated");
                }
                break;
            }
            case OP_ADD: {
                Value b = stack_pop();
                Value a = stack_pop();
                binary_number_op(a, b, '+');
                break;
            }
            case OP_SUBTRACT: {
                Value b = stack_pop();
                Value a = stack_pop();
                binary_number_op(a, b, '-');
                break;
            }
            case OP_MULTIPLY: {
                Value b = stack_pop();
                Value a = stack_pop();
                binary_number_op(a, b, '*');
                break;
            }
            case OP_DIVIDE: {
                Value b = stack_pop();
                Value a = stack_pop();
                binary_number_op(a, b, '/');
                break;
            }
            case OP_MOD: {
                Value b = stack_pop();
                Value a = stack_pop();
                binary_number_op(a, b, '%');
                break;
            }
            case OP_POWER: {
                Value b = stack_pop();
                Value a = stack_pop();
                if (is_number(a) && is_number(b)) {
                    stack_push(float_value(pow(AS_NUMBER(a), AS_NUMBER(b))));
                } else {
                    throw_user_level_runtime_error(Error_TypeError, "TypeError: the operands do not support the power operation");
                }
                break;
            }
            case OP_TEST_LESS: {
                Value b = stack_pop();
                Value a = stack_pop();
                binary_number_op(a, b, '<');
                break;
            }
            case OP_TEST_GREATER: {
                Value b = stack_pop();
                Value a = stack_pop();
                binary_number_op(a, b, '>');
                break;
            }
            case OP_TEST_EQUAL: {
                Value b = stack_pop();
                Value a = stack_pop();
                stack_push(bool_value(value_equal(a, b)));
                break;
            }
            case OP_LOAD_NIL:
                stack_push(nil_value());
                break;
            case OP_LOAD_TRUE:
                stack_push(bool_value(true));
                break;
            case OP_LOAD_FALSE:
                stack_push(bool_value(false));
                break;
            case OP_NOT:
                stack_push(bool_value(is_falsy(stack_pop())));
                break;
            case OP_PRINT: {
                if (REPL || TRACE_EXECUTION) {
                    start_color(BOLD_GREEN);
                }
                print_value(stack_pop());
                NEW_LINE();
                if (REPL || TRACE_EXECUTION) {
                    end_color();
                }
                break;
            }
            case OP_REPL_AUTO_PRINT: {
                Value to_print = stack_pop();
                start_color(GRAY);
                print_value(to_print);
                NEW_LINE();
                end_color();
                break;
            }
            case OP_POP:
                stack_pop();
                break;
            case OP_DEF_GLOBAL: {
                String *name = read_constant_string();
                if (!table_add_new(curr_closure_global, name, stack_peek(0), false, false)) {
                    throw_user_level_runtime_error(Error_NameError, "NameError: re-defining the existent global variable %s", name->chars);
                } else {
                    stack_pop();
                }
                break;
            }
            case OP_DEF_GLOBAL_CONST: {
                String *name = read_constant_string();
                if (!table_add_new(curr_closure_global, name, stack_peek(0), false, true)) {
                    throw_user_level_runtime_error(Error_NameError, "NameError: re-defining the existent global variable %s", name->chars);
                } else {
                    stack_pop();
                }
                break;
            }
            case OP_DEF_PUB_GLOBAL: {
                String *name = read_constant_string();
                if (!table_add_new(curr_closure_global, name, stack_peek(0), true, false)) {
                    throw_user_level_runtime_error(Error_NameError, "NameError: re-defining the existent global variable %s", name->chars);
                } else {
                    stack_pop();
                }
                break;
            }
            case OP_DEF_PUB_GLOBAL_CONST: {
                String *name = read_constant_string();
                if (!table_add_new(curr_closure_global, name, stack_peek(0), true, true)) {
                    throw_user_level_runtime_error(Error_NameError, "NameError: re-defining the existent global variable %s", name->chars);
                } else {
                    stack_pop();
                }
                break;
            }
            case OP_GET_GLOBAL: {
                String *name = read_constant_string();
                Value value;
                if (table_get(curr_closure_global, name, &value)) {
                    stack_push(value);
                } else if (table_get(&vm.builtin, name, &value)) {
                    stack_push(value);
                } else {
                    throw_user_level_runtime_error(Error_NameError, "NameError: accessing an undefined variable: %s", name->chars);
                }
                break;
            }
            case OP_SET_GLOBAL: {
                String *name = read_constant_string();
                char result = table_set_existent(curr_closure_global, name, stack_peek(0), false);
                if (result != 0) {
                    if (result == 1) {
                        throw_user_level_runtime_error(Error_NameError, "NameError: setting an undefined variable: %s", name->chars);
                    } else {
                        throw_user_level_runtime_error(Error_NameError, "NameError: setting a const variable: %s", name->chars);
                    }
                }
                break;
            }
            case OP_GET_LOCAL: {
                int index = read_byte();
                stack_push(curr_frame->FP[index]);
                break;
            }
            case OP_SET_LOCAL: {
                int index = read_byte();
                curr_frame->FP[index] = stack_peek(0);
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = read_uint16();
                if (is_falsy(stack_peek(0))) {
                    curr_frame->PC += offset;
                }
                break;
            }
            case OP_JUMP_IF_TRUE: {
                uint16_t offset = read_uint16();
                if (!is_falsy(stack_peek(0))) {
                    curr_frame->PC += offset;
                }
                break;
            }
            case OP_JUMP: {
                uint16_t offset = read_uint16();
                curr_frame->PC += offset;
                break;
            }
            case OP_JUMP_BACK: {
                uint16_t offset = read_uint16();
                curr_frame->PC -= offset;
                break;
            }
            case OP_JUMP_IF_NOT_EQUAL: {
                uint16_t offset = read_uint16();
                Value b = stack_peek(0);
                Value a = stack_peek(1);
                if (!value_equal(a, b)) {
                    curr_frame->PC += offset;
                }
                break;
            }
            case OP_POP_JUMP_IF_FALSE: {
                uint16_t offset = read_uint16();
                if (is_falsy(stack_pop())) {
                    curr_frame->PC += offset;
                }
                break;
            }
            case OP_POP_JUMP_IF_TRUE: {
                uint16_t offset = read_uint16();
                if (!is_falsy(stack_pop())) {
                    curr_frame->PC += offset;
                }
                break;
            }
            case OP_CALL: {
                int count = read_byte();
                Value callee = stack_peek(count);
                call_value(callee, count);
                break;
            }
            case OP_MAKE_CLOSURE: {
                /* 值得注意的是，对于嵌套的closure，虽然在编译时，是内部的
                 * 函数先编译，但是在运行时，是先执行外层函数的OP_closure。
                 * 然后等到外层函数被调用、内层函数被定义后，内层函数的OP_closure
                 * 才会被执行
                 */
                Value f = read_constant16();
                Closure *closure = new_closure(as_function(f));
                closure->module_of_define = curr_frame->module;
                stack_push(ref_value((Object *) closure));
                for (int i = 0; i < closure->upvalue_count; ++i) {
                    bool is_local = read_byte();
                    int index = read_byte();
                    if (is_local) {
                        closure->upvalues[i] = capture_upvalue(curr_frame->FP + index);
                    } else {
                        closure->upvalues[i] = curr_frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_GET_UPVALUE: {
                int index = read_byte();
                stack_push(*curr_frame->closure->upvalues[index]->position);
                break;
            }
            case OP_SET_UPVALUE: {
                int index = read_byte();
                *curr_frame->closure->upvalues[index]->position = stack_peek(0);
                break;
            }
            case OP_CLOSE_UPVALUE: {
                close_upvalue(vm.stack_top - 1);
                stack_pop();
                break;
            }
            case OP_MAKE_CLASS: {
                String *name = read_constant_string();
                stack_push(ref_value((Object *) new_class(name)));
                break;
            }
            case OP_GET_PROPERTY: {
                Value value = stack_pop(); // instance
                String *field_name = read_constant_string();
                get_property(value, field_name);
                break;
            }
            case OP_SET_PROPERTY: {
                Value target = stack_peek(1); // prevent being gc
                Value value = stack_peek(0);
                String *property_name = read_constant_string();
                if (is_ref_of(target, OBJ_INSTANCE) == false) {
                    if (is_ref_of(target, OBJ_CLASS)) {
                        Class *class = as_class(target);
                        if (table_set_existent(&class->static_fields, property_name, value, false)) {
                            throw_user_level_runtime_error(Error_PropertyError, "PropertyError: %s does not have the static field: %s", class->name->chars,
                                                  property_name->chars);
                        }
                        break;
                    } else if (is_ref_of(target, OBJ_MODULE)) {
                        Module *module = as_module(target);
                        char result = table_set_existent(&module->globals, property_name, value, true);
                        switch (result) {
                            case 0:
                                break;
                            case 1:
                                throw_user_level_runtime_error(Error_PropertyError, "PropertyError: does not find the property: %s",
                                                      property_name->chars);
                                break;
                            case 2:
                                throw_user_level_runtime_error(Error_PropertyError, "PropertyError: cannot modify the const property: %s", property_name->chars);
                                break;
                            case 3:
                                throw_user_level_runtime_error(Error_PropertyError, "PropertyError: cannot access the non-public property: %s",
                                                        property_name->chars);
                                break;
                            default:
                                IMPLEMENTATION_ERROR("bad");
                                break;
                        }
                    } else {
                        throw_user_level_runtime_error(Error_PropertyError, "PropertyError: does not find the property: %s",
                                              property_name->chars);
                    }
                } else {
                    Instance *instance = as_instance(target);
                    table_set(&instance->fields, property_name, value); // potential gc
                    vm.stack_top -= 2;
                    stack_push(value);
                }
                break;
            }
            case OP_MAKE_METHOD: {
                // stack: class, closure,
                // op-method
                Closure *closure = as_closure(stack_peek(0));
                Class *class = as_class(stack_peek(1));
                table_set(&class->methods, closure->function->name, ref_value((Object *) closure));
                stack_pop();
                break;
            }
            case OP_PROPERTY_INVOKE: {
                // stack: [obj, arg1, arg2, top]
                // code: op, name_index, arg_count
                String *name = read_constant_string();
                int arg_count = read_byte();
                invoke_property(name, arg_count);
                break;
            }
            case OP_INHERIT: {
                //  super,sub,top
                Value super = stack_peek(1);
                if (!is_ref_of(super, OBJ_CLASS)) {
                    throw_user_level_runtime_error(Error_TypeError, "TypeError: the value cannot be used as a super class");
                } else {
                    Class *sub = as_class(stack_peek(0));
                    Class *super_class = as_class(super);
                    sub->super_class = super_class;
                    table_add_all(&super_class->methods, &sub->methods, false);
                    stack_pop();
                    // super, top
                }
                break;
            }
            case OP_SUPER_ACCESS: {
                // stack: receiver, super_class, top
                // code: op, name_index
                String *name = read_constant_string();
                Class *class = as_class(stack_pop());
                Value receiver = stack_pop();
                stack_push(bind_method(class, name, receiver));
                // stack: method, top
                break;
            }
            case OP_SUPER_INVOKE: {
                // [receiver, args1, arg2, superclass, top]
                // op, name, arg_count
                String *name = read_constant_string();
                int arg_count = read_byte();
                Class *class = as_class(stack_pop());
                invoke_from_class(class, name, arg_count);
                break;
            }
            case OP_DIMENSION_ARRAY: {
                // [len1, len2, top]
                int dimension = read_byte();
                Value arr = multi_dimension_array(dimension, vm.stack_top - dimension);
                for (int i = 0; i < dimension; ++i) {
                    stack_pop();
                }
                stack_push(arr);
                break;
            }
            case OP_COPY: {
                stack_push(stack_peek(0));
                break;
            }
            case OP_COPY2: {
                stack_push(stack_peek(1));
                stack_push(stack_peek(1));
                break;
            }
            case OP_INDEXING_GET: {
                // [arr, index]
                Value target = stack_peek(1);
                if (is_ref_of(target, OBJ_ARRAY)) {
                    array_indexing_get();
                } else if (is_ref_of(target, OBJ_MAP)) {
                    map_indexing_get();
                } else if (is_ref_of(target, OBJ_STRING)){
                    string_indexing_get();
                } else {
                    throw_user_level_runtime_error(Error_TypeError, "TypeError: TypeError: the value does not support indexing");
                }
                break;
            }
            case OP_INDEXING_SET: {
                // op: [array, index, value]
                Value target = stack_peek(2);
                if (is_ref_of(target, OBJ_ARRAY)) {
                    array_indexing_set();
                } else if (is_ref_of(target, OBJ_MAP)) {
                    map_indexing_set(false);
                } else {
                    throw_user_level_runtime_error(Error_TypeError, "TypeError: the value does not support indexing");
                }
                break;
            }
            case OP_MAKE_ARRAY: {
                int length = read_byte();
                build_array(length);
                break;
            }
            case OP_UNPACK_ARRAY: {
                int len = read_byte();
                // [arr] -> [v0, v1, ... v_n]
                Value v = stack_pop();
                if (!is_ref_of(v, OBJ_ARRAY)) {
                    throw_user_level_runtime_error(Error_TypeError, "TypeError: only arrays can be unpacked");
                    break;
                }
                Array *array = as_array(v);
                if (array->length < len) {
                    throw_user_level_runtime_error(Error_TypeError, "ValueError: array of length %d cannot be unpacked into %d elements", array->length, len);
                    break;
                }
                for (int i = 0; i < len; ++i) {
                    stack_push(array->values[i]);
                }
                break;
            }
            case OP_MAKE_STATIC_FIELD: {
                // [class, field, top]
                String *name = read_constant_string();
                Value field = stack_peek(0);
                Class *class = as_class(stack_peek(1));
                table_add_new(&class->static_fields, name, field, false, false);
                stack_pop();
                break;
            }
            case OP_IMPORT: {
                String *path = as_string(stack_pop());

                char *relative_path;

                char *curr_module_path;
                asprintf(&curr_module_path, "%s", curr_frame->module->path->chars); // copy the string first, because dirname may modify it

                char *curr_dir = dirname(curr_module_path);

                asprintf(&relative_path, "%s/%s", curr_dir, path->chars);

                free(curr_module_path);

                char *absolute_path = resolve_path(relative_path); // no need to free absolute_path

                free(relative_path);

                char *src = read_file(absolute_path);
                if (src == NULL) {
                    throw_user_level_runtime_error(Error_IOError, "IOError: error when reading the file %s (%s)\n", absolute_path, path->chars);
                } else {
                    String *path_string = auto_length_string_copy(absolute_path);
                    import(src, path_string);
                    free(src);
                }
                break;
            }
            case OP_COPY_N: {
                int n = read_byte();
                stack_push(stack_peek(n));
                break;
            }
            case OP_SWAP: {
                int n = read_byte();
                stack_swap(n);
                break;
            }
            case OP_RESTORE_MODULE: {
                // [nil] -> [new_module]
                stack_pop();
                Module *last_module = curr_frame[1].module;
                stack_push(ref_value((Object *) last_module));
                break;
            }
            case OP_EXPORT: {
                String *name = read_constant_string();
                Entry *entry = table_find_entry(&curr_frame->module->globals, name, false, false);
                if (entry->key == NULL && is_bool(entry->value)) {
                    throw_user_level_runtime_error(Error_NameError, "NameError: no such variable: %s", name->chars);
                } else {
                    entry->is_public = true;
                }
                break;
            }
            case OP_LOAD_ABSENCE:
                stack_push(absence_value());
                break;
            case OP_JUMP_IF_NOT_ABSENCE: {
                int offset = read_uint16();
                if (!is_absence(stack_pop())) {
                    curr_frame->PC += offset;
                }
                break;
            }
            case OP_GET_ITERATOR: {
                invoke_property(ITERATOR, 0);
                break;
            }
            case OP_JUMP_FOR_ITER: {
                // [iter]
                int offset = read_uint16();
                stack_push(stack_peek(0)); // [iter, iter]
                invoke_and_wait(HAS_NEXT, 0); // [iter, bool]
                if (is_falsy(stack_pop())) {
                    curr_frame->PC += offset;
                    break;
                }
                // [iter, iter]
                stack_push(stack_peek(0));
                invoke_property(NEXT, 0);
                // [iter, item]
                break;
            }
            case OP_MAP_ADD_PAIR: {
                // [map, k0, v0]
                // [k0, v0, k1, v1, k2, v2 ...] -> [map]
                map_indexing_set(true);
                break;
            }
            case OP_NEW_MAP: {
                stack_push(ref_value((Object *) new_map()));
                break;
            }
            case OP_SET_TRY: {
                int offset = read_uint16();
                TrySavePoint *save_point = malloc(sizeof(TrySavePoint));
                save_point->frame_count = vm.frame_count;
                save_point->PC = curr_frame->PC + offset;
                save_point->stack_top = vm.stack_top;
                save_point->next = vm.last_save;
                vm.last_save = save_point;
                break;
            }
            case OP_SKIP_CATCH: {
                int offset = read_uint16();
                TrySavePoint *last_save = vm.last_save;
                vm.last_save = vm.last_save->next;
                free(last_save);
                curr_frame->PC += offset;
                break;
            }
            case OP_THROW: {
                Value value = stack_pop();
                throw_value(value);
                break;
            }
            case OP_TEST_VALUE_OF: {
                int amount = read_byte();
                // [value, t1, t2, (top)] -> [value, bool]
                bool yes = multi_value_of(amount, vm.stack_top - amount - 1);
                vm.stack_top -= amount;
                stack_push(bool_value(yes));
                break;
            }
            default: {
                IMPLEMENTATION_ERROR("unrecognized instruction");
                break;
            }
        }

    }
}

