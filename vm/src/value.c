#include <memory.h>
#include <object.h>
#include <stdio.h>
#include <string.h>
#include <value.h>

void init_value_array(value_array_t *array)
{
  array->capacity = 0;
  array->count = 0;
  array->values = NULL;
}

void free_value_array(value_array_t *array)
{
  FREE_ARRAY(value_t, array->values, array->capacity);
  init_value_array(array);
}

void write_value_array(value_array_t *array, value_t value)
{
  if(array->capacity < array->count + 1) {
    // grow array
    int old_cap = array->capacity;
    array->capacity = GROW_CAPACITY(old_cap);
    array->values = GROW_ARRAY(value_t, array->values, old_cap, array->capacity);
  }
  array->values[array->count] = value;
  array->count++;
}

void print_value(value_t value)
{
#ifdef NAN_BOXING
  if(IS_BOOL(value)) {
    printf(AS_BOOL(value) ? "true" : "false");
  }
  else if(IS_NIL(value)) {
    printf("nil");
  }
  else if(IS_NUMBER(value)) {
    printf("%g", AS_NUMBER(value));
  }

  else if(IS_OBJ(value)) {
    print_object(value);
  }
#else
  switch(value.type) {
  case VAL_BOOL: {
    printf("%s", AS_BOOL(value) ? "true" : "false");
    break;
  }
  case VAL_NIL: {
    printf("nil");
    break;
  }
  case VAL_NUMBER: {
    printf("%g", AS_NUMBER(value));
    break;
  }

  case VAL_OBJ: {
    print_object(value);
    break;
  }
  }
#endif
}

bool values_equal(value_t left, value_t right)
{
#ifdef NAN_BOXING
  if(IS_NUMBER(left) && IS_NUMBER(right)) {
    // handle NaN == Nan being false as per IEEE 754
    return AS_NUMBER(left) == AS_NUMBER(right);
  }
  return left == right;
#else
  if(left.type != right.type) {
    return false;
  }
  switch(left.type) {
  case VAL_BOOL: {
    return AS_BOOL(left) == AS_BOOL(right);
  }
  case VAL_NIL: {
    return true;
  }
  case VAL_NUMBER: {
    return AS_NUMBER(left) == AS_NUMBER(right);
  }

  case VAL_OBJ: {
    // strings are interned
    return AS_OBJ(left) == AS_OBJ(right);
  }

  default: return false; // unreachable
  }
#endif
}