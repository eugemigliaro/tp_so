include ../Makefile.inc

MODULE=shell.bin
SYMBOL_MODULE=shell.elf
LD_SCRIPT=shellModule.ld
DEBUG_LD_SCRIPT=shellModuleDebug.ld
SOURCES=$(wildcard [^_]*.c)
LIBS=-l:libtests.a -l:libc.a -l:libsys.a

all: $(MODULE) $(SYMBOL_MODULE)

$(MODULE): $(SOURCES)
	$(MAKE) -C ../tests
	$(GCC) $(GCCFLAGS) -T $(LD_SCRIPT) _loader.c -L../ $(SOURCES) $(LIBS) -o ../$(MODULE)

$(SYMBOL_MODULE): $(SOURCES)
	$(MAKE) -C ../tests
	$(GCC) $(GCCFLAGS) -g -no-pie -T $(DEBUG_LD_SCRIPT) _loader.c -L../ $(SOURCES) $(LIBS) -o ../$(SYMBOL_MODULE)

clean:
	rm -rf *.o
	$(MAKE) -C ../tests clean

.PHONY: all clean print
