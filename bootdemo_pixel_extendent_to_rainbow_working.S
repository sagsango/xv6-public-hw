.code16
.global _start
_start:
	mov $0x00, %ah
	mov $0x13, %al
	int $0x10
	movw $0xA000, %ax  # How many 0s should it have?
	movw %ax, %ds  # In i8086 you cannot move imm directly to a *s register. (movw %0xA000, %ds)
    mov $0xf, %bl
fill_clr:
	inc %bl
	movb %bl, %al # A white pixel
	jmp fill_pixel
	movw $0, %di   # The first pixel at the top-left corner of the screen
fill_pixel:
	movb %al, %ds:(%di) # After this instruction you can see the tiny white dot there. (maximize window to make it more visible)
	inc %di
	jmp fill_clr 
#int $0x10
loop:
    jmp loop

# .org 510 # This will also work fine because 0x1fe = 510
.org 0x1fe
.word 0xAA55
