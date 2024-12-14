#include "table.h"

#include "memory.h"
#include "string.h"

/**
 * 判断一个entry是否为“空”：key为null，值为nil
 * @return 是否为空
 */
static inline bool empty_entry(Entry *entry) {
    return entry->key == NULL && is_nil(entry->value);
}

/**
 * del marker：如果key为NULL，值为bool，认为是一个del marker
 * @return 该entry是否是del marker
 */
static inline bool del_mark(Entry *entry) {
    return entry->key == NULL && is_bool(entry->value);
}

static inline bool need_resize(Table *table) {
    return table->count + 1 >= table->capacity * 0.75;
}

static void table_resize(Table *table) {
    int old_capacity = table->capacity;
    int new_capacity = old_capacity < 11 ? 11 : table->capacity * 2 + 1;

    Entry *old_backing = table->backing;
    Entry *new_backing = ALLOCATE(Entry, new_capacity);

    for (int i = 0; i < new_capacity; ++i) {
        new_backing[i].key = NULL;
        new_backing[i].value.type = VAL_NIL;
    }

    table->capacity = new_capacity;
    table->backing =new_backing;
    table->count = 0;

    for (int i = 0; i < old_capacity; ++i) {
        Entry *entry = old_backing + i;
        if (entry->key != NULL) {
            table_set(table, entry->key, entry->value);
        }
    }

    FREE_ARRAY(Entry, old_backing, old_capacity);
}

/**
 * @return 如果key存在，返回对应的entry。如果不存在，返回第一个空位
 */
static Entry *find_entry(Table *table, String *key) {
    int index = key->hash % table->capacity;
    for (int i = 0; i < table->capacity; ++i) {
        int curr = (index + i) % table->capacity;
        Entry *entry = table->backing + curr;
        if (empty_entry(entry)) {
            return entry;
        }
        if (entry->key == key) {
            return entry;
        }
    }
    IMPLEMENTATION_ERROR("find_entry() does not find empty spot and returns NULL");
    return NULL;
}

inline bool table_has(Table *table, String *key) {
    if (table->count == 0) {
        return false;
    }
    return !empty_entry(find_entry(table, key));
}

/**
 *
 * @param table
 * @param key
 * @param value 如果存在该key，则将对应的值储存在这个参数值
 * @return 是否存在该key
 */
bool table_get(Table *table, String *key, Value *value) {
    if (table->count == 0) {
        return false;
    }
    Entry *entry = find_entry(table, key);
    if (empty_entry(entry)) {
        return false;
    } else {
        *value = entry->value;
        return true;
    }
}

/**
 * 设置table的一个键值对。如果key原本就已存在，返回true。如果原本不存在，返回false。
 * 该函数可能导致gc
 */
bool table_set(Table *table, String *key, Value value) {

    if (need_resize(table)) {
        table_resize(table);
    }
    Entry *mark = NULL;
    int index = key->hash % table->capacity;
    for (int i = 0; i < table->capacity; ++i) {
        int curr = (index + i) % table->capacity;
        Entry *entry = table->backing + curr;
        if (empty_entry(entry)) {
            if (mark == NULL) {
                entry->key = key;
                entry->value = value;
                table->count++;
            } else {
                mark->key = key;
                mark->value = value;
            }
            return false;
        } else if(entry->key == key) {
            entry->value = value;
            return true;
        } else if (mark == NULL && del_mark(entry)) {
            mark = entry;
        }
    }
    IMPLEMENTATION_ERROR("table_set() does not find empty spot");
    return false;
}

Value table_delete(Table *table, String *key) {
    if (table->count == 0) {
        return nil_value();
    }
    Entry *entry = find_entry(table, key);
    entry->key = NULL;
    Value result = entry->value;
    entry->value = bool_value(true);
    return result;
}

void table_add_all(Table *from, Table *to) {
    int capacity = from->capacity;
    for (int i = 0; i < capacity; ++i) {
        Entry *entry = from->backing + i;
        if (entry->key != NULL) {
            table_set(to,entry->key, entry->value);
        }
    }
}

/**
 * 标记这个table中的所有entry
 */
void table_mark(Table *table) {
    if (table == NULL) {
        return;
    }
    for (int i = 0; i < table->capacity; ++i) {
        Entry *entry = table->backing + i;
        mark_object((Object *) entry->key);
        mark_value(entry->value);
    }
}

/**
 * 该函数仅仅从table中删除对应的元素，并不清除内存。free_object会在sweep函数中执行。
 */
void table_delete_unreachable(Table *table) {
    for (int i = 0; i < table->capacity; ++i) {
        Entry *entry = table->backing + i;
        if (entry != NULL) {
            Object *object = (Object*) entry->key;
            if (entry->key != NULL && !object->is_marked) {
                entry->key = NULL;
                entry->value = bool_value(true);
            }
        }
    }
}

String *table_find_string(Table *table, const char *name, int length, uint32_t hash) {
    if (table->count == 0) {
        return NULL;
    }
    int index = hash % table->capacity;
    for (int i = 0; i < table->capacity; ++i) {
        int curr = (i + index) % table->capacity;
        Entry *entry = table->backing + curr;
        if (empty_entry(entry)) {
            return NULL;
        }
        if (entry->key == NULL || entry->key->hash != hash || entry->key->length != length) {
            continue;
        }
        if (memcmp(entry->key->chars, name, length) == 0) {
            return entry->key;
        }
    }
    return NULL;
}

void init_table(Table *table) {
    table->backing = NULL;
    table->capacity = 0;
    table->count = 0;
}

void free_table(Table *table) {
    FREE_ARRAY(Entry, table->backing, table->capacity);
    init_table(table);
}

void init_map(Map *map, HashFunction hash, EqualityFunction equal) {
    map->backing = NULL;
    map->capacity = 0;
    map->count = 0;
    map->hash = hash;
    map->equal = equal;
}

void free_map(Map *map) {
    FREE_ARRAY(MapEntry, map->backing, map->capacity);
    map->backing = NULL;
    map->capacity = 0;
    map->count = 0;
}

static inline bool map_empty_entry(MapEntry *entry) {
    return entry->key == NULL && entry->value == NULL;
}

static inline bool map_del_mark(MapEntry *entry) {
    // null key && non-null value -> del
    // not del -> non-null key || null value
    return entry->key == NULL && entry->value != NULL;
}

static inline bool map_need_resize(Map *map) {
    return map->count + 1 >= map->capacity * 0.75;
}

static void map_resize(Map *map) {
    int old_capacity = map->capacity;
    int new_capacity = old_capacity < 11 ? 11 : old_capacity * 2 + 1;
    MapEntry *old_backing = map->backing;
    MapEntry *new_backing = ALLOCATE(MapEntry, new_capacity);
    for (int i = 0; i < new_capacity; i++) {
        new_backing[i].key = NULL;
        new_backing[i].value = NULL;
    }

    map->capacity = new_capacity;
    map->backing = new_backing;
    map->count = 0;
    for (int i = 0; i < old_capacity; i ++) {
        MapEntry *entry = old_backing + i;
        if (entry->key != NULL) {
            map_set(map, entry->key, entry->value);
        }
    }
    FREE_ARRAY(MapEntry, old_backing, old_capacity);
}

/**
 * 如果不存在，则返回空entry（key，value均为null）。否则返回对应的entry.
 * 
 * */
static MapEntry *map_find_entry(Map *map, void *key) {
    int index = map->hash(key);
    for (int i = 0; i < map->capacity; i ++) {
        int curr = (index + i) % (map->capacity);
        MapEntry *entry = map->backing + curr;
        if (map_empty_entry(entry)) {
            return entry;
        }
        if (entry->key != NULL && map->equal(entry->key, key)) {
            return entry;
        }
    }
    IMPLEMENTATION_ERROR("map_find_entry() does not find empty entry spot and returns NULL");
    return NULL;
}

/**
 * @return NULL if not found
 * */
void *map_get(Map *map, void *key) {
    if (map->count == 0) {
        return NULL;
    }
    MapEntry *entry = map_find_entry(map, key);
    if (map_empty_entry(entry)) {
        return NULL;
    } else {
        return entry->value;
    }
}

/**
 *
 * @param map
 * @param key
 * @param value
 * @return true if modifying existing pair; false if adding new entry
 */
bool map_set(Map *map, void *key, void *value) {
    if (map_need_resize(map)) {
        map_resize(map);
    }

    MapEntry *del = NULL;
    int index = map->hash(key);
    for (int i = 0; i < map->capacity; i ++ ) {
        int curr = (index + i) % map->capacity;
        MapEntry *entry = map->backing + curr;
        if (map_empty_entry(entry)) {
            if (del == NULL) {
                entry->key = key;
                entry->value = value;
                map->count ++;
            } else {
                del->key = key;
                del->value = value;
            }
            return false;
        } else if (map_del_mark(entry)){
            del = entry;
        } else if (map->equal(entry->key, key)) {
            entry->value = value;
            return true;
        }
    }
    IMPLEMENTATION_ERROR("map_set() does not find empty spot");
    return false;
}

void *map_delete(Map *map, void *key) {
    if (map->count == 0) {
        return NULL;
    }
    MapEntry *entry = map_find_entry(map, key);
    if (map_empty_entry(entry)) {
        return NULL;
    } else {
        entry->key = NULL;
        void *result = entry->value;
        entry->value = (void *)10086; // this is considered DEL mark
        return result;
    }
}

int int_hash(void *p) {
    int num = (int) p;
    return num * 2654435761 % (INT32_MAX);
}

bool int_equal(void *a, void *b) {
    return a == b;
//    return (int) a == (int) b;
}
