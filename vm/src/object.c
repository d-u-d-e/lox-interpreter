#include <stdio.h>
#include <string.h>

#include <memory.h>
#include <object.h>
#include <value.h>
#include <vm.h>

#define ALLOCATE_OBJ(type, obj_type) (type *)allocate_obj(sizeof(type), obj_type)

static obj_t *allocate_obj(size_t size, obj_type_t type)
{
  obj_t *object = (obj_t *)reallocate(NULL, 0, size);
  object->type = type;
  object->is_marked = false;

  // link into VM object list
  object->next = g_vm.objects;
  g_vm.objects = object;

#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", object, size, type);
#endif

  return object;
}

obj_native_t *new_native(native_fn_t function)
{
  obj_native_t *native = ALLOCATE_OBJ(obj_native_t, OBJ_NATIVE);
  native->function = function;
  return native;
}

obj_upvalue_t *new_upvalue(value_t *location)
{
  obj_upvalue_t *upvalue = ALLOCATE_OBJ(obj_upvalue_t, OBJ_UPVALUE);
  upvalue->location = location;
  upvalue->closed = NIL_VAL;
  upvalue->next = NULL;
  return upvalue;
}

obj_closure_t *new_closure(obj_function_t *function)
{
  // Allocate space for the upvalues; the exact number is computed at compile time
  obj_upvalue_t **upvalues = ALLOCATE(obj_upvalue_t *, function->upvalue_count);

  // Initialize the upvalues to NULL
  for(int i = 0; i < function->upvalue_count; i++) {
    upvalues[i] = NULL;
  }

  // Note how we initialize the upvalues to NULL before the object creation.
  // This is important for the garbage collector, to make sure it does not see uninitialized pointers.
  obj_closure_t *closure = ALLOCATE_OBJ(obj_closure_t, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalue_count = function->upvalue_count;
  return closure;
}

obj_function_t *new_function()
{
  obj_function_t *function = ALLOCATE_OBJ(obj_function_t, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalue_count = 0;
  function->name = NULL;
  init_chunk(&function->chunk);
  return function;
}

obj_string_t *copy_string(const char *chars, int length)
{
  uint32_t hash = hash_string(chars, length);
  obj_string_t *interned = table_find_string(&g_vm.strings, chars, length, hash);
  if(interned != NULL) {
    // Return the existing string object
    return interned;
  }

  // Different string must be allocated
  obj_string_t *res = allocate_string(chars, length, hash);
  // Intern the string: note that GC can run, so we must make the brand new string reachable by
  // pushing it onto the stack
  push(OBJ_VAL(res));
  table_set(&g_vm.strings, res, NIL_VAL);
  pop();
  return res;
}

obj_string_t *allocate_string(const char *chars, int length, uint32_t hash)
{
  obj_string_t *res = (obj_string_t *)allocate_obj(sizeof(obj_string_t) + length + 1, OBJ_STRING);
  memcpy(res->chars, chars, length);
  res->chars[length] = '\0';
  res->length = length;
  res->hash = hash;
  return res;
}

uint32_t hash_string(const char *key, int length)
{
  // https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
  uint32_t hash = 2166136261;
  for(int i = 0; i < length; i++) {
    hash ^= key[i];
    hash *= 16777619;
  }
  return hash;
}

static void print_function(obj_function_t *function)
{
  if(function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}

void print_object(value_t value)
{
  switch(OBJ_TYPE(value)) {
  case OBJ_STRING: {
    obj_string_t *str = AS_STRING(value);
    printf("%s", str->chars);
    break;
  }

  case OBJ_NATIVE: {
    printf("<native fn>");
    break;
  }

  case OBJ_CLOSURE: {
    // Exactly like OBJ_FUNCTION, because from a user perspective, the closure is just an
    // implementation detail.
    print_function(AS_CLOSURE(value)->function);
    break;
  }

  case OBJ_FUNCTION: {
    print_function(AS_FUNCTION(value));
    break;
  }

  case OBJ_UPVALUE: {
    // This gets never called by the runtime, only for compiler warning
    printf("upvalue");
    break;
  }
  }
}