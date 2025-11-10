GLOBAL _divzero
GLOBAL _invalidopcode

_divzero:

    push rbp
    mov rbp, rsp
    push rbx

    mov rax, 10
    xor rbx, rbx

    div rbx

    pop rbx
    mov rsp, rbp
    pop rbp
    ret

_invalidopcode:

    push rbp
    mov rbp, rsp

    ud2

    mov rsp, rbp
    pop rbp
    ret


