# xv6 physical memory mapping
#define EXTMEM  0x100000            // Start of extended memory
#define PHYSTOP 0xE000000           // Top physical memory

# edu device mmio physical adress (last device)
xv6-public-hw ❱❱❱ telnet localhost 4444
Trying ::1...
Connected to localhost.
Escape character is '^]'.
QEMU 10.1.0 monitor - type 'help' for more information
(qemu) info pci
  Bus  0, device   0, function 0:
    Host bridge: PCI device 8086:1237
      PCI subsystem 1af4:1100
      id ""
  Bus  0, device   1, function 0:
    ISA bridge: PCI device 8086:7000
      PCI subsystem 1af4:1100
      id ""
  Bus  0, device   1, function 1:
    IDE controller: PCI device 8086:7010
      PCI subsystem 1af4:1100
      BAR4: I/O at 0xc000 [0xc00f]
      id ""
  Bus  0, device   1, function 3:
    Bridge: PCI device 8086:7113
      PCI subsystem 1af4:1100
      IRQ 9, pin A
      id ""
  Bus  0, device   2, function 0:
    VGA controller: PCI device 1234:1111
      PCI subsystem 1af4:1100
      BAR0: 32 bit prefetchable memory at 0xfd000000 [0xfdffffff]
      BAR2: 32 bit memory at 0xfeb10000 [0xfeb10fff]
      BAR6: 32 bit memory (not mapped)
      id ""
  Bus  0, device   3, function 0:
    Class 0255: PCI device 1234:11e8
      PCI subsystem 1af4:1100
      IRQ 11, pin A
      BAR0: 32 bit memory at 0xfea00000 [0xfeafffff]
      id ""


# conclusion
1. edu device mmio memory is not mapped by the xv6, so its not working
2. vga is using lagecy legacy text mode.



# Why VGA Works in xv6 Despite High BAR Addresses
You're spot on—the VGA controller's BAR0 (0xfd000000, a 1MB prefetchable framebuffer for graphics modes) and BAR2 (0xfeb10000, ~4KB registers) are way beyond xv6's PHYSTOP (0xE000000). If xv6 tried direct access like *(uint32_t*)0xfd000000, it'd page-fault just like the EDU device. But xv6's VGA "works" for basic text console output because it doesn't use those PCI BARs at all. Instead, it relies on legacy VGA hardware interfaces that predate PCI MMIO and are hardcoded in low memory/ports. This is by design for simplicity in xv6 (and many teaching OSes).
How xv6 Handles VGA (Legacy Mode)
xv6's console in kernel/console.c (and vgawrite()/vga_init()) uses:

VGA Text Buffer RAM at fixed physical address 0xB8000 (80x25 chars, 2 bytes each: char + attribute). This is:
Low memory (within EXTMEM to PHYSTOP—fully mapped by kmap() in mmu.c).
Direct write: *(ushort*)0xB8000 = (attr << 8) | ch; (e.g., white-on-black text).
No BAR needed—it's a legacy artifact from IBM PC BIOS.

I/O Ports for Control (not memory-mapped):
Ports 0x3C0–0x3DF (e.g., outb(0x3C8, color); for palette).
Accessed via outb()/inb() in x86.h—these are direct CPU I/O instructions (out %al, %dx), bypassing page tables entirely.
xv6 probes/enables via picenable(2) (IRQ for vertical retrace) but mostly polls.


This setup emulates a "dumb terminal" on the VGA card without touching PCI config or high BARs. During boot:

QEMU's VGA (1234:1111, Cirrus-like) responds to these legacy accesses.
Text scrolls in the QEMU window (if not -nographic).

From your info pci: BAR6 "not mapped" is the expansion ROM (BIOS shadow, ignored in xv6).
Contrast with EDU Device

EDU/pci-testdev: Pure MMIO-only (no legacy ports), so requires ioremap() for high BARs to access registers like IRQ_EN at bar_phys + 0x00.
VGA: Hybrid—legacy ports for basic ops, MMIO BARs for advanced (e.g., VESA framebuffer modes). xv6 sticks to legacy for minimalism.

If You Want Full PCI VGA (Graphics/Framebuffer)
To use BAR0 as a linear framebuffer (e.g., for plotting pixels beyond text mode), extend your driver with ioremap (from previous response):

In pci.c (during enum for class 0x03):Cif (class == 0x03 && subclass == 0x00) {  // VGA
  uint32_t fb_bar = pci_read(bus, dev, func, PCI_BAR0) & ~0xF;  // Prefetchable mem
  uint32_t reg_bar = pci_read(bus, dev, func, PCI_BAR2) & ~0xF; // I/O regs
  if (fb_bar) {
    void* fb_va = ioremap(fb_bar, 0x100000);  // Map 1MB framebuffer
    extern void vga_init(void* fb, void* regs, uint8_t irq);
    vga_init(fb_va, ioremap(reg_bar, 0x1000), irq_line);
  }
}
In new kernel/vga_pci.c:
Write pixels: *(uint32_t*)(fb_va + x + y*width*4) = 0xFF0000; // Red.
But xv6 lacks a window system—add a syscall for user apps to draw.
Enable: Set command reg (PCI 0x04) for mem access, then program VGA mode (e.g., 0x13 via port 0x3C2).


Test: Add -vga std to QEMU for graphical output. Without it, xv6 falls back to serial.
This keeps text console intact while unlocking graphics. If you're seeing blank QEMU screen (no text), check -serial stdio vs. -nographic—or paste boot logs for debug!3.3s
