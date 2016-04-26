CC := clang

ARCHES := default zynq msp430

# By default, build for the host platform.
.PHONY: all clean
all: acceptrt.default.bc

clean:
	rm -rf *.bc *.txt

acceptrt.%.bc: acceptrt.%.c
	$(CC) $(CFLAGS) -g -O0 -c -emit-llvm -o $@ $<

acceptrt.msp430.bc: acceptrt.msp430.c
	$(CC) $(CFLAGS) -target msp430-elf -g -O0 -c -emit-llvm -o $@ $<
