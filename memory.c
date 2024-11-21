//
// Created by Yue Xue  on 11/14/24.
//

#include "memory.h"
#include "assert.h"

/**
 * @param ptr 想要重新分类的指针。为 NULL 则分为新的空间。
 * @param old_size
 * @param byte_size 新的空间大小，以字节为单位
 * */
void *re_allocate(void *ptr, size_t old_size, size_t new_byte_size) {
    if (new_byte_size == 0) {
        free(ptr);
        return NULL;
    }
    void *new_ptr = realloc(ptr, new_byte_size);
    assert(new_ptr != NULL);
    return new_ptr;
}
