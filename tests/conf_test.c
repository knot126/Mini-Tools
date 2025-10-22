#include <stdio.h>

#define KNOT_PROPERTIES_IMPLEMENTATION
#include "../Properties.h"

const char * const gProps = "first=1\n second=2 \nthird=3\rfourth = four!\r\nfifth the fifth entry :3\r\n\n\n Woah, that's so many spaces!\ntest\ntesty:test\n";

int main(int argc, char *argv[]) {
	KPProperties *props = KPParse(gProps);
	
	if (!props) {
		printf("dead\n");
		return 1;
	}
	
	printf("first value: '%s'\n", KPGet(props, "first"));
	printf("fourth value: '%s'\n", KPGet(props, "fourth"));
	printf("fifth value: '%s'\n", KPGet(props, "fifth"));
	printf("Woah, value: '%s'\n", KPGet(props, "Woah,"));
	printf("Bad! value: '%s'\n", KPGet(props, "Bad!"));
	printf("test value: '%s'\n", KPGet(props, "test"));
	printf("testy value: '%s'\n", KPGet(props, "testy"));
	
	free(props);
	
	return 0;
}
