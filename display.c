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

//static struct {
//  struct spinlock lock;
//  int locking;
//} dis;

#define VGA_TEXT_MODE 		0x03
#define VGA_PAL_INDEX 		0x3C8
#define VGA_PAL_DATA 		0x3C9
#define VGA_DATA_READ_PORT  0x3C5
#define VGA_VIDEO_MEMORY_PHYSICAL 0xA0000

#define VGA_INDEX_PORT 		0x3C4 
#define VGA_GRAPHICS_MODE 	0x13
#define VGA_MISC_OUTPUT_REG	0x3C2
#define VGA_DATA_WRITE_PORT 0x3C8

// Function to set the display mode to VGA graphics mode (Mode 13h)
void vgaMode13()
{
	outb(VGA_INDEX_PORT, VGA_MISC_OUTPUT_REG); // Select VGA Misc Output Register
    char misc = inb(VGA_DATA_READ_PORT);
    misc |= VGA_GRAPHICS_MODE; // Set bit 4 to enable graphics mode
    outb(VGA_DATA_WRITE_PORT, misc);
	cprintf("vgaMode13()\n");
}

void vgaMode3()
{
	outb(VGA_INDEX_PORT, VGA_MISC_OUTPUT_REG); // Select VGA Misc Output Register
    char misc = inb(VGA_DATA_READ_PORT);
    misc |= VGA_TEXT_MODE; // Set bit 4 to enable graphics mode
    outb(VGA_DATA_WRITE_PORT, misc);
	cprintf("vgaMode3()\n");
}

// Function to modify a color entry in the palette
void vgaSetPalette(int index, int red, int green, int blue)
{
	// Ensure 'index' is within the valid range (0-255)
	 if (index < 0 || index > 255) {
        return; // Invalid index
    }

    // Write to the VGA Palette Index Register (0x3C8) to select the index
    outb(VGA_PAL_INDEX, index);

    // Write the RGB values for the color entry to the VGA Palette Data Register (0x3C9)
    outb(VGA_PAL_DATA, red);
    outb(VGA_PAL_DATA, green);
    outb(VGA_PAL_DATA, blue);

	cprintf("vgaSetPalette(index:%d, r:%d, g:%d, b:%d)\n", index, red, green, blue);
}

// Function to write a pixel to the display
void write_pixel(int offset, int color) {

    // Write the pixel color to VGA memory
    *(char *)(KERNBASE + VGA_VIDEO_MEMORY_PHYSICAL + offset) = color;
}

int displaywrite(struct file *f, char *buf, int n)
{
	int i;
	for (i = 0; i<n; ++i)
	{
		write_pixel(f->off, buf[i]);
		f->off += 1;
	}
	cprintf("displaywrite(struct file *f, char *buf, int n:%d)\n", n);
	return n;
}
int displayioctl(struct file *f, int param, int value)
{   int index, r, g, b;	
	switch(param)
	{
		case 1: 	/* VGA mode change */
			switch(value)
			{
				case 0x13: 	/* to 0x13 */
					vgaMode13();
					break;
				case 0x3: 	/* to 0x3 */
					vgaMode3();
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
	// initlock(&dis.lock, "display");
	// initlock(&input.lock, "input");

	devsw[DISPLAY].write = displaywrite;
  	//cons.locking = 1;

 	// ioapicenable(IRQ_KBD, 0)
}



