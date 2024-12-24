//
// Created by Yue Xue  on 12/23/24.
//

#ifndef CLOX_NATIVE_H
#define CLOX_NATIVE_H

#include "object.h"

extern Value array_iter_class;
void additional_repl_init();

void init_vm_native();
#endif //CLOX_NATIVE_H
