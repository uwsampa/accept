CC ?= clang

ARCHES := default zynq msp430

# By default, build for the host platform.
.PHONY: all clean
all: acceptrt.default.bc

clean:
	rm -rf $(ARCHES:%=acceptrt.%.bc)

acceptrt.%.bc: acceptrt.%.c
	$(CC) $(CFLAGS) -g -O0 -c -emit-llvm -o $@ $<
