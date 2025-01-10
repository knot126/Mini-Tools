#define FILE_UTILS_IMPLEMENTATION
#include "../FileUtils.h"

#define streq(x, y) (!strcmp(x, y))
#define tfstr(v) (v ? "true" : "false")

int main(int argc, const char *argv[]) {
	if (argc < 3) {
		printf("Usage: %s <test name> <filename>\n", argv[0]);
		return 1;
	}
	
	if (streq(argv[1], "load")) {
		char *data = NULL;
		size_t len = 0;
		
		bool result = FULoad(argv[2], (void **) &data, &len);
		
		printf("Result: %s\n", tfstr(result));
		
		if (result) {
			fwrite(data, 1, len, stdout);
		}
	}
	else if (streq(argv[1], "save")) {
		bool result = FUSaveString(argv[2], "Hello, world! This is a test of FileUtils.\n");
		
		printf("Result: %s\n", tfstr(result));
	}
	
	return 0;
}
