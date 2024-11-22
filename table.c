#include "table.h"

#include "memory.h"
#include "string.h"

/**
 * 判断一个entry是否为“空”：key为null，值为nil
 * @return 是否为空
 */
inline bool empty_entry(Entry *entry) {
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
Entry *find_entry(Table *table, String *key) {
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
    return !empty_entry(find_entry(table, key));
}

/**
 * @return value of nil when the key is not found
 */
inline Value table_get(Table *table, String *key) {
    return find_entry(table, key)->value;
}

void table_set(Table *table, String *key, Value value) {

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
            return;
        } else if(entry->key == key) {
            entry->value = value;
            return;
        } else if (mark == NULL && del_mark(entry)) {
            mark = entry;
        }
    }
    IMPLEMENTATION_ERROR("table_set() does not find empty spot");
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

String *table_find_string(Table *table, const char *name, int length, uint32_t hash) {
    int index = hash % table->capacity;
    for (int i = 0; i < table->capacity; ++i) {
        Entry *entry = table->backing + i + index;
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