#include "types.h"
#include "defs.h"
#include "x86.h"

static uint16_t serial_io_base = 0;

void
pciserial_init(uint32 io)
{
    serial_io_base = io;

    // Init 16550 UART
    outb(io + 1, 0x00);    // Disable interrupts
    outb(io + 3, 0x80);    // Enable DLAB
    outb(io + 0, 0x03);    // Baud divisor low  (38400)
    outb(io + 1, 0x00);    // Baud divisor high
    outb(io + 3, 0x03);    // 8 bits, no parity, one stop
    outb(io + 2, 0xC7);    // Enable FIFO, clear them
    outb(io + 4, 0x0B);    // IRQs enabled, RTS/DSR set

    cprintf("PCI-serial initialized at I/O 0x%x\n", io);


    /* TODO: Need device output backend */
    pciserial_putc('h');
    pciserial_putc('e');
    pciserial_putc('l');
    pciserial_putc('l');
    pciserial_putc('o');
}

void
pciserial_putc(int c)
{
    if (serial_io_base == 0)
        return; // not present

    // Wait for empty FIFO
    while ((inb(serial_io_base + 5) & 0x20) == 0)
        ;

    outb(serial_io_base, c);
}

