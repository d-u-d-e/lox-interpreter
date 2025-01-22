#include <memory.h>
#include <string.h>
#include <table.h>

#define TABLE_MAX_LOAD_FACTOR 0.75

void init_table(table_t *table)
{
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void free_table(table_t *table)
{
  FREE_ARRAY(entry_t, table->entries, table->capacity);
  init_table(table);
}

static entry_t *find_entry(entry_t *entries, int capacity, const obj_string_t *key)
{
  uint32_t index = key->hash % capacity;
  entry_t *tombestone = NULL;

  for(;;) {
    entry_t *entry = &entries[index];

    if(entry->key == NULL) {
      if(IS_NIL(entry->value)) {
        return tombestone != NULL ? tombestone : entry;
      }
      else {
        // tombestone found
        if(tombestone == NULL) {
          tombestone = entry;
        }
      }
    }
    else if(entry->key == key) {
      // strings are interned, so this works!
      return entry;
    }
    index = (index + 1) % capacity;
  }
}

static void adjust_capacity(table_t *table, int capacity)
{
  // entries end up in new buckets when we grow the table
  // we can't just grow the table using realloc
  entry_t *entries = ALLOCATE(entry_t, capacity);
  for(int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  table->count = 0;
  for(int i = 0; i < table->capacity; i++) {
    entry_t *entry = &table->entries[i];
    if(entry->key == NULL) {
      continue;
    }
    entry_t *dest = find_entry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  FREE_ARRAY(entry_t, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

bool table_set(table_t *table, const obj_string_t *key, value_t value)
{
  if(table->count + 1 > table->capacity * TABLE_MAX_LOAD_FACTOR) {
    int cap = GROW_CAPACITY(table->capacity);
    adjust_capacity(table, cap);
  }

  entry_t *entry = find_entry(table->entries, table->capacity, key);
  bool is_new_key = entry->key == NULL;
  if(is_new_key && IS_NIL(entry->value)) {
    // tombestones contribute to load factor
    table->count++;
  }

  entry->key = key;
  entry->value = value;
  return is_new_key;
}

void table_add_all(const table_t *from, table_t *to)
{
  for(int i = 0; i < from->capacity; i++) {
    const entry_t *entry = &from->entries[i];
    if(entry->key != NULL) {
      table_set(to, entry->key, entry->value);
    }
  }
}

bool table_get(const table_t *table, const obj_string_t *key, value_t *value)
{
  // ensure to not access the entries array if null
  if(table->count == 0) {
    return false;
  }

  const entry_t *entry = find_entry(table->entries, table->capacity, key);
  if(entry->key == NULL) {
    return false;
  }
  *value = entry->value;
  return true;
}

bool table_delete(table_t *table, const obj_string_t *key)
{
  // ensure to not access the entries array if null
  if(table->count == 0) {
    return false;
  }

  entry_t *entry = find_entry(table->entries, table->capacity, key);
  if(entry->key == NULL) {
    return false;
  }

  // place a tombestone -> treated as full buckets, so count does not change
  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}

const obj_string_t *table_find_string(const table_t *table, const char *chars, int length,
                                      uint32_t hash)
{
  if(table->count == 0) {
    return NULL;
  }

  uint32_t index = hash % table->capacity;
  for(;;) {
    entry_t *entry = &table->entries[index];
    if(entry->key == NULL) {
      // stop if we find a non-tombestone
      if(IS_NIL(entry->value)) {
        return NULL;
      }
    }
    else if(entry->key->hash == hash && entry->key->length == length
            && memcmp(entry->key->chars, chars, length) == 0) {
      // found it
      return entry->key;
    }
    index = (index + 1) % table->capacity;
  }
  return NULL;
}

void mark_table(table_t *table)
{
  for(int i = 0; i < table->capacity; i++) {
    entry_t *entry = &table->entries[i];
    mark_object((obj_t *)entry->key);
    mark_value(entry->value);
  }
}