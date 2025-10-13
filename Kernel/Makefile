include Makefile.inc

KERNEL=kernel.bin
SOURCES=$(wildcard *.c ./drivers/*.c ./idt/*.c)
SOURCES_ASM=$(wildcard asm/*.asm)
HOT_OBJECTS=./drivers/video.o fonts.o # Compiled with -O3
OBJECTS=$(SOURCES:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
LOADERSRC=loader.asm

LOADEROBJECT=$(LOADERSRC:.asm=.o)
STATICLIBS=
MEMORY_MANAGER ?= mymalloc

ifeq ($(MEMORY_MANAGER),buddy)
SOURCES += ./memory/buddy.c
GCCFLAGS += -DMEMORY_MANAGER_BUDDY
else ifeq ($(MEMORY_MANAGER),mymalloc)
SOURCES += ./memory/myMalloc.c
GCCFLAGS += -DMEMORY_MANAGER_MYMALLOC
else
$(error Unknown MEMORY_MANAGER "$(MEMORY_MANAGER)")
endif

all: $(KERNEL)

$(KERNEL): $(LOADEROBJECT) $(OBJECTS) $(STATICLIBS) $(OBJECTS_ASM)
	$(LD) $(LDFLAGS) -T kernel.ld -o $(KERNEL) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)

$(HOT_OBJECTS) : %.o: %.c
	$(GCC) -O3 $(GCCFLAGS) -I./include -c $< -o $@

$(filter-out $(HOT_OBJECTS),$(OBJECTS)) : %.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -I./font_assets -c $< -o $@

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

# font.o:
# 	objcopy -O elf64-x86-64 -B i386 -I binary ./font_assets/Solarize.12x29.psf font.o

$(LOADEROBJECT):
	$(ASM) $(ASMFLAGS) $(LOADERSRC) -o $(LOADEROBJECT)

clean:
	rm -rf */*.o *.o *.bin

.PHONY: all clean
