#include <malloc.h>

#define STACKARRAY(type, name, size) \
		type* name = (type*) _malloca((size + 1) * sizeof(type));