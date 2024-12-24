//
// Created by Yue Xue  on 12/5/24.
//

#include "io.h"
#include "stdlib.h"

static void write_string(FILE *file, String *string);
static void write_value(FILE *file, Value *value);
static void write_valueArray(FILE *file, ValueArray *valueArray);
static void write_chunk(FILE *file, Chunk *chunk);

static String *read_string(FILE *file);
static Value read_value(FILE *file);
static ValueArray read_valueArray(FILE *file);
static Chunk read_chunk(FILE *file);

static void write_value(FILE *file, Value *value) {

    fwrite(& value->type, sizeof(ValueType), 1, file);
    fwrite(& value->as, sizeof(value->as), 1, file);
    if (value->type == VAL_REF) {
        Object *ref = as_ref(*value);
        fwrite(& ref->type, sizeof(int ), 1, file);
        switch (ref->type) {
            case OBJ_STRING:
                write_string(file, (String *) ref);
                break;
            case OBJ_FUNCTION:
                write_function(file, (LoxFunction *) ref);
                break;
            default:
                IMPLEMENTATION_ERROR("only expect string or function");
                break;
        }
    }
}

static Value read_value(FILE *file) {
    Value value;
    fread(& value.type, sizeof(ValueType), 1, file);
    fread(&value.as, sizeof (value.as), 1, file);
    if (is_ref(value)) {
        int type;
        fread(&type, sizeof(int ), 1, file);
        switch (type) {
            case OBJ_STRING:
                value.as.reference = (Object *) read_string(file);
                break;
            case OBJ_FUNCTION:
                value.as.reference = (Object *) read_function(file);
                break;
            default:
                IMPLEMENTATION_ERROR("bad");
                return nil_value();
        }
    }
    return value;
}

static void write_chunk(FILE *file, Chunk *chunk) {
    fwrite(&chunk->count, sizeof(int ), 1, file);
    fwrite(chunk->code, sizeof(uint8_t), chunk->count, file);
    fwrite(chunk->lines, sizeof(int ), chunk->count, file);
    write_valueArray(file, &chunk->constants);
}

static Chunk read_chunk(FILE *file) {
    Chunk chunk;
    fread(&chunk.count, sizeof(int ), 1, file);
    chunk.code = malloc(sizeof(uint8_t) * chunk.count);
    chunk.lines = malloc(sizeof(int ) * chunk.count);
    chunk.capacity = chunk.count;
    fread(chunk.code, sizeof(uint8_t), chunk.count, file);
    fread(chunk.lines, sizeof(int ), chunk.count, file);
    chunk.constants = read_valueArray(file);
    return chunk;
}

static void write_valueArray(FILE *file, ValueArray *valueArray) {
    fwrite(& valueArray->count, sizeof(int ), 1, file);
    for (int i = 0; i < valueArray->count; ++i) {
        write_value(file, valueArray->values + i);
    }
}

static void write_string(FILE *file, String *string) {
    if (string == NULL) {
        int len = -1;
        fwrite(&len, sizeof(int ), 1, file);
    } else {
        fwrite(&string->length, sizeof(int ), 1, file);
        fwrite(string->chars, 1, string->length + 1, file);
    }
}

static String *read_string(FILE *file) {
    int length;
    char *text;
    fread(&length, sizeof(int ), 1, file);
    if (length == -1) {
        return NULL;
    }
    text = malloc(length + 1);
    fread(text, 1, length + 1, file);
    return string_allocate(text, length);
}

void write_function(FILE *file, LoxFunction *function) {
    fwrite(&function->type, sizeof(FunctionType), 1, file);
    fwrite(&function->arity, sizeof(int ), 1, file);
    write_chunk(file, & function->chunk);
    write_string(file, function->name);
    fwrite(&function->upvalue_count, sizeof(int ), 1, file);
}

LoxFunction *read_function(FILE *file) {
    LoxFunction *function = new_function(TYPE_FUNCTION);
    fread(&function->type, sizeof(FunctionType), 1, file);
    fread(&function->arity, sizeof(int ), 1, file);
    function->chunk = read_chunk(file);
    function->name = read_string(file);
    fread(&function->upvalue_count, sizeof(int ), 1, file);
    return function;
}

static ValueArray read_valueArray(FILE *file) {
    ValueArray array;
    array.count = 0;
    fread(&array.count, sizeof(int ), 1, file);
    array.capacity = array.count;
    array.values = malloc(sizeof(Value) * array.count);
    for (int i = 0; i < array.count; ++i) {
        array.values[i] = read_value(file);
    }
    return array;
}


