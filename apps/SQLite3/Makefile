TARGET := sqlite

bld:
	mkdir -p $@

bld/sqlite3.c: bld
	cd bld ; ../sqlite/configure
	cd bld ; make sqlite3.c

SOURCES := bld/sqlite3.c

APP_MK := ../app.mk
include $(APP_MK)
