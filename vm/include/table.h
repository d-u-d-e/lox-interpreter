#pragma once

#include <object.h>
#include <value.h>

typedef struct {
  const obj_string_t *key;
  value_t value;
} entry_t;

typedef struct {
  int count;
  int capacity;
  entry_t *entries;
} table_t;

void init_table(table_t *table);
void free_table(table_t *table);
bool table_get(const table_t *table, const obj_string_t *key, value_t *value);
bool table_set(table_t *table, const obj_string_t *key, value_t value);
bool table_delete(table_t *table, const obj_string_t *key);
void table_add_all(const table_t *from, table_t *to);
const obj_string_t *table_find_string(const table_t *table, const char *chars, int length, uint32_t hash);