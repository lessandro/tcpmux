all: tcpmux

tcpmux: redismq/redismq.a *.c
	$(CC) -Wall -o tcpmux *.c redismq/redismq.a -lev -lhiredis

redismq/redismq.a:
	$(MAKE) -C redismq static-lib

clean:
	rm -rf *.dSYM *.a *.o tcpmux
