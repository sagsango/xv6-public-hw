.code16
.global _start
.data
msg: .byte 'h' 'e' 'l' 'l' 'o'
_start:
    mov 0, %ds # Make God, Happy!!

    # Printing Char
    mov $0x09, %ah
    mov $0x00, %bh
    mov $0x9, %bl
    mov $0x1, %cx
    mov msg(,1), %al
    int $0x10

# .org 512 # This will also work fine because 0x1fe = 512
.org 0x1fe
.word 0xAA55
