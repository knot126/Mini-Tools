/**
 * Knot's Blob-to-Blob Hash Table - Copyright (C) 2024 Knot126, Zlib License
 * 
 * This single-header library implements a basic hash table which can map
 * arbitrary blobs of data to other arbitrary blobs of data. 'Blob' basically
 * means strings including NUL's.
 * 
 * The hash table has the following properties:
 * 
 *   - Uses the DJB2 hash function
 *   - Collision resolution using open addressing
 *   - Preserves the order of keys by insertion by storing indexes to values in
 *     slots instead of the values themselves
 *   - Common case O(1) insert, update, retrieve, and member check
 *   - O(n) delete (makes deleting keys less complex with order-preserving)
 *   - Only supports power-of-two capacity sizes ATM
 * 
 * Some general usage notes:
 * 
 *   - Exposed hash table functions never make internal copies of KH_Blob's, but
 *     they will always free them if they aren't used longer term. They "steal"
 *     them, per se. This applies even to functions like KH_DictHas().
 *   - In functions where KH_Blob's are returned, copies are also NOT made. You
 *     should not mutate them.
 * 
 * Short example:
 * 
 *   void func(void) {
 *       KH_Dict *myDict = KH_CreateDict();
 *       
 *       if (!myDict) {
 *           // Error! Handle it.
 *       }
 *       
 *       // Create some values. We could check if they succeede by checking that
 *       // their return value is true, but we don't have to.
 *       KH_DictSet(myDict, KH_BlobForString("roughness"), KH_BlobForString("13"));
 *       KH_DictSet(myDict, KH_BlobForString("shine"), KH_BlobForString("0.6"));
 *       KH_DictSet(myDict, KH_BlobForString("shadow"), KH_BlobForString("true"));
 *       
 *       printf("shine = %s\n", KH_DictGet(myDict, KH_BlobForString("shine"))->data);
 *   }
 * 
 * Using blobs:
 * 
 *   - In KHashTable, keys and values are stored in blobs, which are basically
 *     any buffer. They can contain NUL bytes.
 *   
 *   - Almost all functions take "ownership" of the blob you pass them, and use
 *     it internally or automatically free if it it's not used. If you want to
 *     pass a blob with the same value as a previous one, you will need to make
 *     a copy of it.
 *   
 *   - KH_Blob *KH_CreateBlob(uint8_t *buffer, size_t length)
 *     
 *     Creates a new blob for use with hash table functions. Returns NULL if it
 *     fails to allocate memory.
 *   
 *   - KH_Blob *KH_BlobForString(char *string)
 *     
 *     Creates a new blob from a NUL-terminated C string. Returns NULL if it
 *     fails to allocate memory.
 *   
 *   - void KH_ReleaseBlob(KH_Blob *blob)
 *     
 *     Releases all resources associated with a blob. Normally, calling this
 *     function is unnessicary, since the hash table functions free blobs if
 *     they are not used later.
 *   
 *   - The blob structure has the following members:
 *     
 *     - data, an array of the bytes the blob contains
 *     - length, the length of the data the blob holds
 *     
 *     Acessing the blob structure directly is needed, since there are no
 *     functions to get or set data.
 * 
 * Using the hash table (dictionary):
 * 
 *   - KHashTable calls hash tables "dictionaries".
 *   
 *   - KH_Dict *KH_CreateDict(void)
 *     
 *     Creates a new dictionary. Returns NULL if it fails.
 *   
 *   - bool KH_DictSet(KH_Dict *dict, KH_Blob *key, KH_Blob *value)
 *     
 *     Sets the value associated with the key to the given value. Returns true
 *     on success, and false on failure.
 *   
 *   - KH_Blob *KH_DictGet(KH_Dict *dict, KH_Blob *key)
 *     
 *     Gets the blob assocaited with the given key. The blob returned should be
 *     treated as immutable and should not be freed, since it's the same one
 *     used to store the blob internally. Returns NULL if there isn't a value
 *     assocaited with the given key.
 *   
 *   - bool KH_DictHas(KH_Dict *dict, KH_Blob *key)
 *     
 *     Check if there is any value assocaited with the given key. Returns true
 *     if there is, or false if not.
 *   
 *   - bool KH_DictDelete(KH_Dict *dict, KH_Blob *key)
 *     
 *     Delete the mapping assocaited with the given key. Returns true if
 *     successful, or false if not.
 *   
 *   - void KH_ReleaseDict(KH_Dict *dict)
 *     
 *     Releases all resources associated with a dictionary.
 * 
 * Other hash table functions:
 * 
 *   - size_t KH_DictLen(KH_Dict *dict)
 *     
 *     Return the number of entries in the hash table.
 *   
 *   - KH_Blob *KH_DictKeyIter(KH_Dict *dict, size_t index)
 *     
 *     Return the key for the i-th key-value pair in the dictionary, or NULL
 *     if it would be out of bounds.
 *   
 *   - KH_Blob *KH_DictValueIter(KH_Dict *dict, size_t index)
 *     
 *     Return the value for the i-th key-value pair in the dictionary, or NULL
 *     if it would be out of bounds.
 * 
 * Zlib License
 * ------------
 * 
 * Copyright (c) 2024 Knot126
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef _KH_HEADER
#define _KH_HEADER
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>

enum {
	KH_HASH_EMPTY = 0xffffffff,
	KH_HASH_DELETED = 0xfffffffe,
};

#define KH_NOT_FOUND ((size_t)-1)

typedef uint32_t kh_hash_t;
typedef struct KH_Blob {
	size_t length;
	kh_hash_t hash;
	const uint8_t data[0];
} KH_Blob;

typedef uint32_t KH_Slot;

typedef struct KH_DictPair {
	KH_Blob *key;
	KH_Blob *value;
} KH_DictPair;

typedef struct KH_Dict {
	KH_Slot *slots;
	KH_DictPair *pairs;
	size_t data_count;
	size_t data_alloced; // Must be a power of two
} KH_Dict;

KH_Blob *KH_CreateBlob(const uint8_t *buffer, const size_t length);
KH_Blob *KH_BlobForString(const char *str);
void KH_ReleaseBlob(KH_Blob *blob);

KH_Dict *KH_CreateDict(void);
void KH_ReleaseDict(KH_Dict *dict);
bool KH_DictSet(KH_Dict *self, KH_Blob *key, KH_Blob *value);
KH_Blob *KH_DictGet(KH_Dict *self, KH_Blob *key);
bool KH_DictHas(KH_Dict *self, KH_Blob *key);
bool KH_DictDelete(KH_Dict *self, KH_Blob *key);
KH_Blob *KH_DictKeyIter(KH_Dict *self, size_t index);
KH_Blob *KH_DictValueIter(KH_Dict *self, size_t index);
size_t KH_DictLen(KH_Dict *self);

#ifdef KHASHTABLE_IMPLEMENTATION
static kh_hash_t KH_Hash(const uint8_t *buffer, const size_t length) {
	kh_hash_t hash = 5381;
	
	for (size_t i = 0; i < length; i++) {
		hash = ((hash << 5) + hash) ^ buffer[i];
	}
	
	return hash;
}

KH_Blob *KH_CreateBlob(const uint8_t *buffer, const size_t length) {
	KH_Blob *blob = malloc(sizeof *blob + length);
	
	if (!blob) {
		return NULL;
	}
	
	blob->length = length;
	blob->hash = KH_Hash(buffer, length);
	memcpy((void *) blob->data, buffer, length);
	
	return blob;
}

KH_Blob *KH_BlobForString(const char *str) {
	return KH_CreateBlob((const uint8_t *) str, strlen(str) + 1);
}

static bool KH_BlobEqual(KH_Blob *blob1, KH_Blob *blob2) {
	if (blob1 == blob2) {
		return true;
	}
	
	if (blob1->hash != blob2->hash || blob1->length != blob2->length) {
		return false;
	}
	
	return memcmp(blob1->data, blob2->data, blob1->length) == 0;
}

void KH_ReleaseBlob(KH_Blob *blob) {
	free(blob);
}

static uint32_t KH_BlobStartingIndexForSize(uint32_t hash, size_t size) {
	// WARNING: Only works for powers of two
	return hash & (size - 1);
}

static void KH_InsertSlot(KH_Slot *slots, size_t nslots, uint32_t hash, uint32_t index) {
	uint32_t slot_index = KH_BlobStartingIndexForSize(hash, nslots);
	
	while (1) {
		if (slots[slot_index] == KH_HASH_EMPTY || slots[slot_index] == KH_HASH_DELETED) {
			slots[slot_index] = index;
			break;
		}
		
		slot_index = (slot_index + 1) & (nslots - 1);
	}
}

static KH_Dict *KH_ResizeDict(KH_Dict *self) {
	/**
	 * Resize a dict, or if it has size zero, allocate the initial memory.
	 */
	
	// New size of the prealloced memory and index data
	size_t new_size = (self->data_alloced) ? (2 * self->data_alloced) : (8);
	
	// Alloc new slots and pair data
	KH_Slot *new_slots = malloc(sizeof *self->slots * new_size);
	KH_DictPair *new_pairs = malloc(sizeof *self->pairs * new_size);
	
	if (!new_slots || !new_pairs) {
		free(new_slots);
		free(new_pairs);
		return NULL;
	}
	
	// Init slots to empty (0xFFFFFFFF)
	for (size_t i = 0; i < new_size; i++) {
		new_slots[i] = KH_HASH_EMPTY;
	}
	
	// Init pairs to empty (NULL)
	memset(new_pairs, 0, sizeof *self->pairs * new_size);
	
	// Copy old pairs to new pairs
	size_t j = 0;
	
	for (size_t i = 0; i < self->data_alloced; i++) {
		// This used to continue, but since it can never be sparse due the the
		// current way we delete things just breaking is faster.
		if (!self->pairs[i].key || !self->pairs[i].value) {
			break;
		}
		
		new_pairs[j].key = self->pairs[i].key;
		new_pairs[j].value = self->pairs[i].value;
		KH_InsertSlot(new_slots, new_size, new_pairs[j].key->hash, j);
		j++;
	}
	
	// j is now the new data_count
	
	// We should be ready to free old stuff, place new stuff
	free(self->slots);
	free(self->pairs);
	self->slots = new_slots;
	self->pairs = new_pairs;
	self->data_alloced = new_size;
	self->data_count = j;
	
	return self;
}

static bool KH_DictInsert(KH_Dict *self, KH_Blob *key, KH_Blob *value) {
	/**
	 * Insert an entry into the hash table. The key must not exist.
	 */
	
	// Resize if load factor > 0.625, around Wikipedia's recommendation of
	// resizing at 0.6-0.75
	if (self->data_count >= ((self->data_alloced >> 1) + (self->data_alloced >> 3))) {
		self = KH_ResizeDict(self);
		
		if (!self) {
			return false;
		}
	}
	
	self->pairs[self->data_count].key = key;
	self->pairs[self->data_count].value = value;
	
	KH_InsertSlot(self->slots, self->data_alloced, key->hash, self->data_count);
	
	self->data_count++;
	
	return true;
}

static void KH_DictChange(KH_Dict *self, size_t index, KH_Blob *value) {
	/**
	 * Change the value for a key that already exists, given the index to the
	 * key.
	 */
	
	free(self->pairs[index].value);
	self->pairs[index].value = value;
}

static size_t KH_DictLookupIndex(KH_Dict *self, KH_Blob *key) {
	/**
	 * Find the index of a pair given its key. Returns the index or KH_NOT_FOUND
	 * if none was found.
	 */
	
	uint32_t slot_index = KH_BlobStartingIndexForSize(key->hash, self->data_alloced);
	
	for (size_t i = 0; i < self->data_alloced; i++) {
		KH_Slot slot = self->slots[(slot_index + i) & (self->data_alloced - 1)];
		
		// Empty, never-used slot which won't have anything we're looking for
		// located after it
		if (slot == KH_HASH_EMPTY) {
			break;
		}
		
		// Once used slot which may still have hits after it
		if (slot == KH_HASH_DELETED) {
			continue;
		}
		
		// If the key we're looking up matches the key indexed by the current
		// slot, this is a hit and it should be returned.
		if (KH_BlobEqual(key, self->pairs[slot].key)) {
			return slot;
		}
	}
	
	// print_slots(self);
	
	return KH_NOT_FOUND;
}

static void KH_DictRemove(KH_Dict *self, size_t index) {
	/**
	 * Deletes the value at the given index, and updates the slots as needed.
	 */
	
	// Free key and value, they arent needed anymore
	free(self->pairs[index].key);
	free(self->pairs[index].value);
	
	// Move pairs to lower indexes
	// memmove(&self->pairs[index], &self->pairs[index + 1], (size_t)&self->pairs[index + 1] - (size_t)&self->pairs[self->data_count]);
	for (size_t i = index + 1; i < self->data_count; i++) {
		self->pairs[i - 1] = self->pairs[i];
	}
	
	self->data_count--;
	
	// Fix up the slots
	for (size_t i = 0; i < self->data_alloced; i++) {
		// If its already deleted or empty then no fixup should be needed
		if (self->slots[i] == KH_HASH_DELETED || self->slots[i] == KH_HASH_EMPTY) {
			continue;
		}
		
		// If it's greater than the current index we need to decrement one
		else if (self->slots[i] > index) {
			self->slots[i] -= 1;
		}
		
		// If it's the index we deleted we need to mark it deleted
		else if (self->slots[i] == index) {
			self->slots[i] = KH_HASH_DELETED;
		}
		
		// The other case (slot is less than index) requires no action
		else {
		}
	}
}

KH_Dict *KH_CreateDict(void) {
	KH_Dict *dict = malloc(sizeof *dict);
	memset(dict, 0, sizeof *dict);
	return dict;
}

void KH_ReleaseDict(KH_Dict *dict) {
	free(dict->slots);
	
	for (size_t i = 0; i < dict->data_count; i++) {
		free(dict->pairs[i].key);
		free(dict->pairs[i].value);
	}
	
	free(dict->pairs);
	
	free(dict);
}

bool KH_DictSet(KH_Dict *self, KH_Blob *key, KH_Blob *value) {
	/**
	 * Insert a (key, value) pair into the dictionary, overwriting any existing
	 * one.
	 */
	
	size_t index = KH_DictLookupIndex(self, key);
	
	if (index == KH_NOT_FOUND) {
		return KH_DictInsert(self, key, value);
	}
	else {
		KH_DictChange(self, index, value);
		free(key);
		return true;
	}
}

KH_Blob *KH_DictGet(KH_Dict *self, KH_Blob *key) {
	/**
	 * Get a value blob by a key
	 */
	
	size_t index = KH_DictLookupIndex(self, key);
	
	free(key);
	
	if (index == KH_NOT_FOUND) {
		return NULL;
	}
	else {
		return self->pairs[index].value;
	}
}

bool KH_DictHas(KH_Dict *self, KH_Blob *key) {
	/**
	 * Check if the dict has a pair with a given key
	 */
	
	size_t index = KH_DictLookupIndex(self, key);
	free(key);
	return index != KH_NOT_FOUND;
}

bool KH_DictDelete(KH_Dict *self, KH_Blob *key) {
	/**
	 * Delete a key-value pair by its key
	 */
	
	size_t index = KH_DictLookupIndex(self, key);
	
	if (index == KH_NOT_FOUND) {
		free(key);
		return false;
	}
	else {
		KH_DictRemove(self, index);
		free(key);
		return true;
	}
}

KH_Blob *KH_DictKeyIter(KH_Dict *self, size_t index) {
	/**
	 * Return the blob associated with the key at the given index. This can be
	 * used to iterate all keys in the dict, with a return value of NULL
	 * signaling the end of the dict.
	 */
	
	return (index < self->data_count) ? self->pairs[index].key : NULL;
}

KH_Blob *KH_DictValueIter(KH_Dict *self, size_t index) {
	/**
	 * Return the blob associated with the value at the given index.
	 */
	
	return (index < self->data_count) ? self->pairs[index].value : NULL;
}

size_t KH_DictLen(KH_Dict *self) {
	/**
	 * Return the number of key-value pairs in this dict
	 */
	
	return self->data_count;
}
#endif // KHASHTABLE_IMPLEMENTATION

#endif // _KH_HEADER
