//
// Created by Yue Xue  on 11/14/24.
//

#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "object.h"
#include "stdlib.h"

#define INITIAL_GC_SIZE 1024
#define GC_GROW_FACTOR 2

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    ((type *) re_allocate(pointer, sizeof(type) *(oldCount), sizeof(type) * (newCount)))

#define FREE_ARRAY(type, pointer, oldCount) \
    re_allocate(pointer, sizeof(type) * (oldCount), 0)

#define ALLOCATE(type, length) \
    (type*)(re_allocate(NULL, 0, sizeof(type) * (length)))

extern bool gc_enabled;

#define ENABLE_GC (gc_enabled = true)
#define DISABLE_GC (gc_enabled = false)

void *re_allocate(void *ptr, size_t old_size, size_t byte_size);
void free_all_objects();
void free_object(Object *object);
void mark_object(Object *object);

#define mark_value(v) \
if ((v).type == VAL_REF) {\
    mark_object(as_ref(v));\
}\

#endif //CLOX_MEMORY_H
