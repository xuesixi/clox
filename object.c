#include "object.h"
#include "memory.h"
#include "vm.h"
#include <stdio.h>
#include <string.h>

inline bool is_ref_of(Value value, ObjectType type) {
    return is_ref(value) && as_ref(value)->type == type;
}

inline String *as_string(Value value) {
    return (String*)as_ref(value);
}

String *string_copy(const char *src, int length) {
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, src, length);
    chars[length] = '\0';
    return string_allocate(chars, length);
}

/**
 * 使用给定的 char* 来产生一个 String。
 * */
String *string_allocate(char *chars, int length) {
    String *str = ALLOCATE(String, 1);
    str->object.type = OBJ_STRING;
    str->chars = chars;
    str->length = length;
    return str;
}

String *string_concat(Value a, Value b) {
    if (! (is_ref_of(a, OBJ_STRING) || is_ref_of(b, OBJ_STRING))) {
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
