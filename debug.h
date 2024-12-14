//
// Created by Yue Xue  on 11/14/24.
//

#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H

#include "chunk.h"
//#include "table.h"

void disassemble_chunk(Chunk *chunk, const char *name);
int disassemble_instruction(Chunk *chunk, int offset);

//extern Map label_map;

#endif //CLOX_DEBUG_H
