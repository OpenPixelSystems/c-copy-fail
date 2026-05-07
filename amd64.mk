amd64:
	$(CC) -O3 -o copyfail copyfail.c amd64.c
	$(STRIP) -s copyfail

.PHONY: amd64


amd64-gdb:
	$(CC) -g -ggdb3 -o copyfail-gdb copyfail.c amd64.c

.PHONY: amd64-gdb

