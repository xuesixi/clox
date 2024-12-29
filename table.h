#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

#include "value.h"

#define MODULO(a, b) ((a) & ((b) - 1))

typedef struct String String;

typedef struct Entry {
    String *key;
    Value value;
    bool is_const;
    bool is_public;
} Entry;

typedef struct Table {
    int count;
    int capacity;
    Entry *backing;
} Table;

void init_table(Table *table);
void free_table(Table *table);
bool table_get(Table *table, String *key, Value *value);
Entry *table_find_entry(Table *table, String *key, bool public_only, bool mutable_only);
bool table_conditional_get(Table *table, String *key, Value *value, bool public_only, bool mutable_only);
bool table_has(Table *table, String *key);
bool table_set(Table *table, String *key, Value value);
char table_set_existent(Table *table, String *key, Value value, bool public_only);
bool table_add_new(Table *table, String *key, Value value, bool is_public, bool is_const);
Value table_delete(Table *table, String *key);
void table_add_all(Table *from, Table *to, bool public_only);
String *table_find_string(Table *table, const char *name, int length, uint32_t hash);
void table_mark(Table *table);
void table_delete_unreachable(Table *table);

#endif