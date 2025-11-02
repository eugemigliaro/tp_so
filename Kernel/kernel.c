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

static void process_that_prints_its_remaining_quantum(int argc, char **argv);
static void semaphore_worker(int argc, char **argv);
static sem_t demo_sem;

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
    
	_cli();
	
	mem_init();
	process_table_init();
	scheduler_init();
	sem_init(&demo_sem, "demo_sem", 1);

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

	/*
	print("Launching shell process...\n");
 
	char **argv_shell = mem_alloc(sizeof(char *));
	argv_shell[0] = "shell";
	pcb_t *shell_process = createProcess(1, argv_shell, 1, SCHEDULER_MAX_PRIORITY, 1, (void (*)(void))shellModuleAddress);
	if (shell_process != NULL) {
		scheduler_add_ready(shell_process);
	} else {
		print("Failed to create shell process\n");
	}
	*/
	/* print("Launching quantum printing process...\n");

	char **argv_quantum = mem_alloc(2 * sizeof(char *));
	argv_quantum[0] = "quantum";
	char *quantum_arg = mem_alloc(sizeof(char) * 6);
	strcpy(quantum_arg, "arg1");
	argv_quantum[1] = quantum_arg;
	pcb_t *quantum_printing_process = createProcess(2, argv_quantum, 1, SCHEDULER_MAX_PRIORITY, 1, (void (*)(void))process_that_prints_its_remaining_quantum);
	if (quantum_printing_process != NULL) {
		scheduler_add_ready(quantum_printing_process);
	} else {
		print("Failed to create quantum printing process\n");
	}

	print("Launching semaphore demo workers...\n");
	char *sem_worker_a_args[] = { "sem-A" };
	pcb_t *sem_worker_a = createProcess(1, sem_worker_a_args, 1, SCHEDULER_MIN_PRIORITY, 1, semaphore_worker);
	if (sem_worker_a != NULL) {
		scheduler_add_ready(sem_worker_a);
	} else {
		print("Failed to create semaphore worker A\n");
	}

	char *sem_worker_b_args[] = { "sem-B" };
	pcb_t *sem_worker_b = createProcess(1, sem_worker_b_args, 1, SCHEDULER_MIN_PRIORITY, 1, semaphore_worker);
	if (sem_worker_b != NULL) {
		scheduler_add_ready(sem_worker_b);
	} else {
		print("Failed to create semaphore worker B\n");
	} */

	_sti();

	while (1) {
		_hlt();
	}

	__builtin_unreachable();

	return 0;
}

static void process_that_prints_its_remaining_quantum(int argc, char **argv) {
	uint8_t counter = 0;
	while(1){
		counter = scheduler_current()->remaining_quantum;
		setTextColor(0x00FF6666);
		print("remaining quantum ticks: ");
		printDec(counter);
		print("\n");
		print("First arg value: ");
		print(argv[1]);
		print("\n");
		setTextColor(DEFAULT_TEXT_COLOR);
		sleep(1);
	}
}

static void semaphore_worker(int argc, char **argv) {
	const char *label = (argc > 0 && argv[0] != NULL) ? argv[0] : "sem";
	while (1) {
		sem_wait(&demo_sem);
		setTextColor(0x0099FF99);
		print("[SEM] ");
		print(label);
		print(" acquired semaphore\n");
		setTextColor(DEFAULT_TEXT_COLOR);
		sleep(1);
		print("[SEM] ");
		print(label);
		print(" releasing semaphore\n");
		setTextColor(DEFAULT_TEXT_COLOR);
		sem_post(&demo_sem);
		sleep(1);
	}
}
