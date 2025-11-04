#ifndef _SYSCALL_DISPATCHER_H_
#define _SYSCALL_DISPATCHER_H_

#include <stdint.h>
#include <keyboard.h>
#include <sem.h>

typedef struct {
    int64_t r15;
	int64_t r14;
	int64_t r13;
	int64_t r12;
	int64_t r11;
	int64_t r10;
	int64_t r9;
	int64_t r8;
	int64_t rsi;
	int64_t rdi;
	int64_t rbp;
	int64_t rdx;
	int64_t rcx;
	int64_t rbx;
	int64_t rax;
	int64_t rip;
} Registers;

int64_t syscallDispatcher(Registers * registers);

// Linux syscall prototypes
int32_t sys_write(int32_t fd, char * __user_buf, int32_t count);
int32_t sys_read(int32_t fd, signed char * __user_buf, int32_t count);

// Custom syscall prototypes
int32_t sys_start_beep(uint32_t nFrequence);
int32_t sys_stop_beep(void);
int32_t sys_fonts_text_color(uint32_t color);
int32_t sys_fonts_background_color(uint32_t color);
int32_t sys_fonts_decrease_size(void);
int32_t sys_fonts_increase_size(void);
int32_t sys_fonts_set_size(uint8_t size);
int32_t sys_clear_screen(void);
int32_t sys_clear_screen_character(void);
uint16_t sys_window_width(void);
uint16_t sys_window_height(void);
int32_t sys_mem_status_print(void);

// Date syscall prototypes
int32_t sys_hour(int * hour);
int32_t sys_minute(int * minute);
int32_t sys_second(int * second);


int32_t sys_circle(uint32_t hexColor, uint64_t topLeftX, uint64_t topLeftY, uint64_t diameter);
// Draw rectangle syscall prototype
int32_t sys_rectangle(uint32_t color, uint64_t width_pixels, uint64_t height_pixels, uint64_t initial_pos_x, uint64_t initial_pos_y);
int32_t sys_fill_video_memory(uint32_t hexColor);

void *sys_mem_alloc(uint64_t size);
int32_t sys_mem_free(void *ptr);

int64_t sys_sem_open(const char *name, uint32_t initial_count, uint8_t create_if_missing);
int32_t sys_sem_close(sem_t *sem);
int32_t sys_sem_wait(sem_t *sem);
int32_t sys_sem_post(sem_t *sem);
int32_t sys_sem_set_value(sem_t *sem, uint32_t new_value);

// Custom exec syscall prototype
int32_t sys_exec(int32_t (*fnPtr)(void));

// Custom keyboard syscall prototypes
int32_t sys_register_key(uint8_t scancode, SpecialKeyHandler fn);

// System sleep
int32_t sys_sleep_milis(uint32_t milis);

// Register snapshot
int32_t sys_get_register_snapshot(int64_t * registers);

// Process management syscalls
int32_t sys_process_create(void (*entry_point)(int argc, char **argv), int argc, char **argv, uint8_t priority, uint8_t foreground);
int32_t sys_process_exit(int32_t status);
int32_t sys_process_get_pid(void);
int32_t sys_process_list(void);
int32_t sys_process_kill(uint32_t pid);
int32_t sys_process_set_priority(uint32_t pid, uint8_t priority);
int32_t sys_process_block(uint32_t pid);
int32_t sys_process_unblock(uint32_t pid);
int32_t sys_process_yield(void);
int32_t sys_process_wait_pid(uint32_t pid);
int32_t sys_process_wait_children(void);
int32_t sys_process_give_foreground(uint64_t target_pid);

// Pipes and FD target syscalls
int32_t sys_open_pipe(void);
int32_t sys_set_fd_targets(uint64_t read_target, uint64_t write_target, uint64_t error_target);

#endif
