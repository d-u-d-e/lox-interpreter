#pragma once

#include <chunk.h>
#include <common.h>
#include <value.h>

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STRING(value) is_obj_type(value, OBJ_TYPE_STRING)
#define IS_FUNCTION(value) is_obj_type(value, OBJ_FUNCTION)
#define IS_NATIVE(value) is_obj_type(value, OBJ_NATIVE)
#define IS_CLOSURE(value) is_obj_type(value, OBJ_CLOSURE)

#define AS_STRING(value) ((const obj_string_t *)AS_OBJ(value))
#define AS_CSTRING(value) (((const obj_string_t *)AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((obj_function_t *)AS_OBJ(value))
#define AS_NATIVE(value) (((obj_native_t *)AS_OBJ(value))->function)
#define AS_CLOSURE(value) ((obj_closure_t *)AS_OBJ(value))

typedef enum { OBJ_CLOSURE, OBJ_FUNCTION, OBJ_NATIVE, OBJ_TYPE_STRING, OBJ_UPVALUE } obj_type_t;

struct obj {
  obj_type_t type;
  struct obj *next;
};

typedef struct {
  struct obj base;
  int arity;
  int upvalue_count;
  chunk_t chunk;
  const obj_string_t *name;
} obj_function_t;

// Native functions are implemented in C and have a simpler representation than Lox functions.
// E.g. there is no bytecode for them.
// They recieve arguments through the 'args' array.
typedef value_t (*native_fn_t)(int arg_count, value_t *args);

typedef struct {
  struct obj base;
  native_fn_t function;
} obj_native_t;

struct obj_string {
  struct obj base;
  int length;
  uint32_t hash;
  char chars[]; // flexible array
};

typedef struct obj_value {
  struct obj base;
  value_t *location;
  value_t closed; // Once closed its value matches the value at location (which is on the stack)
  struct obj_value *next; // Linked list
} obj_upvalue_t;

// Closures are basically functions, but capture the sorroundings locals through upvalues.
// The number of upvalues is tracked by the function object.
typedef struct {
  obj_t base;
  obj_function_t *function;
  obj_upvalue_t **upvalues;
  int upvalue_count;
} obj_closure_t;

static inline bool is_obj_type(value_t value, obj_type_t type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

obj_native_t *new_native(native_fn_t function);
obj_upvalue_t *new_upvalue(value_t *location);
obj_closure_t *new_closure(obj_function_t *function);
obj_function_t *new_function();
obj_string_t *allocate_string(int length);
uint32_t hash_string(const char *key, int length);
const obj_string_t *copy_string(const char *chars, int length);
void print_object(value_t value);