#include <memory.h>
#include <value.h>
#include <stdio.h>

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

void print_value(value_t value) { printf("%g", value); }