//
// Created by Yue Xue  on 11/14/24.
//

#include "memory.h"
#include "assert.h"

void *re_allocate(void *ptr, size_t old_size, size_t new_size) {
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }
    void *new_ptr = realloc(ptr, new_size);
    assert(new_ptr != NULL);
    return new_ptr;
}
