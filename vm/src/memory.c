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
  case OBJ_FUNCTION: {
    // The function only owns the chunk.
    obj_function_t *function = (obj_function_t *)object;
    free_chunk(&function->chunk);
    FREE(obj_function_t, function);
    break;
  }
  case OBJ_NATIVE: {
    obj_native_t *native = (obj_native_t *)object;
    FREE(obj_native_t, native);
    break;
  }

  case OBJ_CLOSURE: {
    // The object function is not freed! Recall that the closure does not own the function!
    obj_closure_t *closure = (obj_closure_t *)object;
    FREE(obj_closure_t, closure);
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