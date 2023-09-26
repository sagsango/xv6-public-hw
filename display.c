#include <stdarg.h>
#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "vga.h"

#define VGA_VIDEO_MEMORY_PHYSICAL 0xA0000
#define TEXT_MODE_VIDIO_BUFFER_SIZE 64000
#define TEXT_MODE_VIDIO_BUFFER_ADDR 0xb8000

char vidio_buffer[TEXT_MODE_VIDIO_BUFFER_SIZE];
struct cursor cursor;

void write_pixel(int offset, uchar color) {

    *(uchar *)(KERNBASE + VGA_VIDEO_MEMORY_PHYSICAL + offset) = color;
}

int displaywrite(struct file *f, char *buf, int n)
{
	int i;
	for(i = 0; i < n; i+=1){
        write_pixel(f->off, buf[i]);
        f->off += 1;
    }

	return n;
}

void get_text_mode_vidio_buffer (char *vidio_buffer) {
    char* pmem = (char*)P2V(TEXT_MODE_VIDIO_BUFFER_ADDR);
    int i;
    for(i = 0; i < TEXT_MODE_VIDIO_BUFFER_SIZE; ++i)
        vidio_buffer[i] = pmem[i];
}

void restore_text_mode_vidio_buffer(char *vidio_buffer) {
    char* pmem = (char*)P2V(TEXT_MODE_VIDIO_BUFFER_ADDR);
    int i;
    for(i = 0; i < TEXT_MODE_VIDIO_BUFFER_SIZE; ++i)
        pmem[i] = vidio_buffer[i];
}

int displayioctl(struct file *f, int param, int value)
{   int index, r, g, b;	
	switch(param)
	{
		case 1: 	/* VGA mode change */
			switch(value)
			{
				case 0x13: 	/* to 0x13 */
					get_cursor(&cursor);
        			get_text_mode_vidio_buffer(vidio_buffer);
					vgaMode13();
					break;
				case 0x3: 	/* to 0x3 */
					vgaMode3();
					restore_cursor(&cursor);
					restore_text_mode_vidio_buffer(vidio_buffer);
					break;
				default:
					return -1;
			}
			break;
		case 2: 	/* setting palette color */
			b = (value >> 0x00) & 0xff;
			g = (value >> 0x08) & 0xff;
			r = (value >> 0x10) & 0xff;
			index = (value >> 0x18) & 0xff;
			vgaSetPalette(index, r, g, b);
			break;
		default:
			return -1;
	}
	return 0;
}

void
displayinit(void)
{

	/* TODO: Lock on resources like console.c */
	devsw[DISPLAY].write = displaywrite;
	devsw[DISPLAY].read = 0;

}
