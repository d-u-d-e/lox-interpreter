#pragma once

#include <chunk.h>
#include <common.h>
#include <table.h>
#include <value.h>

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STRING(value) is_obj_type(value, OBJ_STRING)
#define IS_FUNCTION(value) is_obj_type(value, OBJ_FUNCTION)
#define IS_INSTANCE(value) is_obj_type(value, OBJ_INSTANCE)
#define IS_NATIVE(value) is_obj_type(value, OBJ_NATIVE)
#define IS_CLOSURE(value) is_obj_type(value, OBJ_CLOSURE)
#define IS_CLASS(value) is_obj_type(value, OBJ_CLASS)

#define AS_STRING(value) ((obj_string_t *)AS_OBJ(value))
#define AS_CSTRING(value) (((obj_string_t *)AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((obj_function_t *)AS_OBJ(value))
#define AS_INSTANCE(value) ((obj_instance_t *)AS_OBJ(value))
#define AS_NATIVE(value) (((obj_native_t *)AS_OBJ(value))->function)
#define AS_CLOSURE(value) ((obj_closure_t *)AS_OBJ(value))
#define AS_CLASS(value) ((obj_class_t *)AS_OBJ(value))

typedef enum {
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_NATIVE,
  OBJ_STRING,
  OBJ_UPVALUE
} obj_type_t;

struct obj {
  obj_type_t type;
  bool is_marked; // for GC
  struct obj *next;
};

typedef struct {
  struct obj base;
  int arity;
  int upvalue_count;
  chunk_t chunk;
  obj_string_t *name;
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

typedef struct obj_upvalue {
  struct obj base;
  value_t *location;
  value_t closed; // Once closed its value matches the value at location (which is on the stack);
                  // after that location points to &closed
  struct obj_upvalue *next; // Linked list
} obj_upvalue_t;

// Closures are basically functions, but capture the sorroundings locals through upvalues.
// The number of upvalues is tracked by the function object.
typedef struct {
  obj_t base;
  obj_function_t *function;
  obj_upvalue_t **upvalues;
  int upvalue_count;
} obj_closure_t;

typedef struct {
  obj_t base;
  obj_string_t *name;
} obj_class_t;

typedef struct {
  obj_t base;
  obj_class_t *klass;
  table_t fields;
} obj_instance_t;

static inline bool is_obj_type(value_t value, obj_type_t type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

obj_instance_t *new_instance(obj_class_t *klass);
obj_class_t *new_class(obj_string_t *name);
obj_native_t *new_native(native_fn_t function);
obj_upvalue_t *new_upvalue(value_t *location);
obj_closure_t *new_closure(obj_function_t *function);
obj_function_t *new_function();
obj_string_t *allocate_string(const char *chars, int length, uint32_t hash);
uint32_t hash_string(const char *key, int length);
obj_string_t *copy_string(const char *chars, int length);
void print_object(value_t value);