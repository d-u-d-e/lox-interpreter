#pragma once

#include <common.h>

// forward declaration
typedef struct obj obj_t;
typedef struct obj_string obj_string_t;

typedef enum { VAL_BOOL, VAL_NIL, VAL_NUMBER, VAL_OBJ } value_type_t;

typedef struct {
  value_type_t type;
  union
  {
    bool b;
    double n;
    obj_t *obj;
  } as;
} value_t;

#define IS_BOOL(v) ((v).type == VAL_BOOL)
#define IS_NIL(v) ((v).type == VAL_NIL)
#define IS_NUMBER(v) ((v).type == VAL_NUMBER)
#define IS_OBJ(v) ((v).type == VAL_OBJ)

#define AS_OBJ(v) ((v).as.obj)
#define AS_BOOL(v) ((v).as.b)
#define AS_NUMBER(v) ((v).as.n)

#define BOOL_VAL(v) ((value_t){VAL_BOOL, {.b = v}})
#define NIL_VAL ((value_t){VAL_NIL, {.n = 0}})
#define NUMBER_VAL(v) ((value_t){VAL_NUMBER, {.n = v}})
#define OBJ_VAL(o) ((value_t){VAL_OBJ, {.obj = (obj_t *)o}})

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