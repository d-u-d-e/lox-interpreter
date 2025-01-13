#pragma once

#include <common.h>

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
} value_type_t;

typedef struct {
  value_type_t type;
  union
  {
    bool b;
    double n;
  } as;
} value_t;

#define IS_BOOL(v) ((v).type == VAL_BOOL)
#define IS_NIL(v) ((v).type == VAL_NIL)
#define IS_NUMBER(v) ((v).type == VAL_NUMBER)

#define AS_BOOL(v) ((v).as.b)
#define AS_NUMBER(v) ((v).as.n)

#define BOOL_VAL(v) ((value_t){VAL_BOOL, {.b = v}})
#define NIL_VAL ((value_t){VAL_NIL, {.n = 0}})
#define NUMBER_VAL(v) ((value_t){VAL_NUMBER, {.n = v}})

typedef struct {
  int capacity;
  int count;
  value_t *values;
} value_array_t;

bool values_equal(value_t left, value_t right);
void init_value_array(value_array_t *array);
void free_value_array(value_array_t *array);
void write_value_array(value_array_t *array, value_t value);
void print_value(value_t value);