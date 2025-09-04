.code16
.text
.global mystart
.global get_ip

mystart:
    ljmp $0, $.setcs
.setcs:
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $__stack_top, %sp
    cld
    call main

get_ip:
    pop %ax
    push %ax
    ret

