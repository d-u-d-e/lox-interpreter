#pragma once

#include <chunk.h>
#include <common.h>
#include <value.h>

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STRING(value) is_obj_type(value, OBJ_TYPE_STRING)
#define IS_FUNCTION(value) is_obj_type(value, OBJ_FUNCTION)

#define AS_STRING(value) ((const obj_string_t *)AS_OBJ(value))
#define AS_CSTRING(value) (((const obj_string_t *)AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((obj_function_t *)AS_OBJ(value))

typedef enum {
  OBJ_FUNCTION,
  OBJ_TYPE_STRING,
} obj_type_t;

struct obj {
  obj_type_t type;
  struct obj *next;
};

typedef struct {
  struct obj base;
  int arity;
  chunk_t chunk;
  obj_string_t *name;
} obj_function_t;

struct obj_string {
  struct obj base;
  int length;
  uint32_t hash;
  char chars[]; // flexible array
};

static inline bool is_obj_type(value_t value, obj_type_t type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

obj_function_t *new_function();
obj_string_t *allocate_string(int length);
uint32_t hash_string(const char *key, int length);
const obj_string_t *copy_string(const char *chars, int length);
void print_object(value_t value);