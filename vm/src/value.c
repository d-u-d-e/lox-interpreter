#include <memory.h>
#include <stdio.h>
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
  }
}

bool values_equal(value_t left, value_t right)
{
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
  }
}