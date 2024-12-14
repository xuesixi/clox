//
// Created by Yue Xue  on 11/14/24.
//

#include "memory.h"
#include "compiler.h"

#include "assert.h"
#include "vm.h"

static void mark_roots();

void mark_object(Object *object) {
    if (object == NULL || object->is_marked) {
        return;
    }

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    print_value_with_color(ref_value(object));
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

//    // mark open upvalues ? 暂时无法理解。理论上closure们应该可以引用这些值
//    UpValueObject *curr = vm.open_upvalues;
//    while (curr != NULL) {
//        mark_object((Object *) curr);
//        curr = curr->next;
//    }

    mark_compiler_roots(); // 在编译过程中也会分配堆内存，因此也可能触发gc

}

static void blacken_object(Object *object) {
#ifdef DEBUG_LOG_GC
    printf("blacken %p ", object);
    print_value_with_color(ref_value(object));
    NEW_LINE();
#endif
    switch (object->type) {
        case OBJ_STRING:
        case OBJ_NATIVE:
            break;
        case OBJ_UPVALUE: {
            UpValue *up = (UpValue *) object;
            mark_value(up->closed);
            break;
        }
        case OBJ_FUNCTION: {
            LoxFunction *function = (LoxFunction *)object;
            mark_object((Object *) function->name);
            for (int i = 0; i < function->chunk.constants.count; ++i) {
                mark_value(function->chunk.constants.values[i]);
            }
            break;
        }
        case OBJ_CLOSURE: {
            Closure *closure = (Closure *) object;
            mark_object((Object *) closure->function);
            for (int i = 0; i < closure->upvalue_count; ++i) {
                mark_object((Object *) closure->upvalues[i]);
            }
            break;
        }
    }
}

static void trace() {
    while (vm.gray_count > 0) {
        Object *object = vm.gray_stack[ -- vm.gray_count];
        blacken_object(object);
    }
}

static void sweep() {
    // 遍历列表，free所有没有被标记的对象
    if (vm.objects == NULL) {
        return;
    }

    Object *pre = vm.objects;
    Object *curr = vm.objects->next;

    while (curr != NULL) {
        if (!curr->is_marked) {
            Object *unreachable = curr;
            curr = curr->next;
            pre->next = curr;
            free_object(unreachable);
        } else {
            curr->is_marked = false;
            pre = curr;
            curr = curr->next;
        }
    }

    if (!vm.objects->is_marked) {
        Object *unreachable = vm.objects;
        vm.objects = vm.objects ->next;

        free_object(unreachable);
    } else {
        vm.objects->is_marked = false;
    }
}

static void gc() {
#ifdef DEBUG_LOG_GC
    printf("gc begin >>> size before gc: %zu\n", vm.allocated_size);
#endif

    mark_roots();
    trace();
    table_delete_unreachable(&vm.string_table);
    sweep();

#ifdef DEBUG_LOG_GC
    printf("<<< gc end. size after gc: %zu\n", vm.allocated_size);
    NEW_LINE();
#endif
}

/**
 * @param ptr 想要重新分类的指针。为 NULL 则分为新的空间。
 * @param old_size
 * @param byte_size 新的空间大小，以字节为单位
 * */
void *re_allocate(void *ptr, size_t old_size, size_t new_byte_size) {

    vm.allocated_size += new_byte_size - old_size;

    #ifdef DEBUG_STRESS_GC
    if (new_byte_size > 0 && ! compiling) {
        gc();
    }
    #endif

    if (new_byte_size == 0) {
        free(ptr);
        return NULL;
    }

    if (!compiling && vm.allocated_size > vm.next_gc) {
        gc();
        vm.next_gc = vm.allocated_size * GC_GROW_FACTOR;
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
#ifdef DEBUG_LOG_GC_FREE
    start_color(BLUE);
    printf("free %p, type: %d, ", object, object->type);
    printf("value: ");
    print_value_with_color(ref_value(object));
    NEW_LINE();
    end_color();
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
            FREE_ARRAY(UpValue*, closure->upvalues, closure->upvalue_count);
            re_allocate(object, sizeof(Closure), 0);
            break;
        }
        case OBJ_UPVALUE: {
            re_allocate(object, sizeof(UpValue), 0);
            break;
        }
        default:
            return;
    }
}
