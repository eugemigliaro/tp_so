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

static void demo_process_alpha(void);
static void demo_process_beta(void);

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

static void demo_process_alpha(void) {
	uint64_t counter = 0;
	while (1) {
		setTextColor(0x00FF6666);
		print("[A] tick ");
		printDec(counter++);
		print("\n");
		setTextColor(DEFAULT_TEXT_COLOR);
		sleepTicks(SECONDS_TO_TICKS / 4);
	}
}

static void demo_process_beta(void) {
	uint64_t counter = 0;
	while (1) {
		setTextColor(0x0066CCFF);
		print("[B] tick ");
		printDec(counter++);
		print("\n");
		setTextColor(DEFAULT_TEXT_COLOR);
		sleepTicks(SECONDS_TO_TICKS / 2);
	}
}

int main(){	
	load_idt();

	mem_init();
	process_table_init();
	scheduler_init();

	setFontSize(2);
	clear();
	print("Kernel scheduler demo\n");

	pcb_t *proc_a = createProcess("demoA", 0, 1, demo_process_alpha);
	if (proc_a != NULL) {
		scheduler_add_ready(proc_a);
	} else {
		print("Failed to create process A\n");
	}

	pcb_t *proc_b = createProcess("demoB", 0, 1, demo_process_beta);
	if (proc_b != NULL) {
		scheduler_add_ready(proc_b);
	} else {
		print("Failed to create process B\n");
	}

	picMasterMask(TIMER_PIC_MASTER);
	picSlaveMask(NO_INTERRUPTS);
	_sti();

	while (1) {
		_hlt();
	}

	__builtin_unreachable();

	return 0;
}
