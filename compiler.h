//
// Created by Yue Xue  on 11/17/24.
//

#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

// #include "chunk.h"
#include "object.h"

LoxFunction *compile(const char* source);

void mark_compiler_roots();

__attribute__((unused)) void show_tokens(const char *source);

#endif //CLOX_COMPILER_H
