GLOBAL cpuVendor
GLOBAL getKeyboardBuffer

GLOBAL getSecond
GLOBAL getMinute
GLOBAL getHour

GLOBAL setPITMode
GLOBAL setPITFrequency
GLOBAL setSpeaker

GLOBAL getRegisterSnapshot
GLOBAL stackInit

GLOBAL semLock
GLOBAL semUnlock

GLOBAL _force_scheduler_interrupt

EXTERN register_snapshot
EXTERN register_snapshot_taken

section .text

getKeyboardBuffer:
	push rbp
	mov rbp, rsp

	in al, 60h

	mov rsp, rbp
	pop rbp

	ret


getSecond:
	push rbp
	mov rbp, rsp

	mov al, 0
	out 70h, al
	in al, 71h

	mov rsp, rbp
	pop rbp

	ret


getMinute:
	push rbp
	mov rbp, rsp

	mov al, 2
	out 70h, al
	in al, 71h

	mov rsp, rbp
	pop rbp

	ret


getHour:
	push rbp
	mov rbp, rsp

	mov al, 4
	out 70h, al
	in al, 71h

	mov rsp, rbp
	pop rbp

	ret


setPITMode:
	push rbp
	mov rbp, rsp
	
	mov rax, rdi
	out 0x43, al

	mov rsp, rbp
	pop rbp

	ret


setPITFrequency:
	push rbp
	mov rbp, rsp

	mov rax, rdi
	out 0x42, al
	mov al, ah
	out 0x42, al

	mov rsp, rbp
	pop rbp
	ret


setSpeaker:
	push rbp
	mov rbp, rsp

	cmp rdi, 0
	je .off

	.on:
	in al, 0x61
	or al, 0x03
	out 0x61, al
	jmp .end

	.off:
	in al, 0x61
	and al, 0xFC
	out 0x61, al

	.end:
	mov rsp, rbp
	pop rbp
	ret

stackInit:

	;of interest:
	;rdi -> rsp
	;rci -> rip

	push rbp
	mov rbp, rsp
	
	mov rsp, rdi	;placed at the stack's top
	push 0x0 ; SS
	push rdi ; RSP
	push 0x202 ;RFLAGS
	push 0x8  ; CS
	push rsi ; RIP
	mov rdi, rdx
	mov rsi, rcx
	
	;GPR
	;ABI calling convention
	;rdi -> argc
	;rsi -> argv
	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	lea rax, [rsp]

	mov rsp, rbp
	pop rbp
	
	ret

semLock:
    mov al, 1
    xchg al, BYTE [rdi]
    cmp al, 0
    jne semLock
    ret

semUnlock:
    mov BYTE [rdi], 0
    ret

_force_scheduler_interrupt:
	int 0x20
	ret