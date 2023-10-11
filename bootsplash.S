.code16
.global start
start:
    # save the device id (%dl)
    push %dx

    # print the messagae, char by char in loop
    mov $0, %edi
    mov $len, %edx
    movb $0x0e, %ah
    movb $0, %bh
    movb $7, %bl
string_loop:
    movb message(,%edi,1), %al  # XXX:base_address(offset_address, index, size) = base_addre
    inc %edi
    int $0x10
    cmp %edi, %edx
    jne string_loop

    # Wait for keypress
    mov $0x00, %ah
    int $0x16

    # Trun on vidio mode
    mov $0x00, %ah
    mov $0x13, %al
    int $0x10

    # Read img in vidio buffer
    mov $0x42, %ah
    pop %dx # put device id in %dl
    #lea packet, %si
    mov $packet, %si
    int $0x13

stop:
    jmp stop

packet:
    .byte 0x10      # Size of packet
    .byte 0x00      # zero
    .word 0x7d      # (no of blocks to copy = 125 = 0x7d)
    # 32-bit field, in segment:offset format. Pay attention to byte order.
    .word 0x0000,0xA000     # (where to put = 0xA0000)
    .quad 0x1       # (starting block offset = 0x1)

message:
    .string "Welcome to xv6@UIC. Press any key to continue"
    len = . - message

.org 0x1fe
.word 0xAA55
