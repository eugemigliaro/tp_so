#ifndef _LIBC_SYSCALLS_H_
#define _LIBC_SYSCALLS_H_

#include <stdint.h>
#include <sys.h>

// Linux syscall prototypes
int32_t sys_write(int64_t fd, const void * buf, int64_t count);
int32_t sys_read(int64_t fd, void * buf, int64_t count);

// Custom syscall prototypes
/* 0x80000000 */
int32_t sys_start_beep(uint32_t nFrequence);
/* 0x80000001 */
int32_t sys_stop_beep(void);
/* 0x80000002 */
int32_t sys_fonts_text_color(uint32_t color);
/* 0x80000003 */
int32_t sys_fonts_background_color(uint32_t color);
/* 0x80000007 */
int32_t sys_fonts_decrease_size(void);
/* 0x80000008 */
int32_t sys_fonts_increase_size(void);
/* 0x80000009 */
int32_t sys_fonts_set_size(uint8_t size);
/* 0x8000000A */
int32_t sys_clear_screen(void);
/* 0x8000000B */
int32_t sys_clear_screen_character(void);

// Date syscall prototypes
/* 0x80000010 */
int32_t sys_hour(int * hour);
/* 0x80000011 */
int32_t sys_minute(int * minute);
/* 0x80000012 */
int32_t sys_second(int * second);

int32_t sys_circle(int color, long long int topleftX, long long int topLefyY, long long int diameter);

int32_t sys_rectangle(int color, long long int width_pixels, long long int height_pixels, long long int initial_pos_x, long long int initial_pos_y);

int32_t sys_fill_video_memory(uint32_t hexColor);
/* 0x80000024 */
int32_t sys_mem_status_print(void);

void *sys_mem_alloc(uint64_t size);
int32_t sys_mem_free(void *ptr);

int64_t sys_sem_open(const char *name, uint32_t initial_count, uint8_t create_if_missing);
int32_t sys_sem_close(void *sem);
int32_t sys_sem_wait(void *sem);
int32_t sys_sem_post(void *sem);
/* 0x80000124 */
int32_t sys_sem_set_value(void *sem, uint32_t new_value);

int32_t sys_process_create(void (*entry_point)(int argc, char **argv), int argc, char **argv, uint8_t priority, uint8_t foreground);
int32_t sys_process_exit(int32_t status);
int32_t sys_process_get_pid(void);
int32_t sys_process_list(void);
int32_t sys_process_kill(uint64_t pid);
int32_t sys_process_set_priority(uint64_t pid, uint8_t priority);
int32_t sys_process_block(uint64_t pid);
int32_t sys_process_unblock(uint64_t pid);
int32_t sys_process_yield(void);
int32_t sys_process_wait_pid(uint64_t pid);
int32_t sys_process_wait_children(void);
int32_t sys_process_give_foreground(uint64_t pid);
int32_t sys_process_get_foreground(void);

int32_t sys_exec(int32_t (*fnPtr)(void));

int32_t sys_register_key(uint8_t scancode, void (*fn)(enum REGISTERABLE_KEYS scancode));

int32_t sys_window_width(void);

int32_t sys_window_height(void);

int32_t sys_sleep_milis(uint32_t milis);

int32_t sys_get_register_snapshot(int64_t * registers);

// Pipes and FD target syscalls
/* 0x80000130 */
int32_t sys_open_pipe(void);
/* 0x80000131 */
int32_t sys_set_fd_targets(uint64_t read_target, uint64_t write_target, uint64_t error_target);
/* 0x80000132 */
int32_t sys_clear_pipe(uint64_t pipe_id);

#endif
