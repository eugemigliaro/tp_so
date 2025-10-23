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

// extern uint8_t text;
// extern uint8_t rodata;
// extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

static void * const shellModuleAddress = (void *)0x400000;
static void * const snakeModuleAddress = (void *)0x500000;

typedef int (*EntryPoint)();

void clearBSS(void * bssAddress, uint64_t bssSize){
	memset(bssAddress, 0, bssSize);
}

void * getStackBase() {
	return (void*)(
		(uint64_t)&endOfKernel
		+ PageSize * 8				//The size of the stack itself, 32KiB
		- sizeof(uint64_t)			//Begin at the top of the stack
	);
}

void * initializeKernelBinary(){
	void * moduleAddresses[] = {
		shellModuleAddress,
		snakeModuleAddress,
	};

	loadModules(&endOfKernelBinary, moduleAddresses);

	clearBSS(&bss, &endOfKernel - &bss);

	return getStackBase();
}

int main(){	
	load_idt();

	mem_init();
	process_table_init();
	scheduler_init();

	setFontSize(2);
	clear();
	print("Launching shell process...\n");
 
	pcb_t *shell_process = createProcess("shell", 0, SCHEDULER_MAX_PRIORITY, (void (*)(void))shellModuleAddress);
	if (shell_process != NULL) {
		scheduler_add_ready(shell_process);
	} else {
		print("Failed to create shell process\n");
	}

	_sti();

	while (1) {
		_hlt();
	}

	__builtin_unreachable();

	return 0;
}
