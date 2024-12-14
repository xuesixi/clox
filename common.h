#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include "stdbool.h"
#include "stdint.h"
#include "stddef.h"
#include "assert.h"
#include "stdio.h"
#include "setjmp.h"

//#define DEBUG_STRESS_GC
//#define DEBUG_LOG_GC
#define DEBUG_LOG_GC_FREE

#define FRAME_MAX 64
#define STACK_MAX (FRAME_MAX * UINT8_MAX)

extern bool SHOW_COMPILE_RESULT;
extern bool TRACE_EXECUTION;
extern int TRACE_SKIP;
extern bool SHOW_LABEL;
extern bool REPL;

extern jmp_buf consume_buf;

#define IMPLEMENTATION_ERROR(msg) \
    fprintf(stderr, "Implement error: %s\nOccurred in file: %s, line: %d\n", msg, __FILE__, __LINE__)

#define NEW_LINE() printf("\n")

#endif