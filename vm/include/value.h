#pragma once

#include <common.h>

// forward declaration
typedef struct obj obj_t;
typedef struct obj_string obj_string_t;

#ifdef NAN_BOXING
#include <string.h>
#define QNAN ((uint64_t)0x7FFC000000000000U) // quiet NaN
#define SIGN_BIT ((uint64_t)0x8000000000000000U)
#define TAG_NIL 1   // 01
#define TAG_FALSE 2 // 10
#define TAG_TRUE 3  // 11
typedef uint64_t value_t;

#define IS_BOOL(value) (((value) | 1) == TRUE_VAL)
#define IS_NIL(value) ((value) == NIL_VAL)
#define IS_NUMBER(value) (((value) & QNAN) != QNAN)
#define IS_OBJ(value) (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value) ((value) == TRUE_VAL)
#define AS_NUMBER(value) value_to_number(value)
#define AS_OBJ(value) ((obj_t*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#define BOOL_VAL(b) ((b) ? TRUE_VAL : FALSE_VAL)
#define TRUE_VAL ((value_t)(uint64_t)(QNAN | TAG_TRUE))
#define FALSE_VAL ((value_t)(uint64_t)(QNAN | TAG_FALSE))
#define NIL_VAL ((value_t)(uint64_t)(QNAN | TAG_NIL))
#define NUMBER_VAL(num) number_to_value(num)
#define OBJ_VAL(o) (value_t)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(o))

static inline value_t number_to_value(double num)
{
  value_t v;
  // This is usually optimized away by the compiler, being recognized as type punning
  memcpy(&v, &num, sizeof(double));
  return v;
}

static inline double value_to_number(value_t value)
{
  double num;
  memcpy(&num, &value, sizeof(value));
  return num;
}
#else

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
#endif

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