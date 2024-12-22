#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

#include "value.h"

//#define ENTRY_CONST 0x01
//#define ENTRY_PUBLIC 0x02

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
Entry *table_find_entry(Table *table, String *key, bool public_only, bool mutable_only);
bool table_get(Table *table, String *key, Value *value);
bool table_conditional_get(Table *table, String *key, Value *value, bool public_only, bool mutable_only);
bool table_has(Table *table, String *key);
bool table_set(Table *table, String *key, Value value);
char table_set_existent(Table *table, String *key, Value value, bool public_only);
bool table_add_new(Table *table, String *key, Value value, bool is_public, bool is_const);
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

//bool entry_is_public(Entry *entry);
//bool entry_is_const(Entry *entry);
//void entry_set_public(Entry *entry);
//void entry_set_const(Entry *entry);

#endif