all:
	$(CC) -std=c99 -Wall -o tcpmux tcpmux.c \
		../redismq/*.c \
		../sev/*.c \
		-lev -lhiredis

clean:
	rm -rf *.dSYM tcpmux
