//
// Created by Yue Xue  on 12/5/24.
//

#ifndef CLOX_IO_H
#define CLOX_IO_H

#include "object.h"


void write_function(FILE *file, LoxFunction *function);
LoxFunction *read_function(FILE *file);

#endif //CLOX_IO_H
