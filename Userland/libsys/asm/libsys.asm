GLOBAL sys_start_beep
GLOBAL sys_stop_beep

GLOBAL sys_fonts_text_color
GLOBAL sys_fonts_background_color
GLOBAL sys_fonts_decrease_size
GLOBAL sys_fonts_increase_size
GLOBAL sys_fonts_set_size
GLOBAL sys_clear_screen
GLOBAL sys_clear_input_buffer

GLOBAL sys_hour
GLOBAL sys_minute
GLOBAL sys_second
GLOBAL sys_sleep_milis

GLOBAL sys_circle
GLOBAL sys_rectangle
GLOBAL sys_fill_video_memory

GLOBAL sys_mem_alloc
GLOBAL sys_mem_free

GLOBAL sys_sem_open
GLOBAL sys_sem_close
GLOBAL sys_sem_wait
GLOBAL sys_sem_post

GLOBAL sys_process_create
GLOBAL sys_process_exit
GLOBAL sys_process_get_pid
GLOBAL sys_process_list
GLOBAL sys_process_kill
GLOBAL sys_process_set_priority
GLOBAL sys_process_block
GLOBAL sys_process_unblock
GLOBAL sys_process_yield
GLOBAL sys_process_wait_pid
GLOBAL sys_process_wait_children

GLOBAL sys_exec

GLOBAL sys_register_key

GLOBAL sys_window_width
GLOBAL sys_window_height

GLOBAL sys_get_register_snapshot

GLOBAL sys_get_character_without_display

section .text

%macro sys_int80 1
    push rbp
    mov rbp, rsp
    mov rax, %1
    int 0x80
    mov rsp, rbp
    pop rbp
    ret
%endmacro

sys_start_beep: sys_int80 0x80000000
sys_stop_beep: sys_int80 0x80000001

sys_fonts_text_color: sys_int80 0x80000002
sys_fonts_background_color: sys_int80 0x80000003



sys_fonts_decrease_size: sys_int80 0x80000007
sys_fonts_increase_size: sys_int80 0x80000008
sys_fonts_set_size: sys_int80 0x80000009
sys_clear_screen: sys_int80 0x8000000A
sys_clear_input_buffer: sys_int80 0x8000000B


sys_hour: sys_int80 0x80000010
sys_minute: sys_int80 0x80000011
sys_second: sys_int80 0x80000012

sys_circle: sys_int80 0x80000019
sys_rectangle: sys_int80 0x80000020
sys_fill_video_memory: sys_int80 0x80000021

sys_mem_alloc: sys_int80 0x80000022
sys_mem_free: sys_int80 0x80000023

sys_sem_open: sys_int80 0x80000120
sys_sem_close: sys_int80 0x80000121
sys_sem_wait: sys_int80 0x80000122
sys_sem_post: sys_int80 0x80000123

sys_process_create: sys_int80 0x80000100
sys_process_exit: sys_int80 0x80000101
sys_process_get_pid: sys_int80 0x80000102
sys_process_list: sys_int80 0x80000103
sys_process_kill: sys_int80 0x80000104
sys_process_set_priority: sys_int80 0x80000105
sys_process_block: sys_int80 0x80000106
sys_process_unblock: sys_int80 0x80000107
sys_process_yield: sys_int80 0x80000108
sys_process_wait_pid: sys_int80 0x80000109
sys_process_wait_children: sys_int80 0x8000010A

sys_exec: sys_int80 0x800000A0

sys_register_key: sys_int80 0x800000B0

sys_window_width: sys_int80 0x800000C0
sys_window_height: sys_int80 0x800000C1

sys_sleep_milis: sys_int80 0x800000D0

sys_get_register_snapshot: sys_int80 0x800000E0

sys_get_character_without_display: sys_int80 0x800000F0
