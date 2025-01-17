#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#if defined(__linux__)
#define _GNU_SOURCE
#endif

#include "stdbool.h"
#include "stdint.h"
#include "stddef.h"
#include "assert.h"
#include "stdio.h"
#include "setjmp.h"

//#define DEBUG_STRESS_GC
//#define DEBUG_LOG_MARK_BLACKEN
//#define DEBUG_LOG_GC_ALLOCATE
//#define DEBUG_LOG_GC_FREE
//#define DEBUG_LOG_GC_SUMMARY

#define COLOR_RUN_FILE_RESULT
#define IMPLEMENTATION_CHECK
//#define COUNT_INSTRUCTIONS_RUN

#define FRAME_MAX 64
#define STACK_MAX (FRAME_MAX * UINT8_MAX)
#define STR(x) #x
#define XSTR(x) STR(x)

extern bool SHOW_COMPILE_RESULT;
extern bool TRACE_EXECUTION;
extern bool COMPILE_ONLY;
extern int TRACE_SKIP;
extern bool REPL;
extern bool LOAD_LIB;
extern bool preload_finished;

char *read_file(const char *path);
char *resolve_path(const char *path);
char *get_filename(char *path);

extern jmp_buf consume_buf;

#define IMPLEMENTATION_ERROR(msg) \
    fprintf(stderr, "Implement error: %s\nOccurred in file: %s, line: %d\n", msg, __FILE__, __LINE__)

#define NEW_LINE() printf("\n")

#endif