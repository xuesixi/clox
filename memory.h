//
// Created by Yue Xue  on 11/14/24.
//

#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "stdlib.h"

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    ((type *) re_allocate(pointer, sizeof(type) *(oldCount), sizeof(type) * (newCount)))

#define FREE_ARRAY(type, pointer, oldCount) \
    re_allocate(pointer, sizeof(type) * (oldCount), 0)

#define ALLOCATE(type, length) \
    (type*)(re_allocate(NULL, 0, sizeof(type) * (length)))

void *re_allocate(void *ptr, size_t old_size, size_t byte_size);

#endif //CLOX_MEMORY_H
