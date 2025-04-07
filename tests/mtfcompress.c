#include <stdio.h>

#define MI_IMPLEMENTATION
#include "../MTF-IX.h"

int main(int argc, char *argv[]) {
	if (argc < 4) {
		printf("Bad arguments\n");
		return 127;
	}
	
	bool decompress = argv[1][0] == 'd';
	
	FILE *input = fopen(argv[2], "rb");
	FILE *output = fopen(argv[3], "wb");
	
	if (!input) {
		printf("Could not open input file\n");
		return 1;
	}
	
	if (!output) {
		printf("Could not open output file\n");
		return 2;
	}
	
	if (decompress) {
		if (!MIDecompressStream(input, output)) {
			printf("Failed to decompress file\n");
			return 3;
		}
	}
	else {
		if (!MICompressStream(input, output)) {
			printf("Failed to compress file\n");
			return 3;
		}
	}
	
	fclose(input);
	fclose(output);
	
	return 0;
}
