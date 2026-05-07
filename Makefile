SYSROOT ?= /
CFLAGS += -I$(SYSROOT)usr/include
LDFLAGS += -L$(SYSROOT)usr/lib

CC := $(CROSS_COMPILE)gcc --sysroot=$(SYSROOT)
LD := $(CROSS_COMPILE)ld --sysroot=$(SYSROOT)
AS := $(CROSS_COMPILE)as
STRIP :=$(CROSS_COMPILE)strip

all: amd64 aarch64

clean:
	rm -rf copyfail copyfail-gdb *.o *.elf

.PHONY: all clean

include amd64.mk aarch64.mk
