#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

#include "object.h"

typedef struct Entry {
    String *key;
    Value value;
} Entry;

typedef struct Table{
    int count;
    int capacity;
    Entry *backing;
} Table;

void init_table(Table *table);
bool empty_entry(Entry *entry);
Entry *find_entry(Table *table, String *key);
void free_table(Table *table);
Value table_get(Table *table, String *key);
bool table_has(Table *table, String *key);
void table_set(Table *table, String *key, Value value);
Value table_delete(Table *table, String *key);
void table_add_all(Table *from, Table *to);
String *table_find_string(Table *table, const char *name, int length, uint32_t hash);

#endif