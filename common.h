#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include "stdbool.h"
#include "stdint.h"
#include "stddef.h"
#include "assert.h"
#include "stdio.h"

#define DEBUG_TRACE_EXECUTION

#define IMPLEMENTATION_ERROR(msg) \
    fprintf(stderr, "Implement error: %s\nOccurred in file: %s, line: %d\n", msg, __FILE__, __LINE__)

#endif