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
#include <sem.h>

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
		case 0x8000000B: return sys_clear_input_buffer();

		case 0x80000010: return sys_hour((int *) registers->rdi);
		case 0x80000011: return sys_minute((int *) registers->rdi);
		case 0x80000012: return sys_second((int *) registers->rdi);

		case 0x80000019: return sys_circle(registers->rdi, registers->rsi, registers->rdx, registers->rcx);
		case 0x80000020: return sys_rectangle(registers->rdi, registers->rsi, registers->rdx, registers->rcx, registers->r8);
		case 0x80000021: return sys_fill_video_memory(registers->rdi);
		case 0x80000022: return (int64_t) sys_mem_alloc(registers->rdi);
		case 0x80000023: return sys_mem_free((void *) registers->rdi);

		case 0x800000A0: return sys_exec((int (*)(void)) registers->rdi);

		case 0x800000B0: return sys_register_key((uint8_t) registers->rdi, (SpecialKeyHandler) registers->rsi);

		case 0x800000C0: return sys_window_width();
		case 0x800000C1: return sys_window_height();

		case 0x800000D0: return sys_sleep_milis(registers->rdi);

		case 0x800000E0: return sys_get_register_snapshot((int64_t *) registers->rdi);

	case 0x800000F0: return sys_get_character_without_display();

	case 0x80000120: return sys_sem_open((const char *) registers->rdi, (uint32_t) registers->rsi, (uint8_t) registers->rdx);
	case 0x80000121: return sys_sem_close((sem_t *) registers->rdi);
	case 0x80000122: return sys_sem_wait((sem_t *) registers->rdi);
	case 0x80000123: return sys_sem_post((sem_t *) registers->rdi);

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
		case 0x80000104: return sys_process_kill((uint64_t) registers->rdi);
		case 0x80000105: return sys_process_set_priority((uint64_t) registers->rdi, (uint8_t) registers->rsi);
		case 0x80000106: return sys_process_block((uint64_t) registers->rdi);
		case 0x80000107: return sys_process_unblock((uint64_t) registers->rdi);
		case 0x80000108: return sys_process_yield();
		case 0x80000109: return sys_process_wait_pid((uint64_t) registers->rdi);
		case 0x8000010A: return sys_process_wait_children();
		
		default:
            return 0;
	}
}

// ==================================================================
// Linux syscalls
// ==================================================================

int32_t sys_write(int32_t fd, char * __user_buf, int32_t count) {
    return printToFd(fd, __user_buf, count);
}

int32_t sys_read(int32_t fd, signed char * __user_buf, int32_t count) {
	int32_t i;
	int8_t c;
	for(i = 0; i < count && (c = getKeyboardCharacter(AWAIT_RETURN_KEY | SHOW_BUFFER_WHILE_TYPING)) != EOF; i++){
		*(__user_buf + i) = c;
	}
    return i;
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

int32_t sys_clear_input_buffer(void) {
	while(clearBuffer() != 0);
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

	sem = mem_alloc(sizeof(sem_t));
	if (sem == NULL) {
		return -1;
	}

	memset(sem, 0, sizeof(sem_t));
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

int32_t sys_get_character_without_display(void) {
	return getKeyboardCharacter(0);
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

int32_t sys_process_set_priority(uint64_t pid, uint8_t priority) {
	return scheduler_set_process_priority(pid, priority);
}

int32_t sys_process_create(void (*entry_point)(int argc, char **argv), int argc, char **argv, uint8_t priority, uint8_t foreground) {
	pcb_t *pcb = createProcess(argc, argv, (uint64_t)get_pid(), priority, foreground, entry_point);
	if (pcb == NULL) {
		return -1;
	}
	return (int32_t)pcb->pid;
}

int32_t sys_process_exit(int32_t status) {
	pcb_t *pcb = scheduler_current();

	if (pcb == NULL) {
		return status;
	}

	pcb->state = PROCESS_STATE_TERMINATED;

	if (!process_exit(pcb)) {
        return -1;
    }

	// Hasta que no estén implementados los semáforos queda así
	// La idea sería que se marque como TERMINATED, se borre de las colas, se borre el current y se fuerce el tick,
	// y que luego, cuando lo capture el padre, lo borre de la pcb_table y libere la memoria.

	_force_scheduler_interrupt();

	return status;
}


int32_t sys_process_block(uint64_t pid) {
    pcb_t *pcb = process_lookup(pid);
    if (pcb == NULL) {
        return -1;
    }
    if (!process_block(pcb)) {
        return -1;
    }
    _force_scheduler_interrupt();
    return 0;
}

int32_t sys_process_unblock(uint64_t pid) {
    pcb_t *pcb = process_lookup(pid);
    if (pcb == NULL) {
        return -1;
    }
    return process_unblock(pcb) ? 0 : -1;
}

int32_t sys_process_kill(uint64_t pid) {
    pcb_t *pcb = process_lookup(pid);
    if (pcb == NULL) {
        return -1;
    }

    process_state_t previous_state = pcb->state;
    pcb->state = PROCESS_STATE_TERMINATED;

    if (!process_exit(pcb)) {
        return -1;
    }

	//TODO: Ver si conviene forzar el tick acá o en process_exit
    if (previous_state == PROCESS_STATE_RUNNING) {
        _force_scheduler_interrupt();
    }

    return 0;
}

int32_t sys_process_yield(void) {
	pcb_t *pcb = scheduler_current();
	if (pcb != NULL) {
		pcb->state = PROCESS_STATE_YIELD;
	}
	_force_scheduler_interrupt();
	return 0;
}

int32_t sys_process_wait_pid(uint64_t pid) {
	return process_wait_pid(pid);
}

int32_t sys_process_wait_children(void) {
	return process_wait_children();
}
