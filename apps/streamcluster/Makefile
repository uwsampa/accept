TARGET := streamcluster
RUNARGS := 10 20 64 8192 8192 1000 none output.txt 1
LDFLAGS := -lpthread

include ../app.mk

PREFIX=${PARSECDIR}/pkgs/kernels/streamcluster/inst/${PARSECPLAT}
install:
	mkdir -p $(PREFIX)/bin
	cp -f $(TARGET) $(PREFIX)/bin/$(TARGET)
