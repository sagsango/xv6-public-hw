.code16
.global _start
_start:
	mov $0x00, %ah
	mov $0x13, %al
	int $0x10
# Now fill all the pixels from  location 0xA0000
	mov $0x42, %ah
	mov $0x80, %dl
	# mov packet, (%ds:%si)
	mov packet, %si
    int $0x13

#Φ ls -l cover.raw
#-rw-r--r--@ 1 sagarsingh  staff  64000 Sep  7 21:48 cover.raw
# 6400 = 125 * 512 = 0x7d * 512

packet:
	.byte 0x18 # Size of packet 
	.byte 0x00 
	.word 0x007d
	.word 0xffff, 0xffff # XXX: Make Sure
	.word 0x0000, 0x0000, 0x0000, 0x0001
	.word 0x0000, 0x0000, 0x000A, 0x0000 # XXX: Make Sure

.org 0x1fe
.word 0xAA55
