#include <sys.h>
#include <syscalls.h>

void startBeep(uint32_t nFrequence) {
    sys_start_beep(nFrequence);
}

void stopBeep(void) {
    sys_stop_beep();
}

void setTextColor(uint32_t color) {
    sys_fonts_text_color(color);
}

void setBackgroundColor(uint32_t color) {
    sys_fonts_background_color(color);
}

uint8_t increaseFontSize(void) {
    return sys_fonts_increase_size();
}

uint8_t decreaseFontSize(void) {
    return sys_fonts_decrease_size();
}

uint8_t setFontSize(uint8_t size) {
    return sys_fonts_set_size(size);
}

void getDate(int * hour, int * minute, int * second) {
    sys_hour(hour);
    sys_minute(minute);
    sys_second(second);
}

void clearScreen(void) {
    sys_clear_screen();
}

void clearScreenCharacter(void) {
    sys_clear_screen_character();
}

void drawCircle(uint32_t color, long long int topleftX, long long int topLefyY, long long int diameter) {
    sys_circle(color, topleftX, topLefyY, diameter);
}

void drawRectangle(uint32_t color, long long int width_pixels, long long int height_pixels, long long int initial_pos_x, long long int initial_pos_y){
    sys_rectangle(color, width_pixels, height_pixels, initial_pos_x, initial_pos_y);
}

void fillVideoMemory(uint32_t hexColor) {
    sys_fill_video_memory(hexColor);
}

int32_t exec(int32_t (*fnPtr)(void)) {
    return sys_exec(fnPtr);
}

void registerKey(enum REGISTERABLE_KEYS scancode, void (*fn)(enum REGISTERABLE_KEYS scancode)) {
    sys_register_key(scancode, fn);
}


int getWindowWidth(void) {
    return sys_window_width();
}

int getWindowHeight(void) {
    return sys_window_height();
}

void sleep(uint32_t miliseconds) {
    sys_sleep_milis(miliseconds);
}

int32_t getRegisterSnapshot(int64_t * registers) {
    return sys_get_register_snapshot(registers);
}

int32_t processCreate(void (*entry_point)(int argc, char **argv), int argc, char **argv, uint8_t priority, uint8_t foreground) {
    return sys_process_create(entry_point, argc, argv, priority, foreground);
}

int32_t processExit(int32_t status) {
    return sys_process_exit(status);
}

int32_t processGetPid(void) {
    return sys_process_get_pid();
}

int32_t processList(void) {
    return sys_process_list();
}

int32_t processKill(uint64_t pid) {
    return sys_process_kill(pid);
}

int32_t processSetPriority(uint64_t pid, uint8_t priority) {
    return sys_process_set_priority(pid, priority);
}

int32_t processBlock(uint64_t pid) {
    return sys_process_block(pid);
}

int32_t processUnblock(uint64_t pid) {
    return sys_process_unblock(pid);
}

int32_t processYield(void) {
    return sys_process_yield();
}

int32_t processWaitPid(uint64_t pid) {
    return sys_process_wait_pid(pid);
}

int32_t processWaitChildren(void) {
    return sys_process_wait_children();
}

int32_t processGiveForeground(uint64_t pid) {
    return sys_process_give_foreground(pid);
}

int32_t processGetForeground(void) {
    return sys_process_get_foreground();
}

int32_t openPipe(void) {
    return sys_open_pipe();
}

int32_t setFdTargets(uint64_t read_target, uint64_t write_target, uint64_t error_target) {
    return sys_set_fd_targets(read_target, write_target, error_target);
}

void *semOpen(const char *name, uint32_t initial_count, uint8_t create_if_missing) {
    return (void *)sys_sem_open(name, initial_count, create_if_missing);
}

int32_t semClose(void *sem) {
    return sys_sem_close(sem);
}

int32_t semWait(void *sem) {
    return sys_sem_wait(sem);
}

int32_t semPost(void *sem) {
    return sys_sem_post(sem);
}

int32_t printMemStatus(void) {
    return sys_mem_status_print();
}
