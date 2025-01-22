#pragma once
#include <vm.h>

obj_function_t * compile(const char *source);
void mark_compiler_roots();