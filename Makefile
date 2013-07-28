all:
	$(MAKE) -C hiredis static
	$(MAKE) -C src

clean:
	$(MAKE) -C hiredis clean
	$(MAKE) -C src clean
