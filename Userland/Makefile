include Makefile.inc

all: libc libsys shell

libc:
	cd libc; make all

libsys:
	cd libsys; make all

shell:
	cd Shell; make

clean:
	cd Shell; make clean
	cd libc; make clean
	cd libsys; make clean
	rm -rf *.bin *.elf *.o *.so *.a

.PHONY: libc libsys shell all clean
