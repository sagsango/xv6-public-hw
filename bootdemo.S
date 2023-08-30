.code16
.global _start
_start:
    mov $0xe, %ah    # function instruction location
    mov $'H', %al    # ASCII of char, which have to be printed
    movb $0, %bh     # foreground color of the char
    movb $7, %bl     # background color of the char
    int $0x10        # interrupt number awaken
loop:
    jmp loop         # just print it forever

.org 0x1fe   		 # .org 510 # this will work fine, it is just setting the offset for writing        
.word 0xAA55         # write a word 0xAA55 at given offset
