BUILTDIR := $(ACCEPT_DIR)/build/built
CXX := $(BUILTDIR)/bin/clang++
LLVMLINK := $(BUILTDIR)/bin/llvm-link

LIBERROR_BCS = liberror.bc

# By default, build for the host platform.
.PHONY: all clean
all: liberror_all.bc

clean:
	rm -rf *.bc *.txt

%.bc: %.cpp
	$(CXX) $(CXXFLAGS) -c -emit-llvm -o $@ $<

liberror_all.bc: $(LIBERROR_BCS)
	$(LLVMLINK) $^ > $@
	rm *.txt
