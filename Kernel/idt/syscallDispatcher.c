#include <syscallDispatcher.h>
#include <stddef.h>
#include <sound.h>
#include <keyboard.h>
#include <fonts.h>
#include <lib.h>
#include <video.h>
#include <time.h>
#include <process.h>
#include <scheduler.h>
#include <memoryManager.h>
#include <queueADT.h>
#include <sem.h>
#include <fd.h>
#include <pipes.h>
#include <interrupts.h>
#include <memoryManager.h>

extern int64_t register_snapshot[18];
extern int64_t register_snapshot_taken;

// @todo Note: Technically.. registers on the stack are modifiable (since its a struct pointer, not struct). 
int64_t syscallDispatcher(Registers * registers) {
	switch(registers->rax){
		case 3: return sys_read(registers->rdi, (signed char *) registers->rsi, registers->rdx);
		// Note: Register parameters are 64-bit
		case 4: return sys_write(registers->rdi, (char *) registers->rsi, registers->rdx);
		
		case 0x80000000: return sys_start_beep(registers->rdi);
		case 0x80000001: return sys_stop_beep();
		case 0x80000002: return sys_fonts_text_color(registers->rdi);
		case 0x80000003: return sys_fonts_background_color(registers->rdi);
		case 0x80000004: /* Reserved for sys_set_italics */
		case 0x80000005: /* Reserved for sys_set_bold */
		case 0x80000006: /* Reserved for sys_set_underline */
			return -1;
		case 0x80000007: return sys_fonts_decrease_size();
		case 0x80000008: return sys_fonts_increase_size();
		case 0x80000009: return sys_fonts_set_size((uint8_t) registers->rdi);
		case 0x8000000A: return sys_clear_screen();
		case 0x8000000B: return sys_clear_screen_character();

		case 0x80000010: return sys_hour((int *) registers->rdi);
		case 0x80000011: return sys_minute((int *) registers->rdi);
		case 0x80000012: return sys_second((int *) registers->rdi);

		case 0x80000019: return sys_circle(registers->rdi, registers->rsi, registers->rdx, registers->rcx);
		case 0x80000020: return sys_rectangle(registers->rdi, registers->rsi, registers->rdx, registers->rcx, registers->r8);
		case 0x80000021: return sys_fill_video_memory(registers->rdi);
		case 0x80000022: return (int64_t) sys_mem_alloc(registers->rdi);
		case 0x80000023: return sys_mem_free((void *) registers->rdi);
		case 0x80000024: return sys_mem_status_print();

		case 0x800000A0: return sys_exec((int (*)(void)) registers->rdi);

		case 0x800000B0: return sys_register_key((uint8_t) registers->rdi, (SpecialKeyHandler) registers->rsi);

		case 0x800000C0: return sys_window_width();
		case 0x800000C1: return sys_window_height();

		case 0x800000D0: return sys_sleep_milis(registers->rdi);

		case 0x800000E0: return sys_get_register_snapshot((int64_t *) registers->rdi);

	case 0x80000120: return sys_sem_open((const char *) registers->rdi, (uint32_t) registers->rsi, (uint8_t) registers->rdx);
	case 0x80000121: return sys_sem_close((sem_t *) registers->rdi);
	case 0x80000122: return sys_sem_wait((sem_t *) registers->rdi);
	case 0x80000123: return sys_sem_post((sem_t *) registers->rdi);
	case 0x80000124: return sys_sem_set_value((sem_t *) registers->rdi, (uint32_t)registers->rsi);

	case 0x80000130: return sys_open_pipe();
	case 0x80000131: return sys_set_fd_targets((uint64_t)registers->rdi, (uint64_t)registers->rsi, (uint64_t)registers->rdx);

	case 0x80000100: return sys_process_create(
			(void (*)(int, char **)) registers->rdi,
			(int) registers->rsi,
			(char **) registers->rdx,
			(uint8_t) registers->rcx,
			(uint8_t) registers->r8
		);
		case 0x80000101: return sys_process_exit((int32_t) registers->rdi);
		case 0x80000102: return sys_process_get_pid();
		case 0x80000103: return sys_process_list();
		case 0x80000104: return sys_process_kill((uint32_t) registers->rdi);
		case 0x80000105: return sys_process_set_priority((uint32_t) registers->rdi, (uint8_t) registers->rsi);
		case 0x80000106: return sys_process_block((uint32_t) registers->rdi);
		case 0x80000107: return sys_process_unblock((uint32_t) registers->rdi);
		case 0x80000108: return sys_process_yield();
		case 0x80000109: return sys_process_wait_pid((uint32_t) registers->rdi);
		case 0x8000010A: return sys_process_wait_children();
		case 0x8000010B: return sys_process_give_foreground(registers->rdi);
		case 0x8000010C: return sys_process_get_foreground();
		
		default:
            return 0;
	}
}

// ==================================================================
// Linux syscalls
// ==================================================================

int32_t sys_write(int32_t fd, char * __user_buf, int32_t count) {
    return write(fd, (const uint8_t *) __user_buf, count);
}

int32_t sys_read(int32_t fd, signed char * __user_buf, int32_t count) {
    return read(fd, (uint8_t *) __user_buf, count);
}

// ==================================================================
// Custom system calls
// ==================================================================

int32_t sys_start_beep(uint32_t nFrequence) {
	play_sound(nFrequence);
	return 0;
}

int32_t sys_stop_beep(void) {
	setSpeaker(SPEAKER_OFF);
	return 0;
}

int32_t sys_fonts_text_color(uint32_t color) {
	setTextColor(color);
	return 0;
}

int32_t sys_fonts_background_color(uint32_t color) {
	setBackgroundColor(color);
	return 0;
}

int32_t sys_fonts_decrease_size(void) {
	return decreaseFontSize();
}

int32_t sys_fonts_increase_size(void) {
	return increaseFontSize();
}

int32_t sys_fonts_set_size(uint8_t size) {
	return setFontSize(size);
}

int32_t sys_clear_screen(void) {
	clear();
	return 0;
}

int32_t sys_clear_screen_character(void) {
	clearPreviousCharacter();
	return 0;
}

uint16_t sys_window_width(void) {
	return getWindowWidth();
}

uint16_t sys_window_height(void) {
	return getWindowHeight();
}

// ==================================================================
// Date system calls
// ==================================================================

int32_t sys_hour(int * hour) {
	*hour = getHour();
	return 0;
}

int32_t sys_minute(int * minute) {
	*minute = getMinute();
	return 0;
}

int32_t sys_second(int * second) {
	*second = getSecond();
	return 0;
}

// ==================================================================
// Draw system calls
// ==================================================================


int32_t sys_circle(uint32_t hexColor, uint64_t topLeftX, uint64_t topLeftY, uint64_t diameter){
	drawCircle(hexColor, topLeftX, topLeftY, diameter);
	return 0;
}

int32_t sys_rectangle(uint32_t color, uint64_t width_pixels, uint64_t height_pixels, uint64_t initial_pos_x, uint64_t initial_pos_y){
	drawRectangle(color, width_pixels, height_pixels, initial_pos_x, initial_pos_y);
	return 0;
}

int32_t sys_fill_video_memory(uint32_t hexColor) {
	fillVideoMemory(hexColor);
	return 0;
}

void *sys_mem_alloc(uint64_t size) {
	return mem_alloc((size_t) size);
}

int32_t sys_mem_free(void *ptr) {
	mem_free(ptr);
	return 0;
}

int32_t sys_mem_status_print(void) {
	return print_mem_status();
}

int64_t sys_sem_open(const char *name, uint32_t initial_count, uint8_t create_if_missing) {
	if (name == NULL) {
		return -1;
	}

	sem_t *sem = sem_find(name);
	if (sem != NULL) {
		return (int64_t)sem;
	}

	if (!create_if_missing) {
		return -1;
	}

	sem = sem_create();
	if (sem == NULL) {
		return -1;
	}

	sem_init(sem, name, initial_count);
	if (sem->name == NULL || sem->waiting_processes == NULL) {
		sem_destroy(sem);
		mem_free(sem);
		return -1;
	}

	return (int64_t)sem;
}

int32_t sys_sem_close(sem_t *sem) {
	if (sem == NULL) {
		return -1;
	}

	sem_destroy(sem);
	mem_free(sem);
	return 0;
}

int32_t sys_sem_wait(sem_t *sem) {
	if (sem == NULL) {
		return -1;
	}
	return sem_wait(sem);
}

int32_t sys_sem_post(sem_t *sem) {
	if (sem == NULL) {
		return -1;
	}
	return sem_post(sem);
}

int32_t sys_sem_set_value(sem_t *sem, uint32_t new_value) {
	return sem_set_value(sem, new_value);
}

// ==================================================================
// Custom exec system call
// ==================================================================

int32_t sys_exec(int32_t (*fnPtr)(void)) {
	clear();

	uint8_t fontSize = getFontSize(); 					// preserve font size
	uint32_t text_color = getTextColor();				// preserve text color
	uint32_t background_color = getBackgroundColor();	// preserve background color
	
	SpecialKeyHandler map[ F12_KEY - ESCAPE_KEY + 1 ] = {0};
	clearKeyFnMapNonKernel(map); // avoid """processes/threads/apps""" registering keys across each other over time. reset the map every time
	
	int32_t aux = fnPtr();

	restoreKeyFnMapNonKernel(map);
	setFontSize(fontSize);
	setTextColor(text_color);
	setBackgroundColor(background_color);

	clear();
	return aux;
}

// ==================================================================
// Custom keyboard system calls
// ==================================================================

int32_t sys_register_key(uint8_t scancode, SpecialKeyHandler fn){
	registerSpecialKey(scancode, fn, 0);
	return 0;
}

// ==================================================================
// Sleep system calls
// ==================================================================
int32_t sys_sleep_milis(uint32_t milis) {
	sleepTicks( (milis * SECONDS_TO_TICKS) / 1000 );
	return 0;
}

// ==================================================================
// Register snapshot system calls
// ==================================================================
int32_t sys_get_register_snapshot(int64_t * registers) {
	if (register_snapshot_taken == 0) return 0;  

	uint8_t i = 0;
	
	while (i < 18) {
		*(registers++) = register_snapshot[i++];
	}

	return 1;
}

// ==================================================================
// Context Switch system calls
// ==================================================================

int32_t sys_process_get_pid(void) {
	return get_pid();
}

int32_t sys_process_list(void) {
	return print_process_list();
}

int32_t sys_process_set_priority(uint32_t pid, uint8_t priority) {
	return scheduler_set_process_priority(pid, priority);
}

int32_t sys_process_create(void (*entry_point)(int argc, char **argv), int argc, char **argv, uint8_t priority, uint8_t foreground) {
	int32_t caller_pid = get_pid();
	uint32_t parent_pid = (caller_pid < 0) ? 0 : (uint32_t)caller_pid;
	process_t *process = createProcess(argc, argv, parent_pid, priority, foreground, entry_point);
	if (process == NULL) {
		return -1;
	}
	scheduler_add_ready(process);
	return (int32_t)process->pid;
}

int32_t sys_process_exit(int32_t status) {
	process_t *process = scheduler_current();

	if (process == NULL) {
		return status;
	}

	if (!process_exit(process)) {
        status = -1;
    }

	return status;
}


int32_t sys_process_block(uint32_t pid) {
    process_t *process = process_lookup(pid);
    if (process == NULL) {
        return -1;
    }
    if (!process_block(process)) {
        return -1;
    }
    _force_scheduler_interrupt();
    return 0; 
}

int32_t sys_process_unblock(uint32_t pid) {
    process_t *process = process_lookup(pid);
    if (process == NULL) {
        return -1;
    }
    return process_unblock(process) ? 0 : -1;
}

int32_t sys_process_kill(uint32_t pid) {
    process_t *process = process_lookup(pid);
    if (process == NULL) {
        return -1;
    }

    if (!process_exit(process)) {
        return -1;
    }

    return 0;
}

int32_t sys_process_yield(void) {
	process_yield();
	return 0;
}

int32_t sys_process_wait_pid(uint32_t pid) {
	return process_wait_pid(pid);
}

int32_t sys_process_wait_children(void) {
	return process_wait_children();
}

int32_t sys_process_give_foreground(uint64_t target_pid) {
	if (target_pid < PROCESS_FIRST_PID || target_pid >= PROCESS_FIRST_PID + PROCESS_MAX_PROCESSES) {
		return -1;
	}

	return give_foreground_to((uint32_t)target_pid); 
}

int32_t sys_process_get_foreground(void) {
	return get_foreground_process_pid();
}

// ==================================================================
// Pipes and FD target system calls
// ==================================================================

int32_t sys_open_pipe(void) {
    return open_pipe();
}

int32_t sys_set_fd_targets(uint64_t read_target, uint64_t write_target, uint64_t error_target) {
    return set_fd_targets((uint8_t)read_target, (uint8_t)write_target, (uint8_t)error_target);
}
