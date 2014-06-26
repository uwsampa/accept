CC ?= clang
ARCH ?= default

acceptrt.$(ARCH).bc: acceptrt.c
	$(CC) $(CFLAGS) -g -O0 -c -emit-llvm -o $@ $<
