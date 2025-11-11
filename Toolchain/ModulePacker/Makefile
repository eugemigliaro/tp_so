MP=mp.bin
SOURCES=$(wildcard *.c)

all: $(MP)

$(MP): $(SOURCES)
	gcc $(EXTRA_WARNINGS) $(SOURCES) -o $(MP)

clean:
	rm -rf $(MP)

.PHONY: all clean
