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

#define MODULO(a, b) ((a) & ((b) - 1))


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
    int new_capacity = old_capacity < 8 ? 8 : table->capacity * 2;
#ifdef DEBUG_LOG_GC_ALLOCATE
    printf("table resize. old capacity: %d, new: %d\n", old_capacity, new_capacity);
#endif

    Entry *old_backing = table->backing;
    Entry *new_backing = ALLOCATE(Entry, new_capacity);

    for (int i = 0; i < new_capacity; ++i) {
        new_backing[i].key = NULL;
        new_backing[i].value.type = VAL_NIL;
        new_backing[i].is_public = false;
        new_backing[i].is_const = false;
    }

    table->capacity = new_capacity;
    table->backing =new_backing;
    table->count = 0;

    for (int i = 0; i < old_capacity; ++i) {
        Entry *entry = old_backing + i;
        if (entry->key != NULL) {
            table_add_new(table, entry->key, entry->value, entry->is_public, entry->is_const);
        }
    }

    FREE_ARRAY(Entry, old_backing, old_capacity);
}

/**
 * @return 如果key存在，返回对应的entry。如果不存在，返回第一个空位.
 * 如果存在，但不是满足条件，返回NULL
 */
static Entry *find_entry(Table *table, String *key, bool public_only, bool mutable_only) {
    int index = MODULO(key->hash, table->capacity);
    for (int i = 0; i < table->capacity; ++i) {
        int curr = MODULO(index + i, table->capacity);
        Entry *entry = table->backing + curr;
        if (empty_entry(entry)) {
            return entry;
        }
        if (entry->key == key) {
            if (public_only && !entry->is_public) {
                return NULL;
            }
            if (mutable_only && entry->is_const) {
                return NULL;
            }
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
    return !empty_entry(find_entry(table, key, false, false));
}

/**
 *
 * @param table
 * @param key
 * @param value 如果存在该key，则将对应的值储存在这个参数值
 * @return 是否存在该key
 */
inline bool table_get(Table *table, String *key, Value *value) {
    return table_conditional_get(table, key, value, false, false);
}

bool table_conditional_get(Table *table, String *key, Value *value, bool public_only, bool mutable_only) {
    if (table->count == 0) {
        return false;
    }
    Entry *entry = find_entry(table, key, public_only, mutable_only);
    if (entry == NULL || empty_entry(entry)) {
        return false;
    } else {
        *value = entry->value;
        return true;
    }
}

/**
 * 设置table的一个键值对。如果该entry为const，则不进行操作，返回false。
 * 否则进行操作，并且返回true。
 * 该函数可能导致gc
 */
bool table_set(Table *table, String *key, Value value) {

    if (need_resize(table)) {
        table_resize(table);
    }
    Entry *mark = NULL;
    int index = MODULO(key->hash, table->capacity);
    for (int i = 0; i < table->capacity; ++i) {
        int curr = MODULO(index + i, table->capacity);
        Entry *entry = table->backing + curr;
        if (empty_entry(entry)) {
            if (mark == NULL) {
                entry->key = key;
                entry->value = value;
                entry->is_const = false;
                entry->is_public = false;
                table->count++;
            } else {
                mark->key = key;
                mark->value = value;
                entry->is_const = false;
                entry->is_public = false;
            }
            return true;
        } else if(entry->key == key) {
            if (entry->is_const) {
                return false;
            } else {
                entry->value = value;
                return true;
            }
        } else if (mark == NULL && del_mark(entry)) {
            mark = entry;
        }
    }
    IMPLEMENTATION_ERROR("table_set() does not find empty spot");
    return false;
}

/**
 * 设置table的一个键值对。如果key原本就已存在（正常情况），返回0;
 * 如果原本不存在，不进行操作，返回1;
 * 如果该entry为const，不进行操作，返回2;
 * 该函数可能导致gc
 */
char table_set_existent(Table *table, String *key, Value value) {
    if (need_resize(table)) {
        table_resize(table);
    }
    int index = MODULO(key->hash, table->capacity);
    for (int i = 0; i < table->capacity; ++i) {
        int curr = MODULO(index + i, table->capacity);
        Entry *entry = table->backing + curr;
        if (empty_entry(entry)) {
            return 1;
        } else if(entry->key == key) {
            if (entry->is_const) {
                return 2;
            } else {
                entry->value = value;
                return 0;
            }
        }
    }
    return false;
}

/**
 * 如果该key不存在，则添加一个新的entry，并设定它的is_public, is_const属性，然后返回true.
 * 如果该key已存在，什么都不做，返回false.
 * 该函数可能导致gc
 */
bool table_add_new(Table *table, String *key, Value value, bool is_public, bool is_const) {

    if (need_resize(table)) {
        table_resize(table);
    }
    Entry *mark = NULL;
    int index = MODULO(key->hash, table->capacity);
    for (int i = 0; i < table->capacity; ++i) {
        int curr = MODULO(index + i, table->capacity);
        Entry *entry = table->backing + curr;
        if (empty_entry(entry)) {
            if (mark == NULL) {
                entry->key = key;
                entry->value = value;
                entry->is_public = is_public;
                entry->is_const = is_const;
                table->count++;
            } else {
                mark->key = key;
                mark->value = value;
                mark->is_public = is_public;
                mark->is_const = is_const;
            }
            return true;
        } else if(entry->key == key) {
            return false;
        } else if (mark == NULL && del_mark(entry)) {
            mark = entry;
        }
    }
    IMPLEMENTATION_ERROR("table_add() does not find empty spot");
    return false;
}

Value table_delete(Table *table, String *key) {
    if (table->count == 0) {
        return nil_value();
    }
    Entry *entry = find_entry(table, key, false, false);
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
            table_add_new(to,entry->key, entry->value, entry->is_public, entry->is_public);
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
    int index = MODULO(hash, table->capacity);
    for (int i = 0; i < table->capacity; ++i) {
        int curr = MODULO(index + i, table->capacity);
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
}
