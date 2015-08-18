CXX := $(BUILTDIR)/bin/clang++
LLVMLINK := $(BUILTDIR)/bin/llvm-link

# By default, build for the host platform.
.PHONY: all clean
all: $(LINKEDBC)

clean:
	rm -rf *.bc *.txt

%.bc: %.cpp
	$(CXX) $(CXXFLAGS) -g -O0 -c -emit-llvm -o $@ $<

liberror_all.bc: $(wildcard *.bc)
	$(LLVMLINK) $^ > $@