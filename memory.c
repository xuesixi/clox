//
// Created by Yue Xue  on 11/14/24.
//

#include "memory.h"
#include "compiler.h"

#include "assert.h"
#include "vm.h"

static void mark_roots();

void mark_object(Object *object) {
    if (object == NULL) {
        return;
    }

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    print_value(ref_value(object));
    printf("\n");
#endif

    object->is_marked = true;

    if (vm.gray_count == vm.gray_capacity) {
        vm.gray_capacity = vm.gray_capacity < 8 ? 8 : vm.gray_capacity * 2;
        vm.gray_stack = realloc(vm.gray_stack, vm.gray_capacity * sizeof(Object *));
    }

    vm.gray_stack[vm.gray_count ++ ] = object;
}

void mark_value(Value value) {
    if (value.type == VAL_REF) {
        mark_object(as_ref(value));
    }
}

static void mark_roots() {

    // mark stack
    for (Value *curr = vm.stack; curr < vm.stack_top; curr ++) {
        mark_value(*curr);
    }

    // mark global
    table_mark(& vm.globals);

    // mark existing frames。否则调用中的函数可能会被回收
    for (int i = 0; i < vm.frame_count; ++i) {
        mark_object((Object *) vm.frames[i].closure);
    }

    // mark open upvalues ? 暂时无法理解。理论上closure们应该可以引用这些值
    UpValueObject *curr = vm.open_upvalues;
    while (curr != NULL) {
        mark_object((Object *) curr);
        curr = curr->next;
    }

    mark_compiler_roots(); // 在编译过程中也会分配堆内存，因此也可能触发gc

}

static void gc() {
    #ifdef DEBUG_LOG_GC
    printf("---- gc begin\n");
    #endif

    mark_roots();

    #ifdef DEBUG_LOG_GC
    printf("---- gc end\n");
    #endif
}

/**
 * @param ptr 想要重新分类的指针。为 NULL 则分为新的空间。
 * @param old_size
 * @param byte_size 新的空间大小，以字节为单位
 * */
void *re_allocate(void *ptr, size_t old_size, size_t new_byte_size) {
    (void )old_size;

    #ifdef DEBUG_STRESS_GC
    if (new_byte_size > 0) {
        gc();
    }
    #endif

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
#ifdef DEBUG_LOG_GC
    printf("%p is free with type %d\n", object, object->type);
#endif
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
