#pragma once
#include <common.h>
#include <object.h>

#define ALLOCATE(type, count) (type *)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate((pointer), sizeof(type), 0)

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, old_count, new_count)                                            \
  (type *)reallocate((pointer), sizeof(type) * (old_count), sizeof(type) * (new_count))

#define FREE_ARRAY(type, pointer, old_count) reallocate((pointer), sizeof(type) * (old_count), 0)

void *reallocate(void *pointer, size_t old_size, size_t new_size);
void free_objects();
void free_object(obj_t *object);
void mark_value(value_t value);
void mark_object(obj_t *object);
void collect_garbage();