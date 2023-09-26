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
#include "vgafont.h"

//#define VGA_TEXT_MODE 		0x03
#define VGA_PAL_INDEX 			0x3C8
#define VGA_PAL_DATA 			0x3C9
//#define VGA_DATA_READ_PORT  	0x3C5
#define VGA_VIDEO_MEMORY_PHYSICAL 0xA0000

#define VGA_INDEX_PORT 			0x3C4 
//#define VGA_GRAPHICS_MODE 	0x01 /*0x13*/
//#define VGA_GRAPHICS_MODE 	0x13

//#define VGA_MISC_OUTPUT_REG	0x3C2
//#define VGA_DATA_WRITE_PORT 0x3C8

#define VGA 0x3C0
/*
int vga256_24bit[256] = { 0x000000, 0x0000a8, 0x00a800, 0x00a8a8, 0xa80000, 0xa800a8, 0xa85400, 0xa8a8a8, 0x545454, 0x5454fc, 0x54fc54, 0x54fcfc, 0xfc5454, 0xfc54fc, 0xfcfc54, 0xfcfcfc, 0x000000, 0x141414, 0x202020, 0x2c2c2c, 0x383838, 0x444444, 0x505050, 0x606060, 0x707070, 0x808080, 0x909090, 0xa0a0a0, 0xb4b4b4, 0xc8c8c8, 0xe0e0e0, 0xfcfcfc, 0x0000fc, 0x4000fc, 0x7c00fc, 0xbc00fc, 0xfc00fc, 0xfc00bc, 0xfc007c, 0xfc0040, 0xfc0000, 0xfc4000, 0xfc7c00, 0xfcbc00, 0xfcfc00, 0xbcfc00, 0x7cfc00, 0x40fc00, 0x00fc00, 0x00fc40, 0x00fc7c, 0x00fcbc, 0x00fcfc, 0x00bcfc, 0x007cfc, 0x0040fc, 0x7c7cfc, 0x9c7cfc, 0xbc7cfc, 0xdc7cfc, 0xfc7cfc, 0xfc7cdc, 0xfc7cbc, 0xfc7c9c, 0xfc7c7c, 0xfc9c7c, 0xfcbc7c, 0xfcdc7c, 0xfcfc7c, 0xdcfc7c, 0xbcfc7c, 0x9cfc7c, 0x7cfc7c, 0x7cfc9c, 0x7cfcbc, 0x7cfcdc, 0x7cfcfc, 0x7cdcfc, 0x7cbcfc, 0x7c9cfc, 0xb4b4fc, 0xc4b4fc, 0xd8b4fc, 0xe8b4fc, 0xfcb4fc, 0xfcb4e8, 0xfcb4d8, 0xfcb4c4, 0xfcb4b4, 0xfcc4b4, 0xfcd8b4, 0xfce8b4, 0xfcfcb4, 0xe8fcb4, 0xd8fcb4, 0xc4fcb4, 0xb4fcb4, 0xb4fcc4, 0xb4fcd8, 0xb4fce8, 0xb4fcfc, 0xb4e8fc, 0xb4d8fc, 0xb4c4fc, 0x000070, 0x1c0070, 0x380070, 0x540070, 0x700070, 0x700054, 0x700038, 0x70001c, 0x700000, 0x701c00, 0x703800, 0x705400, 0x707000, 0x547000, 0x387000, 0x1c7000, 0x007000, 0x00701c, 0x007038, 0x007054, 0x007070, 0x005470, 0x003870, 0x001c70, 0x383870, 0x443870, 0x543870, 0x603870, 0x703870, 0x703860, 0x703854, 0x703844, 0x703838, 0x704438, 0x705438, 0x706038, 0x707038, 0x607038, 0x547038, 0x447038, 0x387038, 0x387044, 0x387054, 0x387060, 0x387070, 0x386070, 0x385470, 0x384470, 0x505070, 0x585070, 0x605070, 0x685070, 0x705070, 0x705068, 0x705060, 0x705058, 0x705050, 0x705850, 0x706050, 0x706850, 0x707050, 0x687050, 0x607050, 0x587050, 0x507050, 0x507058, 0x507060, 0x507068, 0x507070, 0x506870, 0x506070, 0x505870, 0x000040, 0x100040, 0x200040, 0x300040, 0x400040, 0x400030, 0x400020, 0x400010, 0x400000, 0x401000, 0x402000, 0x403000, 0x404000, 0x304000, 0x204000, 0x104000, 0x004000, 0x004010, 0x004020, 0x004030, 0x004040, 0x003040, 0x002040, 0x001040, 0x202040, 0x282040, 0x302040, 0x382040, 0x402040, 0x402038, 0x402030, 0x402028, 0x402020, 0x402820, 0x403020, 0x403820, 0x404020, 0x384020, 0x304020, 0x284020, 0x204020, 0x204028, 0x204030, 0x204038, 0x204040, 0x203840, 0x203040, 0x202840, 0x2c2c40, 0x302c40, 0x342c40, 0x3c2c40, 0x402c40, 0x402c3c, 0x402c34, 0x402c30, 0x402c2c, 0x40302c, 0x40342c, 0x403c2c, 0x40402c, 0x3c402c, 0x34402c, 0x30402c, 0x2c402c, 0x2c4030, 0x2c4034, 0x2c403c, 0x2c4040, 0x2c3c40, 0x2c3440, 0x2c3040, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000 };


void setdefaultVGApalette() {
  for(int index=0;index<256;index++) {
    int value = vga256_24bit[index];
    vgaSetPalette(index,
               (value>>18)&0x3f,
               (value>>10)&0x3f,
               (value>>2)&0x3f);

	return;
	//sleep(60);
  }
}
*/

void write3C2(unsigned char value) {
  outb(0x3C2, value);
}

void write3C4(unsigned char index, unsigned char value) {
  outb(0x3C4, index);
  outb(0x3C5, value);
}

void write3D4(unsigned char index, unsigned char value) {
  outb(0x3D4, index);
  outb(0x3D5, value);
}

void write3CE(unsigned char index, unsigned char value) {
  outb(0x3CE, index);
  outb(0x3CF, value);
}

void write3C0(unsigned char index, unsigned char value) {
// get VGA port into index mode
  inb(VGA+0x1A);
  outb(0x3C0, index);
  outb(0x3C0, value);
}


// Function to set the display mode to VGA graphics mode (Mode 13h)
void vgaMode13()
{

	/* We can not generate this trap in user mode :-)
	__asm__("mov $0x00, %ah\n\t"
			"mov $0x13, %al\n\t"
			"int $0x10");
	return;
	*/

      write3C2(0x63);

      write3C4(0x00, 0x03);
      write3C4(0x01, 0x01);
      write3C4(0x02, 0x0F);
      write3C4(0x03, 0x00);
      write3C4(0x04, 0x0E);

      // unlock VGA register access
      // the top bit says to disable write access to some VGA index registers
      // see http://www.osdever.net/FreeVGA/vga/crtcreg.htm#11
      write3D4(0x11, 0x0E);


      write3D4(0x00, 0x5F);
      write3D4(0x01, 0x4F);
      write3D4(0x02, 0x50);
      write3D4(0x03, 0x82);
      write3D4(0x04, 0x54);
      write3D4(0x05, 0x80);
      write3D4(0x06, 0xBF);
      write3D4(0x07, 0x1F);
      write3D4(0x08, 0x00);
      write3D4(0x09, 0x41);
      write3D4(0x10, 0x9C);
      // index 0x11, see above
      write3D4(0x12, 0x8F);
      write3D4(0x13, 0x28);
      write3D4(0x14, 0x40);
      write3D4(0x15, 0x96);
      write3D4(0x16, 0xB9);
      write3D4(0x17, 0xA3);
      write3D4(0x18, 0xFF);

      write3CE(0x00, 0x00);
      write3CE(0x01, 0x00);
      write3CE(0x02, 0x00);
      write3CE(0x03, 0x00);
      write3CE(0x04, 0x00);
      write3CE(0x05, 0x40);
      write3CE(0x06, 0x05);
      write3CE(0x07, 0x0F);
      write3CE(0x08, 0xFF);

      // switch to mode 0x13
      write3C0(0x00, 0x00);
      write3C0(0x01, 0x01);
      write3C0(0x02, 0x02);
      write3C0(0x03, 0x03);
      write3C0(0x04, 0x04);
      write3C0(0x05, 0x05);
      write3C0(0x06, 0x06);
      write3C0(0x07, 0x07);
      write3C0(0x08, 0x08);
      write3C0(0x09, 0x09);
      write3C0(0x0A, 0x0A);
      write3C0(0x0B, 0x0B);
      write3C0(0x0C, 0x0C);
      write3C0(0x0D, 0x0D);
      write3C0(0x0E, 0x0E);
      write3C0(0x0F, 0x0F);
      write3C0(0x10, 0x41);
      write3C0(0x11, 0x00);
      write3C0(0x12, 0x0F);
      write3C0(0x13, 0x00);
      write3C0(0x14, 0x00);

      // enable display??
      inb(VGA+0x1A);
      outb(0x3C0, 0x20);

      //setdefaultVGApalette();

	return;
	/*
	outb(VGA_INDEX_PORT, VGA_MISC_OUTPUT_REG); // Select VGA Misc Output Register
    char misc = inb(VGA_DATA_READ_PORT);
    misc |= VGA_GRAPHICS_MODE; // Set bit 4 to enable graphics mode
    outb(VGA_DATA_WRITE_PORT, misc);
	cprintf("vgaMode13(1)\n");
	*/
}

void vgaMode3()
{

      write3C2(0x67);

      write3C4(0x00, 0x03);
      write3C4(0x01, 0x00);
      write3C4(0x02, 0x03);
      write3C4(0x03, 0x00);
      write3C4(0x04, 0x02);

      // unlock VGA register access
      // the top bit says to disable write access to some VGA index registers
      // see http://www.osdever.net/FreeVGA/vga/crtcreg.htm#11
      write3D4(0x11, 0x0E);

      write3D4(0x00, 0x5F);
      write3D4(0x01, 0x4F);
      write3D4(0x02, 0x50);
      write3D4(0x03, 0x82);
      write3D4(0x04, 0x55);
      write3D4(0x05, 0x81);
      write3D4(0x06, 0xBF);
      write3D4(0x07, 0x1F);


      write3D4(0x08, 0x00);
      write3D4(0x09, 0x4F);
      write3D4(0x0A, 0x20);
      write3D4(0x0B, 0x0e);
      write3D4(0x0C, 0x00);
      write3D4(0x0D, 0x00);
      write3D4(0x0E, 0x01);
      write3D4(0x0F, 0xe0);

      write3D4(0x10, 0x9C);
      write3D4(0x11, 0x8E);
      write3D4(0x12, 0x8F);
      write3D4(0x13, 0x28);
      write3D4(0x14, 0x1F);
      write3D4(0x15, 0x96);
      write3D4(0x16, 0xB9);
      write3D4(0x17, 0xA3);
      write3D4(0x18, 0xFF);

      write3CE(0x00, 0x00);
      write3CE(0x01, 0x00);
      write3CE(0x02, 0x00);
      write3CE(0x03, 0x00);
      write3CE(0x04, 0x00);
      write3CE(0x05, 0x10);
      write3CE(0x06, 0x0E);
      write3CE(0x07, 0x00);
      write3CE(0x08, 0xFF);

      inb(VGA+0x1A);

      write3C0(0x00, 0x00);
      write3C0(0x01, 0x01);
      write3C0(0x02, 0x02);
      write3C0(0x03, 0x03);
      write3C0(0x04, 0x04);
      write3C0(0x05, 0x05);
      write3C0(0x06, 0x14);
      write3C0(0x07, 0x07);
      write3C0(0x08, 0x38);
      write3C0(0x09, 0x39);
      write3C0(0x0A, 0x3A);
      write3C0(0x0B, 0x3B);
      write3C0(0x0C, 0x3C);
      write3C0(0x0D, 0x3D);
      write3C0(0x0E, 0x3E);
      write3C0(0x0F, 0x3F);
      write3C0(0x10, 0x0C);
      write3C0(0x11, 0x00);
      write3C0(0x12, 0x0F);
      write3C0(0x13, 0x08);
      write3C0(0x14, 0x00);


      write3C4(0x00, 0x01); // seq reset
      write3C4(0x02, 0x04); // image plane 2
      write3C4(0x04, 0x07); // disable odd/even in sequencer
      write3C4(0x00, 0x03); // seq reset

      // graphics registers
      write3CE(0x04, 0x02); // read select plane 2
      write3CE(0x05, 0x00); // odd/even disabled
      write3CE(0x06, 0x00); // memory map select A0000h-BFFFFh

      for(int i=0;i<4096;i+=16)
        for(int j=0;j<16;j++)
          ((char*)KERNBASE+0xa0000)[2*i+j] = vgafont16[i+j];

      write3C4(0x00, 0x01); // seq reset
      write3C4(0x02, 0x03); // image planes 0 and 1
      write3C4(0x03, 0x00); // character sets 0
      write3C4(0x04, 0x02); // plain 64 kb memory layout, with odd/even for text
      write3C4(0x00, 0x03); // seq reset

      write3CE(0x02, 0x00); // no color compare
      write3CE(0x03, 0x00); // no rotate
      write3CE(0x04, 0x00); // read select plane 0
      write3CE(0x05, 0x10); // odd/even enable
      write3CE(0x06, 0x0E); // memory map select: 0xb8000

      inb(VGA+0x1A);
      outb(0x3C0, 0x20);

	return;

	/*
	outb(VGA_INDEX_PORT, VGA_MISC_OUTPUT_REG); // Select VGA Misc Output Register
    char misc = inb(VGA_DATA_READ_PORT);
    misc |= VGA_TEXT_MODE; // Set bit 4 to enable graphics mode
    outb(VGA_DATA_WRITE_PORT, misc);
	cprintf("vgaMode3()\n");
	*/
}

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

#define TEXT_MODE_VIDIO_BUFFER_SIZE 64000
#define TEXT_MODE_VIDIO_BUFFER_ADDR 0xb8000
char vidio_buffer[TEXT_MODE_VIDIO_BUFFER_SIZE];
int cursor_position;

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
					get_cursor_position(&cursor_position);
        			get_text_mode_vidio_buffer(vidio_buffer);
					vgaMode13();
					break;
				case 0x3: 	/* to 0x3 */
					vgaMode3();
					restore_cursor_position(cursor_position); // todo: fix: mode switch will show cursor in prev row upon exit
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
	// initlock(&dis.lock, "display");
	// initlock(&input.lock, "input");

	devsw[DISPLAY].write = displaywrite;
	devsw[DISPLAY].read = 0;
  	//cons.locking = 1;

 	// ioapicenable(IRQ_KBD, 0)
}



