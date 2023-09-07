.code16
.global _start
_start:
    mov $0xe, %ah
    mov $'H', %al
    movb $0, %bh
    movb $7, %bl
    int $0x10
loop:
    jmp loop

# .org 512 # This will also work fine because 0x1fe = 512
.org 0x1fe
.word 0xAA55
