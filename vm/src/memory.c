#include <memory.h>
#include <stdlib.h>
#include <vm.h>

void *reallocate(void *pointer, size_t old_size, size_t new_size)
{
  if(new_size == 0) {
    free(pointer);
    return NULL;
  }

  void *result = realloc(pointer, new_size);
  if(result == NULL) {
    exit(1);
  }
  return result;
}

void free_object(obj_t *object)
{
  switch(object->type) {
  case OBJ_TYPE_STRING: {
    obj_string_t *str = (obj_string_t *)object;
    FREE(char, str); // FAM
    break;
  }
  }
}

void free_objects()
{
  obj_t *o = g_vm.objects;
  while(o != NULL) {
    obj_t *next = o->next;
    free_object(o);
    o = next;
  }
}