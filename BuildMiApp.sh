#!/usr/bin/bash

echo "#define MI_COMPILE_MAIN
#define MI_IMPLEMENTATION
#include \"Mi.h\"" > Mi.c
# clang -O2 -o mi Mi.c
clang -o mi Mi.c
rm Mi.c
