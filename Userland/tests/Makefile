include ../Makefile.inc

MODULE=libtests.a
SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)

all: $(MODULE)

$(MODULE): $(OBJECTS)
	$(AR) rcs ../$@ $^

%.o : %.c
	$(GCC) -c $(GCCFLAGS) $<

clean:
	rm -f *.o
	rm -f ../$(MODULE)

.PHONY: all clean
