// edu.c – driver for QEMU "edu" PCI device
//
// Covers:
//  - ID register
//  - liveness register
//  - raw IRQ raise/ack
//  - factorial engine (with IRQ)
//  - DMA engine using internal 0x40000 buffer

#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "traps.h"
#include "spinlock.h"
#include "param.h"
#include "proc.h"
#include "defs.h"

// MMIO offsets and bits from spec

// < 0x80: only 4-byte access
#define EDU_REG_ID           0x00  // RO: 0xRRrr00edu, version
#define EDU_REG_LIVENESS     0x04  // RW: value inversion (~)
#define EDU_REG_FACTORIAL    0x08  // RW: n (in), n! (out)
#define EDU_REG_STATUS       0x20  // RW
# define EDU_STATUS_BUSY     0x01  // computing factorial (RO)
# define EDU_STATUS_IRQFACT  0x80  // raise interrupt when factorial done
#define EDU_REG_IRQ_STATUS   0x24  // RO: bits that caused IRQ
#define EDU_REG_IRQ_RAISE    0x60  // WO: OR into IRQ_STATUS, trigger IRQ
#define EDU_REG_IRQ_ACK      0x64  // WO: clear bits from IRQ_STATUS

// DMA registers (size >= 4, or 8 allowed)
#define EDU_REG_DMA_SRC      0x80  // RW: DMA source addr
#define EDU_REG_DMA_DST      0x88  // RW: DMA dest addr
#define EDU_REG_DMA_CNT      0x90  // RW: length
#define EDU_REG_DMA_CMD      0x98  // RW: command bits
# define EDU_DMA_CMD_START   0x01  // start transfer
# define EDU_DMA_CMD_DIR     0x02  // 0: RAM->EDU, 1: EDU->RAM
# define EDU_DMA_CMD_IRQ     0x04  // raise IRQ (bit 0x100) after DMA

// internal device buffer offset for DMA
#define EDU_DEVBUF_OFFSET    0x40000   // within MMIO BAR

// default INTx, if PCI_INTERRUPT_LINE is 0/0xff
#define EDU_IRQ_DEFAULT 10

static volatile uint *edu_mmio;
static uchar edu_irq = EDU_IRQ_DEFAULT;

static struct spinlock edu_lock;
static uint edu_intr_count;
static uint edu_last_irq_status;

// exported from pci.c
uint  pci_read32(uchar, uchar, uchar, uchar);
ushort pci_read16(uchar, uchar, uchar, uchar);
uchar  pci_read8(uchar, uchar, uchar, uchar);
void   pci_write32(uchar, uchar, uchar, uchar, uint);

// ======== attach / init ========

// called from pci_scan()
void
edu_attach(uint bar0_raw, uchar irq_line)
{
  uint paddr = bar0_raw & ~0xF;   // clear BAR flags

  edu_mmio = (volatile uint*)P2V(paddr);

  if (irq_line != 0 && irq_line != 0xFF)
    edu_irq = irq_line;
  else
    edu_irq = EDU_IRQ_DEFAULT;

  initlock(&edu_lock, "edu");

  cprintf("EDU: mmio paddr=0x%x vaddr=%p irq=%d\n",
          paddr, edu_mmio, edu_irq);

  // enable legacy INTx on IOAPIC → CPU0
  ioapicenable(edu_irq, 0);

  // --- basic self-test ---

  // ID
  uint id = edu_mmio[EDU_REG_ID / 4];
  cprintf("EDU: ID reg=0x%x\n", id);

  // liveness: invert and check
  uint live = edu_mmio[EDU_REG_LIVENESS / 4];
  edu_mmio[EDU_REG_LIVENESS / 4] = ~live;
  uint live2 = edu_mmio[EDU_REG_LIVENESS / 4];
  cprintf("EDU: liveness 0x%x -> 0x%x\n", live, live2);

  // enable interrupt-after-factorial
  uint status = edu_mmio[EDU_REG_STATUS / 4];
  status |= EDU_STATUS_IRQFACT;
  edu_mmio[EDU_REG_STATUS / 4] = status;

  // clear any pending IRQ bits
  uint istatus = edu_mmio[EDU_REG_IRQ_STATUS / 4];
  if (istatus)
    edu_mmio[EDU_REG_IRQ_ACK / 4] = istatus;

  edu_intr_count = 0;
  edu_last_irq_status = 0;

  cprintf("EDU: attached and ready\n");
}

// ======== IRQ handling ========

void
edu_intr(void)
{
  if (!edu_mmio)
    return;

  uint istatus = edu_mmio[EDU_REG_IRQ_STATUS / 4];
  if (istatus == 0)
    return;   // spurious

  acquire(&edu_lock);
  edu_intr_count++;
  edu_last_irq_status = istatus;
  release(&edu_lock);

  // Ack: clear the same bits
  edu_mmio[EDU_REG_IRQ_ACK / 4] = istatus;

  cprintf("EDU: IRQ, irq_status=0x%x total=%d\n",
          istatus, edu_intr_count);
}

uint
edu_get_intr_count(void)
{
  uint n;
  acquire(&edu_lock);
  n = edu_intr_count;
  release(&edu_lock);
  return n;
}

uint
edu_get_last_irq_status(void)
{
  uint s;
  acquire(&edu_lock);
  s = edu_last_irq_status;
  release(&edu_lock);
  return s;
}

uchar
edu_get_irq(void)
{
  return edu_irq;
}

// Raise a raw IRQ with arbitrary value (tests IRQ_RAISE / IRQ_ACK)
void
edu_raise_irq(uint val)
{
  if (!edu_mmio)
    return;
  edu_mmio[EDU_REG_IRQ_RAISE / 4] = val;
}

// ======== liveness & ID helpers ========

uint
edu_get_id(void)
{
  if (!edu_mmio)
    return 0;
  return edu_mmio[EDU_REG_ID / 4];
}

// flips liveness register (~) and returns new value
uint
edu_liveness_flip(void)
{
  if (!edu_mmio)
    return 0;
  uint live = edu_mmio[EDU_REG_LIVENESS / 4];
  edu_mmio[EDU_REG_LIVENESS / 4] = ~live;
  return edu_mmio[EDU_REG_LIVENESS / 4];
}

// ======== factorial engine ========
//
// n must be small enough that n! fits in 32 bits (≤12).
// This uses STATUS.IRQFACT to generate an IRQ when done,
// but also busy-waits so the syscall is synchronous.

int
edu_factorial(uint n, uint *result)
{
  if (!edu_mmio || !result)
    return -1;
  if (n > 12)
    return -1;

  acquire(&edu_lock);

  // ensure no leftover IRQ bits
  uint istatus = edu_mmio[EDU_REG_IRQ_STATUS / 4];
  if (istatus)
    edu_mmio[EDU_REG_IRQ_ACK / 4] = istatus;

  // make sure "raise IRQ after factorial" bit is set
  uint status = edu_mmio[EDU_REG_STATUS / 4];
  status |= EDU_STATUS_IRQFACT;
  edu_mmio[EDU_REG_STATUS / 4] = status;

  // wait if currently busy
  while (edu_mmio[EDU_REG_STATUS / 4] & EDU_STATUS_BUSY)
    ;

  // write input
  edu_mmio[EDU_REG_FACTORIAL / 4] = n;

  // now wait until BUSY clears
  while (edu_mmio[EDU_REG_STATUS / 4] & EDU_STATUS_BUSY)
    ;

  uint fact = edu_mmio[EDU_REG_FACTORIAL / 4];

  release(&edu_lock);

  *result = fact;
  return 0;
}

// ======== DMA test ========
//
// Use one 4096-byte kernel buffer, DMA it into the device buffer at
// MMIO offset 0x40000, then DMA back and verify content.
// Generates DMA-complete IRQs via DMA_CMD_IRQ.

int
edu_dma_test(void)
{
  if (!edu_mmio)
    return -1;

  char *kbuf = kalloc();
  if (!kbuf)
    return -1;

  uint pa = V2P(kbuf);
  int len = 100;   // must be <= 4096

  // fill pattern
  for (int i = 0; i < len; i++)
    kbuf[i] = (char)(i ^ 0x5a);

  // RAM -> EDU buffer (0x40000)
  edu_mmio[EDU_REG_DMA_SRC / 4] = pa;
  edu_mmio[EDU_REG_DMA_DST / 4] = EDU_DEVBUF_OFFSET;
  edu_mmio[EDU_REG_DMA_CNT / 4] = len;
  edu_mmio[EDU_REG_DMA_CMD / 4] = EDU_DMA_CMD_START | EDU_DMA_CMD_IRQ; // start, raise IRQ (0x100)

  // wait until DONE
  while (edu_mmio[EDU_REG_DMA_CMD / 4] & EDU_DMA_CMD_START)
    ;

  // clear buffer
  for (int i = 0; i < len; i++)
    kbuf[i] = 0;

  // EDU buffer -> RAM
  edu_mmio[EDU_REG_DMA_SRC / 4] = EDU_DEVBUF_OFFSET;
  edu_mmio[EDU_REG_DMA_DST / 4] = pa;
  edu_mmio[EDU_REG_DMA_CNT / 4] = len;
  edu_mmio[EDU_REG_DMA_CMD / 4] = EDU_DMA_CMD_START |
                                  EDU_DMA_CMD_DIR   | // EDU->RAM
                                  EDU_DMA_CMD_IRQ;    // raise IRQ (0x100)

  while (edu_mmio[EDU_REG_DMA_CMD / 4] & EDU_DMA_CMD_START)
    ;

  // verify pattern
  int ok = 1;
  for (int i = 0; i < len; i++) {
    if (kbuf[i] != (char)(i ^ 0x5a)) {
      ok = 0;
      break;
    }
  }

  kfree(kbuf);
  return ok ? 0 : -1;
}

