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

  // link into VM object list
  object->next = g_vm.objects;
  g_vm.objects = object;

  return object;
}

obj_native_t *new_native(native_fn_t function)
{
  obj_native_t *native = ALLOCATE_OBJ(obj_native_t, OBJ_NATIVE);
  native->function = function;
  return native;
}

obj_function_t *new_function()
{
  obj_function_t *function = ALLOCATE_OBJ(obj_function_t, OBJ_FUNCTION);
  function->arity = 0;
  function->name = NULL;
  init_chunk(&function->chunk);
  return function;
}

const obj_string_t *copy_string(const char *chars, int length)
{
  uint32_t hash = hash_string(chars, length);
  const obj_string_t *interned = table_find_string(&g_vm.strings, chars, length, hash);
  if(interned != NULL) {
    // return the existing string object
    return interned;
  }

  // different string must be allocated
  obj_string_t *res = allocate_string(length);
  memcpy(res->chars, chars, length);
  res->chars[length] = '\0';
  res->hash = hash;
  // intern the string
  table_set(&g_vm.strings, res, NIL_VAL);
  return res;
}

obj_string_t *allocate_string(int length)
{
  obj_string_t *res = ALLOCATE(obj_string_t, sizeof(obj_string_t) + length + 1);
  res->length = length;
  res->base.type = OBJ_TYPE_STRING;
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
  case OBJ_TYPE_STRING: {
    const obj_string_t *str = AS_STRING(value);
    printf("%s", str->chars);
    break;
  }

  case OBJ_NATIVE: {
    printf("<native fn>");
    break;
  }

  case OBJ_FUNCTION: {
    print_function(AS_FUNCTION(value));
    break;
  }
  }
}