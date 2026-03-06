#include <stdio.h>
#include <stdlib.h>

#if 1
#define MAGIC 0xdeadbeef

int allocId = 0;

typedef struct CustomMallocHeader {
	int allocId;
	int magic;
	size_t size;
} CustomMallocHeader;

void *test_malloc(size_t size) {
	if (!size) {
		return NULL;
	}
	
	void *block = malloc(size + sizeof(CustomMallocHeader));
	
	CustomMallocHeader *c = block;
	c->allocId = allocId++;
	c->magic = MAGIC;
	c->size = size;
	
	return block + sizeof(CustomMallocHeader);
}

void test_free(void *block) {
	if (!block) {
		return;
	}
	
	if (block) {
		CustomMallocHeader *c = block - sizeof(CustomMallocHeader);
		if (c->magic != MAGIC) {
			fprintf(stderr, "MEMORY CORRUPTION!! addr=%p allocId=%d size=0x%llx\n", block, c->allocId, c->size);
			abort();
		}
	}
	
	free(block - sizeof(CustomMallocHeader));
}

#define KH_MALLOC(x) test_malloc(x)
#define KH_FREE(x) test_free(x)
#endif

#define KHASHTABLE_IMPLEMENTATION
#include "../Hashtable.h"

void print_dict(KH_Dict *dict) {
	size_t size = KH_DictLen(dict);
	printf("dict contents (%zu items):\n", size);
	
	for (size_t i = 0; i < size; i++) {
		printf("  * [0x%zx] %s -> %s\n", i, KH_DictKeyIter(dict, i)->data, KH_DictValueIter(dict, i)->data);
	}
}

void print_slots(KH_Dict *dict) {
	printf("Slots (%zu)\n", dict->data_alloced);
	for (size_t i = 0; i < dict->data_alloced; i++) {
		printf("  - [0x%zx] 0x%08x\n", i, dict->slots[i]);
	}
}

void insert_many_items(KH_Dict *dict, const char *prefix) {
	char item_name[50] = {'\0'};
	
	for (size_t i = 0; i < 50; i++) {
		snprintf(item_name, sizeof item_name, "%s_%zu_%zx", prefix, i, i);
		KH_DictSet(dict, KH_BlobForString(item_name), KH_BlobForString("yip yip yip!"));
	}
}

#define PHAS(KEY) printf("Has '%s' ? %b\n", KEY, KH_DictHas(myDict, KH_BlobForString(KEY)));

int main(int argc, char *argv[]) {
	printf("CreateDictionary\n");
	
	KH_Dict *myDict = KH_CreateDict();
	
	printf("InsertItems\n");
	
	KH_DictSet(myDict, KH_BlobForString("hello"), KH_BlobForString("world!"));
	KH_DictSet(myDict, KH_BlobForString("balls"), KH_BlobForString("I have 69 balls!"));
	KH_DictSet(myDict, KH_BlobForString("iscute"), KH_BlobForString("no :<"));
	KH_DictSet(myDict, KH_BlobForString("urmom"), KH_BlobForString("hehehe"));
	
	KH_DictSet(myDict, KH_BlobForString("coffee"), KH_BlobForString("stain"));
	KH_DictSet(myDict, KH_BlobForString("shitstain"), KH_BlobForString("coffeestain"));
	KH_DictSet(myDict, KH_BlobForString("knot"), KH_BlobForString("one two six"));
	KH_DictSet(myDict, KH_BlobForString(":3"), KH_BlobForString("UwU"));
	
	KH_DictSet(myDict, KH_BlobForString("poop"), KH_BlobForString("name of my cat!"));
	KH_DictSet(myDict, KH_BlobForString("skibidi"), KH_BlobForString("L rizz"));
	
	printf("PrintDict\n");
	
	print_dict(myDict);
	
	printf("InsertManyItems\n");
	
	insert_many_items(myDict, "first");
	
	printf("DeleteItems\n");
	
	KH_DictDelete(myDict, KH_BlobForString("skibidi"));
	KH_DictDelete(myDict, KH_BlobForString("urmom"));
	
	printf("InsertManyItems2\n");
	
	insert_many_items(myDict, "second");
	
	printf("CheckForItems\n");
	
	PHAS("skibidi");
	PHAS("urmom");
	PHAS("hello");
	PHAS("balls");
	PHAS("place");
	PHAS(":3");
	
	printf("GetItems\n");
	
	printf("Value for %s: %s\n", "coffee", KH_DictGet(myDict, KH_BlobForString("coffee"))->data);
	printf("Value for %s: %s\n", "knot", KH_DictGet(myDict, KH_BlobForString("knot"))->data);
	printf("Value for %s: %s\n", "balls", KH_DictGet(myDict, KH_BlobForString("balls"))->data);
	printf("Value for %s: %s\n", ":3", KH_DictGet(myDict, KH_BlobForString(":3"))->data);
	
	printf("PrintDict2\n");
	
	print_dict(myDict);
	
	printf("ReleaseDict\n");
	
	KH_ReleaseDict(myDict);
	
	return 0;
}
