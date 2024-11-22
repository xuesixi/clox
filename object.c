#include "object.h"

#include <string.h>

#include "memory.h"
#include "vm.h"

static uint32_t chars_hash(const char *key, int length);

inline bool is_ref_of(Value value, ObjectType type) {
    return is_ref(value) && as_ref(value)->type == type;
}

inline String *as_string(Value value) {
    return (String *)as_ref(value);
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
        FREE_ARRAY(char , chars, length + 1);
        return interned;
    }

    String *str = (String *)allocate_object(sizeof(String), OBJ_STRING);
    str->object.type = OBJ_STRING;
    str->chars = chars;
    str->length = length;
    str->hash = hash;
    table_set(&vm.string_table, str, nil_value());
    return str;
}

/**
 * 将a 和 b 的字符串表达拼接在一起，产生一个 String
 * */
String *string_concat(Value a, Value b) {
    if (!(is_ref_of(a, OBJ_STRING) || is_ref_of(b, OBJ_STRING))) {
        runtime_error("at least one operand needs to be string");
        return NULL;
    }
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
 * */
Object *allocate_object(size_t size, ObjectType type) {
    Object *obj = re_allocate(NULL, 0, size);
    obj->type = type;
    obj->next = vm.objects;
    vm.objects = obj;
    return obj;
}

static uint32_t chars_hash(const char *key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}