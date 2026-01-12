#include "types.h"
#include "defs.h"
#include "x86.h"

/*
 * PCI config mechanism #1:
 *   addr port: 0xCF8
 *   data port: 0xCFC
 */

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// Bits in config address (mechanism #1)
static uint
pci_config_addr(uchar bus, uchar dev, uchar func, uchar offset)
{
  return (1U << 31)          // enable bit
       | ((uint)bus  << 16)
       | ((uint)dev  << 11)
       | ((uint)func << 8)
       | (offset & 0xFC);    // dword aligned
}

static uint
pci_read32(uchar bus, uchar dev, uchar func, uchar offset)
{
  uint addr = pci_config_addr(bus, dev, func, offset);
  outl(PCI_CONFIG_ADDRESS, addr);
  return inl(PCI_CONFIG_DATA);
}

static ushort
pci_read16(uchar bus, uchar dev, uchar func, uchar offset)
{
  uint v = pci_read32(bus, dev, func, offset & ~3);
  uint shift = (offset & 2) * 8;
  return (v >> shift) & 0xFFFF;
}

static uchar
pci_read8(uchar bus, uchar dev, uchar func, uchar offset)
{
  uint v = pci_read32(bus, dev, func, offset & ~3);
  uint shift = (offset & 3) * 8;
  return (v >> shift) & 0xFF;
}

static void
pci_write16(uchar bus, uchar dev, uchar func, uchar offset, ushort val)
{
    uint addr = pci_config_addr(bus, dev, func, offset);
    outl(PCI_CONFIG_ADDRESS, addr);
    outw(PCI_CONFIG_DATA, val);
}

// Offsets in PCI config space
#define PCI_VENDOR_ID   0x00
#define PCI_DEVICE_ID   0x02
#define PCI_COMMAND     0x04
#define PCI_STATUS      0x06
#define PCI_CLASS_CODE  0x0B
#define PCI_SUBCLASS    0x0A
#define PCI_PROG_IF     0x09
#define PCI_HEADER_TYPE 0x0E

// Very small name helper for some common classes (optional)
static char *
pci_class_name(uchar class, uchar subclass)
{
  switch(class) {
  case 0x01: // mass storage
    switch(subclass) {
    case 0x01: return "IDE controller";
    case 0x06: return "SATA controller";
    default:   return "Storage controller";
    }
  case 0x02:
    return "Network controller";
  case 0x03:
    return "Display controller";
  case 0x06:
    switch(subclass) {
    case 0x00: return "Host bridge";
    case 0x01: return "ISA bridge";
    case 0x04: return "PCI-to-PCI bridge";
    default:   return "Bridge device";
    }
  default:
    return "Unknown class";
  }
}

#define MAX_BUS 256
#define AVAILABLE_BUS 1
    void
pci_scan(void)
{
    cprintf("=== PCI devices ===\n");

    int n_bus = AVAILABLE_BUS;
    for(uchar bus = 0; bus < n_bus; bus++) {
        for(uchar dev = 0; dev < 32; dev++) {
            for(uchar func = 0; func < 8; func++) {
                ushort vendor = pci_read16(bus, dev, func, PCI_VENDOR_ID);
                if(vendor == 0xFFFF)
                    continue;  // no device here

                ushort device   = pci_read16(bus, dev, func, PCI_DEVICE_ID);
                uchar  class    = pci_read8(bus, dev, func, PCI_CLASS_CODE);
                uchar  subclass = pci_read8(bus, dev, func, PCI_SUBCLASS);
                uchar  prog_if  = pci_read8(bus, dev, func, PCI_PROG_IF);
                uchar  hdr      = pci_read8(bus, dev, func, PCI_HEADER_TYPE);

                cprintf("bus %d dev %d func %x: "
                        "vendor %d device %d "
                        "class %d subclass %d prog_if %d hdr %d  (%s)\n",
                        bus, dev, func,
                        vendor, device,
                        class, subclass, prog_if, hdr,
                        pci_class_name(class, subclass));


                if (vendor == 0x1b36 && device == 0x0005) {
                    cprintf("[pci] QEMU PCI testdev found at %d:%d.%d\n",
                            bus, dev, func);

                    // BAR0 (could be MMIO or IO – here we expect MMIO in your setup)
                    uint bar0 = pci_read32(bus, dev, func, 0x10);
                    uint bar0_pa   = bar0 & ~0xF;   // strip type bits
                    uint bar0_type = bar0 & 0xF;
                    cprintf("[pci] BAR0 raw=0x%x type=0x%x PA=0x%x\n",
                            bar0, bar0_type, bar0_pa);

                    // BAR1 (might be IO in your config)
                    uint bar1 = pci_read32(bus, dev, func, 0x14);
                    uint bar1_port  = bar1 & ~0x3;
                    uint bar1_type  = bar1 & 0x3;
                    cprintf("[pci] BAR1 raw=0x%x type=0x%x I/O=0x%x\n",
                            bar1, bar1_type, bar1_port);

                    // Enable I/O + MEM + Bus Master in PCI Command (0x04)
                    ushort cmd = pci_read16(bus, dev, func, 0x04);
                    cmd |= 0x1;  // I/O Space
                    cmd |= 0x2;  // MEM Space
                    cmd |= 0x4;  // Bus Master
                    pci_write16(bus, dev, func, 0x04, cmd);
                    cprintf("[pci] command=0x%x\n", cmd);

                    // NOTE: spec does not define interrupts; IRQ line is often 0 -> ignore
                    uchar irq_line = pci_read8(bus, dev, func, 0x3C);
                    uchar irq_pin  = pci_read8(bus, dev, func, 0x3D);
                    cprintf("[pci] IRQ line=%d pin=%d (ignored for pci-testdev)\n",
                            irq_line, irq_pin);

                    // No ioapicenable() here – device is for I/O tests, not IRQ tests.

                    pcitestdev_init(bar0_pa, bar1_port);
                }
            }
        }
    }

    cprintf("=== PCI scan done ===\n");
}

