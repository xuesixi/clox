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
void additional_repl_init();
void load_libraries();

void init_vm_native();
#endif //CLOX_NATIVE_H
