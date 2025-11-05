#include <fonts.h>
#include <interrupts.h>
#include <syscallDispatcher.h>
#include <process.h>
#include <pipes.h>
#include <fd.h>
#include <strings.h>
#include <lib.h>

static void print_err(const char *string);
static void print_err_dec(uint64_t value);
static void print_err_hex(uint64_t value);

const static char * register_names[] = {
	"rax", "rbx", "rcx", "rdx", "rbp", "rdi", "rsi", "r8 ", "r9 ", "r10", "r11", "r12", "r13", "r14", "r15", "rsp", "rip", "rflags"
};

const static int registers_amount = sizeof(register_names) / sizeof(*register_names);

#define ZERO_EXCEPTION_ID 0
#define INVALID_OPCODE_ID 6

static void zero_division(uint64_t * registers, int errorCode);
static void invalid_opcode(uint64_t * registers, int errorCode);

void printExceptionData(uint64_t * registers, int errorCode);

void exceptionDispatcher(int exception, uint64_t * registers) {
	clear();
	switch(exception) {
		case ZERO_EXCEPTION_ID:
			return zero_division(registers, exception);
		case INVALID_OPCODE_ID:
			return invalid_opcode(registers, exception);
		default:
			return ; // returns to the asm exceptionHandler which will return to the shell
	}
}

static void zero_division(uint64_t * registers, int errorCode) {
	setTextColor(0x00FF0000);
	setFontSize(3); print_err("Division exception\n"); setFontSize(2);
	print_err("Arqui Screen of Death\n");
	printExceptionData(registers, errorCode);
}

static void invalid_opcode(uint64_t * registers, int errorCode) {
	setTextColor(0x00FF6600);
	setFontSize(3); print_err("Invalid Opcode Exception\n"); setFontSize(2);
	print_err("Arqui screen of death\n");
	printExceptionData(registers, errorCode);
}


void printExceptionData(uint64_t * registers, int errorCode) {
	print_err("Exception (# ");
	print_err_dec(errorCode);
	print_err(") triggered\n");
	print_err("Current registers values are\n");
	
	for (int i = 0; i < registers_amount; i++) {
		print_err(register_names[i]);
		print_err(": ");
		print_err_hex((uint64_t) registers[i]);
		print_err("\n");
	}

	setTextColor(0x00FFFFFF);

	print_err("Press r to go back to Shell");

	uint8_t a = 0;

	picMasterMask(KEYBOARD_PIC_MASTER);
	picSlaveMask(NO_INTERRUPTS);

	process_t *current = process_lookup((uint32_t)get_pid());
	if (current != NULL) {
		uint8_t stdin_id = current->fd_targets[READ_FD];
		do {
			if (read_pipe(stdin_id, &a, 1) != 1) {
				print_err("\nError reading from keyboard input\n");
				break;
			}
		} while (a != 'r');
	}

	clear();
	
	picMasterMask(KEYBOARD_PIC_MASTER & TIMER_PIC_MASTER);
	picSlaveMask(NO_INTERRUPTS);
	
	if (current != NULL) {
		process_exit(current);
	}
	
	_force_scheduler_interrupt();
	
	// Failsafe: should never reach here, but if we do, yield forever
	while(1) {
		process_yield();
	}
}

static void print_err(const char *string) {
	if (string == NULL) {
		return;
	}
	printToFd(STDERR, string, strlen(string));
}

static void print_err_dec(uint64_t value) {
	char buffer[32];
	uint32_t length = uint_to_base(value, buffer, 10);
	buffer[length] = '\0';
	print_err(buffer);
}

static void print_err_hex(uint64_t value) {
	char buffer[32];
	uint32_t length = uint_to_base(value, buffer, 16);
	buffer[length] = '\0';
	print_err(buffer);
}
