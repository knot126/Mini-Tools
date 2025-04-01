#define KNOT_COMPRESS_IMPLEMENTATION
#define KNOT_TESTING
#include "../Compress.h"

#include <stdio.h>

size_t write_func(void *context, void *buffer, size_t size) {
	const char *data = buffer;
	
	printf("size=0x%zx ", size);
	
	for (size_t i = 0; i < size; i++) {
		printf("%02x", data[i]&0xff);
		
		if (i != (size - 1)) {
			printf(" ");
		}
	}
	
	printf("\n");
	
	return size;
}

int main(int argc, char *argv[]) {
	return !TestWriteStream(NULL, write_func);
}
