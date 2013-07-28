all: tcpmux wsmux

tcpmux:
	$(CC) -std=c99 -Wall -o tcpmux tcpmux.c mux.c \
		../redismq/*.c \
		../sev/*.c \
		-lev -lhiredis

wsmux:
	$(CC) -std=c99 -Wall -o wsmux wsmux.c mux.c \
		../redismq/*.c \
		../sev/*.c \
		../libws/*.c \
		-lev -lhiredis

clean:
	rm -rf *.dSYM tcpmux wsmux

.PHONY: all tcpmux wsmux clean
