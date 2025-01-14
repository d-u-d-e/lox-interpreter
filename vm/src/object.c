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

obj_string_t *copy_string(const char *chars, int length)
{
  obj_string_t *res = allocate_string(length);
  memcpy(res->chars, chars, length);
  res->chars[length] = '\0';
  return res;
}

obj_string_t *allocate_string(int length)
{
  obj_string_t *res = ALLOCATE(obj_string_t, sizeof(obj_string_t) + length + 1);
  res->length = length;
  res->base.type = OBJ_TYPE_STRING;
  return res;
}

void print_object(value_t value)
{
  switch(OBJ_TYPE(value)) {
  case OBJ_TYPE_STRING: {
    obj_string_t *str = AS_STRING(value);
    printf("%s", str->chars);
    break;
  }
  }
}