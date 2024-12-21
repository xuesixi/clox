#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

#include "value.h"

typedef struct String String;

typedef struct Entry {
    String *key;
    Value value;
} Entry;

typedef struct Table {
    int count;
    int capacity;
    Entry *backing;
} Table;

/**
 * 值得注意的是，中无论是key还是value都是指针，其所有权在外部，不在Map中。
 */
typedef struct MapEntry {
    void *key;
    void *value;
} MapEntry;

typedef int (*HashFunction)(void *key); ;
typedef bool (*EqualityFunction)(void *a, void *b);


typedef struct Map {
    int count;
    int capacity;
    HashFunction hash;
    EqualityFunction equal;
    MapEntry *backing;
} Map;

void init_table(Table *table);
void free_table(Table *table);
bool table_get(Table *table, String *key, Value *value);
bool table_has(Table *table, String *key);
bool table_set(Table *table, String *key, Value value);
Value table_delete(Table *table, String *key);
void table_add_all(Table *from, Table *to);
String *table_find_string(Table *table, const char *name, int length, uint32_t hash);
void table_mark(Table *table);
void table_delete_unreachable(Table *table);

void init_map(Map *map, HashFunction hash, EqualityFunction equal);
void free_map(Map *map);
void *map_get(Map *map, void *key);
bool map_set(Map *map, void *key, void *value);
void *map_delete(Map *map, void *key);
int int_hash(void *p);
bool int_equal(void *a, void *b);

#endif