
MEMORY_MANAGER ?= mymalloc

all:  bootloader kernel userland image

bootloader:
	cd Bootloader; make all

kernel:
	cd Kernel; make all MEMORY_MANAGER=$(MEMORY_MANAGER)

userland:
	cd Userland; make all

image: kernel bootloader userland
	cd Image; make all

clean:
	cd Bootloader; make clean
	cd Image; make clean
	cd Kernel; make clean
	cd Userland; make clean
	rm -f *.zip

buddy:
	$(MAKE) all MEMORY_MANAGER=buddy

.PHONY: bootloader image collections kernel userland all clean buddy
