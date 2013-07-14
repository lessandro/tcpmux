all: tcpmux

tcpmux: redismq/redismq.a sev/sev.a *.c
	$(CC) -Wall -o tcpmux *.c redismq/redismq.a sev/sev.a -lev -lhiredis

redismq/redismq.a:
	$(MAKE) -C redismq static-lib

sev/sev.a:
	$(MAKE) -C sev static-lib

clean:
	rm -rf *.dSYM *.a *.o tcpmux
