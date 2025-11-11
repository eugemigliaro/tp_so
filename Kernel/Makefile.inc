GCC=x86_64-linux-gnu-gcc
LD=x86_64-linux-gnu-ld
AR=x86_64-linux-gnu-ar
OBJCOPY=x86_64-linux-gnu-objcopy
ASM=nasm

GCCFLAGS=-g -m64 -fno-exceptions -fno-asynchronous-unwind-tables -mno-mmx -mno-sse -mno-sse2 -fno-builtin-malloc -fno-builtin-free -fno-builtin-realloc -mno-red-zone -Wall -ffreestanding -nostdlib -fno-common -std=c99 $(EXTRA_WARNINGS)
ARFLAGS=rvs
ASMFLAGS=-g -felf64
LDFLAGS=--warn-common -z max-page-size=0x1000
