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

static obj_string_t *allocate_string(char *chars, int length)
{
  obj_string_t *str = ALLOCATE_OBJ(obj_string_t, OBJ_TYPE_STRING);
  str->length = length;
  str->chars = chars;
  return str;
}

obj_string_t *take_string(char *chars, int length)
{
  // takes ownership
  return allocate_string(chars, length);
}

obj_string_t *copy_string(const char *chars, int length)
{
  char *heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';
  return allocate_string(heap_chars, length);
}

void print_object(value_t value)
{
  switch(OBJ_TYPE(value)) {
  case OBJ_TYPE_STRING: {
    printf("%s", AS_CSTRING(value));
    break;
  }
  }
}