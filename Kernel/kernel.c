// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stdint.h>
#include <string.h>
#include <lib.h>
#include <moduleLoader.h>
#include <video.h>
#include <idtLoader.h>
#include <fonts.h>
#include <syscallDispatcher.h>
#include <sound.h>
#include <memoryManager.h>
#include <scheduler.h>
#include <process.h>
#include <interrupts.h>
#include <time.h>
#include <sem.h>
#include <pipes.h>

extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

static void * const shellModuleAddress = (void *)0x400000;

typedef int (*EntryPoint)();

void clearBSS(void * bssAddress, uint64_t bssSize){
	memset(bssAddress, 0, bssSize);
}

void * getStackBase() {
	return (void*)(
		(uint64_t)&endOfKernel
		+ PageSize * 8
		- sizeof(uint64_t)
	);
}

void * initializeKernelBinary(){
	void * moduleAddresses[] = {
		shellModuleAddress,
	};

	loadModules(&endOfKernelBinary, moduleAddresses);

	clearBSS(&bss, &endOfKernel - &bss);

	return getStackBase();
}

int main(){	
	load_idt();
    
	_cli();
	
	mem_init();
	process_table_init();
	scheduler_init();
	
	init_pipes();

	setFontSize(2);
	clear();

	print("Launching init process...\n");

	if (add_first_process() < 0) {
		print("Failed to create init process\n");
		while (1) {
			_hlt();
		}
	}

	_sti();

	while (1) {
		_hlt();
	}

	__builtin_unreachable();

	return 0;
}
