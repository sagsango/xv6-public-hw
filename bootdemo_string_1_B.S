.code16
.global _start
_start:
	mov 0, %ds
#mov 0, %es
	mov $0x13, %ah
	mov $0x03, %al # XXX: Check it you may want to change the mode
	               # XXX: Do not put 0 here, it gives bad ram accessed.
	mov $0x00, %bh
	mov $0x0f, %bl # 1st 4 bit = foreground clr, 2nd 4 bit = background
	mov len, %cx
	mov $0x0, %dh
	mov $0x0, %dl
# XXX: ES:BP = pointer to string
#mov %ds, %es # XXX: Why this is giving error?
#mov msg, %es:(%bp)  # XXX: Address Not Sure!!
	push %bp
	push msg
	mov %sp, %bp
	int $0x10

loop:
    jmp loop

msg:
	.string "Hello"
	len = . - msg

# .org 510 # This will also work fine because 0x1fe = 510
.org 0x1fe
.word 0xAA55
