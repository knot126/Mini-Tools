/**
 * Single header mostly untyped arrays implementation
 */

#ifndef _KNOT_ARRAY_H_
#define _KNOT_ARRAY_H_

void *ka_new_internal(size_t element_size, const char *element_type_name, void *initial_elements, size_t initial_size);

#define ka_array(TYPE) ka_new_internal(sizeof(TYPE), #TYPE, NULL, 0)
#define ka_array(TYPE, ELEMENTS, SIZE) ka_new_internal(sizeof(TYPE), #TYPE, ELEMENTS, SIZE)

#ifdef KNOT_ARRAY_IMPLEMENTATION

#include <stdlib.h>

typedef struct {
	size_t size;
	size_t capacity;
	size_t type_size;
	const char *type_name;
} KnotArray_;

#define GET_SELF(PTR) (((KnotArray_ *) PTR) - 1)
#define GET_PTR(SELF) ((void *)(((KnotArray_ *)SELF) + 1))

void *ka_new_internal(size_t element_size, const char *element_type_name, void *initial_elements, size_t initial_size) {
	KnotArray_ *self = malloc(sizeof *self + element_size * initial_size);
	
	if (!self) {
		return NULL;
	}
	
	self->size = element_size * initial_size;
	self->capacity = self->size;
	self->type_size = element_size;
	self->type_name = element_type_name;
	
	// Copy initial elements, if they exist
	if (initial_elements) {
		memcpy(&self->elements, initial_elements, initial_size * element_size);
	}
	
	return GET_PTR(self);
}

void *ka_append_internal()

#endif

#endif
