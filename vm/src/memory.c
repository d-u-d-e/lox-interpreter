#include <compiler.h>
#include <memory.h>
#include <stdlib.h>
#include <vm.h>

#ifdef DEBUG_LOG_GC
#include <debug.h>
#include <stdio.h>
#endif

#define GC_HEAP_GROWTH_FACTOR 2

void *reallocate(void *pointer, size_t old_size, size_t new_size)
{
  g_vm.bytes_allocated += new_size - old_size;

  if(new_size > old_size) {
#ifdef DEBUG_STRESS_GC
    collect_garbage();
#endif
  }

  if(g_vm.bytes_allocated > g_vm.next_gc) {
    collect_garbage();
  }

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
#ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", (void *)object, object->type);
#endif

  switch(object->type) {
  case OBJ_STRING: {
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

  case OBJ_BOUND_METHOD: {
    FREE(obj_bound_method_t, object);
    break;
  }

  case OBJ_CLASS: {
    obj_class_t *klass = (obj_class_t *)object;
    free_table(&klass->methods);
    FREE(obj_class_t, object);
    break;
  }

  case OBJ_INSTANCE: {
    obj_instance_t *instance = (obj_instance_t *)object;
    free_table(&instance->fields);
    FREE(obj_instance_t, object);
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
  free(g_vm.gray_stack);
}

void mark_object(obj_t *object)
{
  if(object == NULL) {
    return;
  }
  // Don't add an already gray object
  if(object->is_marked) {
    return;
  }
#ifdef DEBUG_LOG_GC
  printf("%p mark ", (void *)object);
  print_value(OBJ_VAL(object));
  printf("\n");
#endif
  object->is_marked = true;

  if(g_vm.gray_capacity < g_vm.gray_count + 1) {
    g_vm.gray_capacity = GROW_CAPACITY(g_vm.gray_capacity);
    // Note that we realloc, not reallocate since the gray stack is not managed by the GC
    g_vm.gray_stack = (obj_t **)realloc(g_vm.gray_stack, sizeof(obj_t *) * g_vm.gray_capacity);
    if(g_vm.gray_stack == NULL) {
      // Out of memory error, simply exit
      exit(1);
    }
  }
  // The object becomes gray
  g_vm.gray_stack[g_vm.gray_count++] = object;
}

void mark_value(value_t value)
{
  if(IS_OBJ(value)) {
    mark_object(AS_OBJ(value));
  }
}

static void mark_array(value_array_t *array)
{
  for(int i = 0; i < array->count; i++) {
    mark_value(array->values[i]);
  }
}

static void blacken_object(obj_t *object)
{
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void *)object);
  print_value(OBJ_VAL(object));
  printf("\n");
#endif

  switch(object->type) {
  case OBJ_BOUND_METHOD: {
    obj_bound_method_t *bound_method = (obj_bound_method_t *)object;
    // Tracing the closure is not really necessary, because the receiver is an object instance with
    // a pointer to the class containing a table to all the methods (closures)
    mark_value(bound_method->receiver);
    mark_object((obj_t *)bound_method->method);
    break;
  }

  case OBJ_CLASS: {
    obj_class_t *klass = (obj_class_t *)object;
    mark_object((obj_t *)klass->name);
    mark_table(&klass->methods);
    break;
  }

  case OBJ_INSTANCE: {
    obj_instance_t *instance = (obj_instance_t *)object;
    mark_object((obj_t *)instance->klass);
    mark_table(&instance->fields);
    break;
  }

  case OBJ_CLOSURE: {
    obj_closure_t *closure = (obj_closure_t *)object;
    mark_object((obj_t *)closure->function);
    for(int i = 0; i < closure->upvalue_count; i++) {
      // Mark closed upvalues
      mark_object((obj_t *)closure->upvalues[i]);
    }
    break;
  }
  case OBJ_FUNCTION: {
    obj_function_t *function = (obj_function_t *)object;
    mark_object((obj_t *)function->name);
    mark_array(&function->chunk.constants);
    break;
  }
  case OBJ_UPVALUE: {
    mark_value(((obj_upvalue_t *)object)->closed);
    break;
  }
  case OBJ_NATIVE:
  case OBJ_STRING: {
    // These contain no references to other objs.
    break;
  }
  }
}

static void mark_roots()
{
  // Most roots are on the stack
  for(value_t *slot = g_vm.stack; slot < g_vm.stack_top; slot++) {
    mark_value(*slot);
  }

  // Closures are less obvious roots
  for(int i = 0; i < g_vm.frame_count; i++) {
    callframe_t *frame = &g_vm.frames[i];
    mark_object((obj_t *)frame->closure);
  }

  // Open upvalues are less obvious roots
  for(obj_upvalue_t *upvalue = g_vm.open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
    mark_object((obj_t *)upvalue);
  }

  // Globals are also roots
  mark_table(&g_vm.globals);

  // Variables allocated by the compiler are also roots
  mark_compiler_roots();

  // Strings are interned by the VM, so we deliberately not consider them to be roots.
  // But we don't want dangling pointers in the string table!
  // The right phase to remove them is between the mark and sweep phases
}

static void trace_references()
{
  while(g_vm.gray_count > 0) {
    obj_t *object = g_vm.gray_stack[--g_vm.gray_count];
    // Mark the object black
    blacken_object(object);
  }
}

static void sweep()
{
  obj_t *previous = NULL;
  obj_t *object = g_vm.objects;
  while(object != NULL) {
    if(object->is_marked) {
      // Set it white for the next cycle
      object->is_marked = false;
      previous = object;
      object = object->next;
    }
    else {
      obj_t *unreached = object;
      object = object->next;
      // Unlink it
      if(previous != NULL) {
        previous->next = object;
      }
      else {
        g_vm.objects = object;
      }
      free_object(unreached);
    }
  }
}

void collect_garbage()
{
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
  size_t before = g_vm.bytes_allocated;
#endif

  mark_roots();
  trace_references();
  // Remove white strings from the set of interned strings,
  // since they are not considered roots
  table_remove_white(&g_vm.strings);
  sweep();

  // Adjust the threshold for the next collection
  g_vm.next_gc = g_vm.bytes_allocated * GC_HEAP_GROWTH_FACTOR;

#ifdef DEBUG_LOG_GC
  printf("-- gc end\n");
  printf("   collected %zu bytes (from %zu to %zu) next at %zu\n", before - g_vm.bytes_allocated,
         before, g_vm.bytes_allocated, g_vm.next_gc);
#endif
}