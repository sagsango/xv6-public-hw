// pci-testdev.c — driver for QEMU PCI test device 1b36:0005 (no IRQ)

#include "types.h"
#include "x86.h"
#include "defs.h"
#include "memlayout.h"
#include <stdint.h>

// From QEMU docs:
//
// typedef struct PCITestDevHdr {
//     uint8_t  test;       // write-only, select test number
//     uint8_t  width_type; // read-only, 1/2/4 => byte/word/long
//     uint8_t  pad0[2];
//     uint32_t offset;     // offset within this BAR for test access
//     uint32_t data;       // value to write for the test
//     uint32_t count;      // number of writes detected
//     uint8_t  name[];     // 0-terminated ASCII
// } PCITestDevHdr;

#define uint8 unsigned char
#define uint32 unsigned int
#define uint64 unsigned long
#define uint16 uint16_t

typedef struct __attribute__((packed)) {
  volatile uint8  test;
  volatile uint8  width_type;
  volatile uint8  pad0[2];
  volatile uint32 offset;
  volatile uint32 data;
  volatile uint32 count;
  volatile uint8  name[32];   // small fixed buffer for debug printing
} PCITestDevHdr;

static volatile uint8 *pt_bar0_base = 0;       // raw BAR0 virtual base
static volatile PCITestDevHdr *pt_hdr = 0;
static ushort pt_iobase = 0;
static int pt_present = 0;


static void
pcitestdev_run_some_tests(void)
{
  if (!pt_hdr)
    return;

  cprintf("[pt] scanning tests on BAR0...\n");

  for (int t = 0; t < 16; t++) {
    pt_hdr->test = t;

    uint8 w = pt_hdr->width_type;
    if (w != 1 && w != 2 && w != 4) {
      cprintf("[pt]  test %d: unsupported (width_type=%d), stopping\n", t, w);
      break;
    }

    uint32 off   = pt_hdr->offset;
    uint32 data  = pt_hdr->data;
    uint32 before = pt_hdr->count;

    // Name is at hdr->name[]; may not be null-terminated in our 32-byte view
    char nm[33];
    int i;
    for (i = 0; i < 32; i++) {
      uint8 c = pt_hdr->name[i];
      nm[i] = (c ? c : '\0');
      if (!c) break;
    }
    nm[32] = '\0';

    cprintf("[pt]  test %d: width=%d offset=0x%x data=0x%x count=%d name=\"%s\"\n",
            t, w, off, data, before, nm);

    // Compute target address inside BAR0
    volatile uint8 *addr = pt_bar0_base + off;

    // Perform ONE write as specified
    if (w == 1) {
      *(volatile uint8*)addr = (uint8)data;
    } else if (w == 2) {
      *(volatile uint16*)addr = (uint16)data;
    } else if (w == 4) {
      *(volatile uint32*)addr = (uint32)data;
    }

    uint32 after = pt_hdr->count;
    cprintf("[pt]      count: %d -> %d\n", before, after);
  }
}

// Called from pci.c once BAR0/BAR1 have been read and PCI command set.
void
pcitestdev_init(uint bar0_pa, uint bar1_port)
{
  cprintf("[pt] init: BAR0 PA=%p, BAR1 IO=0x%x\n", bar0_pa, bar1_port);

  // Only handle the BAR0 == MMIO case for now.
  if (bar0_pa == 0) {
    cprintf("[pt] BAR0 not present, nothing to do.\n");
    return;
  }

  pt_bar0_base = (uint8*)P2V(bar0_pa);
  pt_hdr = (PCITestDevHdr*)pt_bar0_base;
  pt_iobase = bar1_port;
  pt_present = 1;

  // Dump raw header fields for test 0
  pt_hdr->test = 0;
  cprintf("[pt] Header for test 0:\n");
  cprintf("     width_type=%d offset=0x%x data=0x%x count=%u\n",
          pt_hdr->width_type, pt_hdr->offset, pt_hdr->data, pt_hdr->count);

  // Run a few tests as a demo
  pcitestdev_run_some_tests();

  // If you want, you can also poke BAR1 I/O (if it happens to be IO)
  if (pt_iobase) {
    uchar r = inb(pt_iobase);
    cprintf("[pt] BAR1 I/O test: inb(0x%x)=0x%x (likely unused)\n",
            pt_iobase, r);
  }

  cprintf("[pt] ready (no IRQs, just MMIO/IO tests)\n\n");
}

