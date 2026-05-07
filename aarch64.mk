aarch64:
	$(CC) $(CFLAGS) $(LDFLAGS) -O3 -o copyfail copyfail.c aarch64.c
	$(STRIP) -s copyfail

.PHONY: aarch64


aarch64-gdb:
	$(CC) $(CFLAGS) $(LDFLAGS) -g -ggdb3 -o copyfail-gdb copyfail.c aarch64.c

.PHONY: aarch64-gdb


aarch64-payload:
	$(AS) aarch64.s -o aarch64.o
	$(LD) aarch64.o -o aarch64.elf
	$(STRIP) -s aarch64.elf
	xxd -i aarch64.elf

.PHONY: aarch64-payload
