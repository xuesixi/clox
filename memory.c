//
// Created by Yue Xue  on 11/14/24.
//

#include "memory.h"

#include "assert.h"
#include "vm.h"

/**
 * @param ptr 想要重新分类的指针。为 NULL 则分为新的空间。
 * @param old_size
 * @param byte_size 新的空间大小，以字节为单位
 * */
void *re_allocate(void *ptr, size_t old_size, size_t new_byte_size) {
    (void )old_size;
    if (new_byte_size == 0) {
        free(ptr);
        return NULL;
    }
    void *new_ptr = realloc(ptr, new_byte_size);
    assert(new_ptr != NULL);
    return new_ptr;
}

void free_all_objects() {
    Object *curr = vm.objects;
    while (curr != NULL) {
        Object *next = curr->next;
        free_object(curr);
        curr = next;
    }
}

void free_object(Object *object) {
    switch (object->type) {
        case OBJ_STRING: {
            String *str = (String*)object;
            FREE_ARRAY(char, 0, str->length + 1);
            re_allocate(object, sizeof(String), 0);
            break;
        }
        case OBJ_FUNCTION: {
            LoxFunction *function = (LoxFunction *) object;
            free_chunk(& function->chunk);
            re_allocate(object, sizeof(LoxFunction), 0);
            break;
        }
        case OBJ_NATIVE: {
            re_allocate(object, sizeof(NativeFunction), 0);
            break;
        }
        case OBJ_CLOSURE: {
            Closure *closure = (Closure *) object;
            FREE_ARRAY(UpValueObject*, closure->upvalues, closure->upvalue_count);
            re_allocate(object, sizeof(Closure), 0);
            break;
        }
        case OBJ_UPVALUE: {
            re_allocate(object, sizeof(UpValueObject), 0);
            break;
        }
        default:
            return;
    }
}
