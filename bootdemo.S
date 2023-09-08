.code16
.global _start
_start:
	mov %dl, %al
	cmpb $0x80, %al
    je loop # XXX: Commnet this line and you will see the output
	mov $'H', %al
    mov $0xe, %ah
#mov $'H', %al
    movb $0, %bh
    movb $7, %bl
    int $0x10
loop:
    jmp loop

.org 0x1fe
.word 0xAA55


#48 80 41
#01001000
#10000000
#01000001
#10001000
#88
