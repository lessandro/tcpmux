all: tcpmux wsmux

tcpmux:
	$(CC) -std=c99 -Wall -o tcpmux tcpmux.c mux.c \
		../redismq/*.c \
		../sev/*.c \
		../hiredis/libhiredis.a \
		-I.. \
		-lev

wsmux:
	$(CC) -std=c99 -Wall -o wsmux wsmux.c mux.c \
		../redismq/*.c \
		../sev/*.c \
		../libws/*.c \
		../hiredis/libhiredis.a \
		-I.. \
		-lev

clean:
	rm -rf *.dSYM tcpmux wsmux

.PHONY: all tcpmux wsmux clean
