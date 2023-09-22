// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

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

#define DEFAULT_CONSOLE_COLOR 0x0700  // gray on black
#define SET_FD_COLOR 0
#define SET_GLOBAL_COLOR 1

static void consputc(int, int);

static int panicked = 0;
static int global_color = DEFAULT_CONSOLE_COLOR;

static struct {
  struct spinlock lock;
  int locking;
} cons;

static char digits[] = "0123456789abcdef";

  static void
print_x64(addr_t x, int color)
{
  int i;
  for (i = 0; i < (sizeof(addr_t) * 2); i++, x <<= 4)
    consputc(digits[x >> (sizeof(addr_t) * 8 - 4)], color);
}

  static void
print_x32(uint x, int color)
{
  int i;
  for (i = 0; i < (sizeof(uint) * 2); i++, x <<= 4)
   consputc(digits[x >> (sizeof(uint) * 8 - 4)], color);
}

  static void
print_d(int v, int color)
{
  char buf[16];
  int64 x = v;

  if (v < 0)
    x = -x;

  int i = 0;
  do {
    buf[i++] = digits[x % 10];
    x /= 10;
  } while(x != 0);

  if (v < 0)
    buf[i++] = '-';

  while (--i >= 0)
    consputc(buf[i], color);
}
//PAGEBREAK: 50

// Print to the console. only understands %d, %x, %p, %s.
  void
cprintf(char *fmt, ...)
{
  va_list ap;
  int i, c, locking;
  char *s;

  va_start(ap, fmt);

  locking = cons.locking;
  if (locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
    if (c != '%') {
      consputc(c, global_color);
      continue;
    }
    c = fmt[++i] & 0xff;
    if (c == 0)
      break;
    switch(c) {
    case 'd':
      print_d(va_arg(ap, int), global_color);
      break;
    case 'x':
      print_x32(va_arg(ap, uint), global_color);
      break;
    case 'p':
      print_x64(va_arg(ap, addr_t), global_color);
      break;
    case 's':
      if ((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      while (*s)
        consputc(*(s++), global_color);
      break;
    case '%':
      consputc('%', global_color);
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%', global_color);
      consputc(c, global_color);
      break;
    }
  }

  if (locking)
    release(&cons.lock);
}

__attribute__((noreturn))
  void
panic(char *s)
{
  int i;
  addr_t pcs[10];

  cli();
  cons.locking = 0;
  cprintf("cpu%d: panic: ", cpu->id);
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for (i=0; i<10; i++)
    cprintf(" %p\n", pcs[i]);
  panicked = 1; // freeze other CPU
  for (;;)
    hlt();
}

//PAGEBREAK: 50
#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory

  static void
cgaputc(int c, int color)
{
  int pos;

  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);

  if (c == '\n')
    pos += 80 - pos%80;
  else if (c == BACKSPACE) {
    if (pos > 0) --pos;
  } else {
	//if (color == 0) {
	//	color = DEFAULT_CONSOLE_COLOR;
	//}
    crt[pos++] = (c&0xff) | color;  // it used be gray on black
  }

  if ((pos/80) >= 24){  // Scroll up.
    memmove(crt, crt+80, sizeof(crt[0])*23*80);
    pos -= 80;
    memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
  }

  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);
  crt[pos] = ' ' | 0x0700;
}

/*
XXX:
   This is the first wrapper above the device drivers
   and all the other code is using this wrapper to
   print the chars on the console.
*/
  void
consputc(int c, int color)
{
  if (panicked) {
    cli();
    for(;;)
      hlt();
  }

  if (c == BACKSPACE) {
    uartputc('\b'); uartputc(' '); uartputc('\b');
  } else
    uartputc(c);
  cgaputc(c, color);
}

#define INPUT_BUF 128
struct {
  struct spinlock lock;
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} input;

#define C(x)  ((x)-'@')  // Control-x

  void
consoleintr(int (*getc)(void))
{
  int c;

  acquire(&input.lock);
  while((c = getc()) >= 0){
    switch(c){
    case C('Z'): // reboot
      lidt(0,0);
      break;
    case C('P'):  // Process listing.
      procdump();
      break;
    case C('U'):  // Kill line.
      while(input.e != input.w &&
          input.buf[(input.e-1) % INPUT_BUF] != '\n'){
        input.e--;
        consputc(BACKSPACE, 0);
      }
      break;
    case C('H'): case '\x7f':  // Backspace
      if (input.e != input.w) {
        input.e--;
        consputc(BACKSPACE, 0);
      }
      break;
    default:
      if (c != 0 && input.e-input.r < INPUT_BUF) {
        c = (c == '\r') ? '\n' : c;
        input.buf[input.e++ % INPUT_BUF] = c;
        consputc(c, global_color);
        if (c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF) {
          input.w = input.e;
          wakeup(&input.r);
        }
      }
      break;
    }
  }
  release(&input.lock);
}

int
consoleread(struct file *f, char *dst, int n)
{
  uint target;
  int c;

  target = n;
  acquire(&input.lock);
  while(n > 0){
    while(input.r == input.w){
      if (proc->killed) {
        release(&input.lock);
        ilock(f->ip);
        return -1;
      }
      sleep(&input.r, &input.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if (c == C('D')) {  // EOF
      if (n < target) {
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if (c == '\n')
      break;
  }
  release(&input.lock);

  return target - n;
}

int
consoleioctl(struct file *f, int param, int value)
{
  switch(param)
  {
	  case SET_FD_COLOR:
	 f->dev_payload = value * 16 * 16; /* Why not shift operator? Because it is more relateable to hexadecimal representation */
	 break;
	  case SET_GLOBAL_COLOR:
	 global_color = value * 16 * 16;
	 break;
	  default:
	 cprintf("Got unknown console ioctl request. %d = %d\n",param,value);
	 return -1;
  }
  return 0;
}

int
consolewrite(struct file *f, char *buf, int n)
{
  int i;

  acquire(&cons.lock);
  for(i = 0; i < n; i++)
    consputc(buf[i] & 0xff, (int)f->dev_payload);
  release(&cons.lock);

  return n;
}

  void
consoleinit(void)
{
  initlock(&cons.lock, "console");
  initlock(&input.lock, "input");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  ioapicenable(IRQ_KBD, 0);
}
