include ../Makefile.inc

MODULE=shell.bin
SOURCES=$(wildcard [^_]*.c)
LIBS=-l:libtests.a -l:libc.a -l:libsys.a

all: $(MODULE)

$(MODULE): $(SOURCES)
	$(MAKE) -C ../tests
	$(GCC) $(GCCFLAGS) -T shellModule.ld _loader.c -L../ $(SOURCES) $(LIBS) -o ../$(MODULE)

clean:
	rm -rf *.o
	$(MAKE) -C ../tests clean

.PHONY: all clean print
