//
// Created by Yue Xue  on 12/23/24.
//

#ifndef CLOX_NATIVE_H
#define CLOX_NATIVE_H

#define RUNTIME_ERROR_VA_BUF_LEN 256

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

extern Class *Error;
extern Class *TypeError;
extern Class *IndexError;
extern Class *ArgError;
extern Class *NameError;
extern Class *PropertyError;
extern Class *ValueError;
extern Class *FatalError;
extern Class *CompileError;
extern Class *IOError;

extern String *INIT;
extern String *LENGTH;
extern String *HAS_NEXT;
extern String *NEXT;
extern String *ITERATOR;
extern String *EQUAL;
extern String *HASH;
extern String *MESSAGE;
extern String *POSITION;

typedef enum ErrorType {
    Error_Error,
    Error_FatalError,
    Error_TypeError,
    Error_ValueError,
    Error_NameError,
    Error_PropertyError,
    Error_ArgError,
    Error_IndexError,
    Error_CompileError,
    Error_IOError,
} ErrorType;

void additional_repl_init();
void load_libraries();
void init_static_strings();
bool is_subclass(Class *one, Class *two);
bool multi_value_of(int count, Value *values);
Value native_backtrace(int count, Value *value);
void new_error(ErrorType type, const char *message);
void throw_new_runtime_error(ErrorType type, const char *format, ...);
void throw_user_level_runtime_error(ErrorType type, const char *format, ...);

void init_vm_native();
#endif //CLOX_NATIVE_H
