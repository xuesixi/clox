//
// Created by Yue Xue  on 12/23/24.
//

#ifndef CLOX_NATIVE_H
#define CLOX_NATIVE_H

#include "object.h"

extern Class *array_class;
extern Class *string_class;
extern Class *float_class;
extern Class *int_class;
extern Class *bool_class;
extern Class *native_class;
extern Class *class_class;
extern Class *function_class;
extern Class *closure_class;
extern Class *map_class;
extern Class *method_class;
extern Class *nil_class;
extern Class *module_class;
extern Class *native_object_class;

extern String *ARRAY_CLASS;
extern String *STRING_CLASS;
extern String *INT_CLASS;
extern String *FLOAT_CLASS;
extern String *BOOL_CLASS;
extern String *NATIVE_CLASS;
extern String *FUNCTION_CLASS;
extern String *MAP_CLASS;
extern String *CLOSURE_CLASS;
extern String *METHOD_CLASS;
extern String *MODULE_CLASS;
extern String *CLASS_CLASS;
extern String *NIL_CLASS;
extern String *NATIVE_OBJECT_CLASS;

extern String *INIT;
extern String *LENGTH;
extern String *HAS_NEXT;
extern String *NEXT;
extern String *ITERATOR;
extern String *EQUAL;
extern String *HASH;

void additional_repl_init();
void load_libraries();
void init_static_strings();

void init_vm_native();
#endif //CLOX_NATIVE_H
