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
    FREE(char, object); // FAM
    break;
  }
  case OBJ_FUNCTION: {
    // The function only owns the chunk.
    obj_function_t *function = (obj_function_t *)object;
    free_chunk(&function->chunk);
    FREE(obj_function_t, function);
    break;
  }
  case OBJ_NATIVE: {
    FREE(obj_native_t, object);
    break;
  }

  case OBJ_CLOSURE: {
    // The closure owns the array of upvalues, but not the upvalues.
    obj_closure_t *closure = (obj_closure_t *)object;
    FREE_ARRAY(obj_upvalue_t *, closure->upvalues, closure->upvalue_count);

    // The object function is not freed!
    FREE(obj_closure_t, object);
    break;
  }

  case OBJ_UPVALUE: {
    // Multiple closures may use the same variable, so it is not owned.
    FREE(obj_upvalue_t, object);
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