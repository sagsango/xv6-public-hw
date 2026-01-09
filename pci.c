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
      }
    }
  }

  cprintf("=== PCI scan done ===\n");
}

