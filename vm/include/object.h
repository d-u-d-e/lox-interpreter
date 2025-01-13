#pragma once

#include <common.h>
#include <value.h>

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STRING(value) is_obj_type(value, OBJ_TYPE_STRING)

#define AS_STRING(value) ((obj_string_t *)AS_OBJ(value))
#define AS_CSTRING(value) (((obj_string_t *)AS_OBJ(value))->chars)

typedef enum {
  OBJ_TYPE_STRING,
} obj_type_t;

struct obj {
  obj_type_t type;
  struct obj *next;
};

struct obj_string {
  struct obj base;
  int length;
  char *chars;
};

static inline bool is_obj_type(value_t value, obj_type_t type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

obj_string_t *take_string(char *chars, int length);
obj_string_t *copy_string(const char *chars, int length);
void print_object(value_t value);