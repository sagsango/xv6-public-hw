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
uint   // <-- remove static so edu.c can call it
pci_config_addr(uchar bus, uchar dev, uchar func, uchar offset)
{
  return (1U << 31)          // enable bit
       | ((uint)bus  << 16)
       | ((uint)dev  << 11)
       | ((uint)func << 8)
       | (offset & 0xFC);    // dword aligned
}

uint   // <-- remove static
pci_read32(uchar bus, uchar dev, uchar func, uchar offset)
{
  uint addr = pci_config_addr(bus, dev, func, offset);
  outl(PCI_CONFIG_ADDRESS, addr);
  return inl(PCI_CONFIG_DATA);
}

ushort  // <-- remove static
pci_read16(uchar bus, uchar dev, uchar func, uchar offset)
{
  uint v = pci_read32(bus, dev, func, offset & ~3);
  uint shift = (offset & 2) * 8;
  return (v >> shift) & 0xFFFF;
}

uchar  // <-- remove static
pci_read8(uchar bus, uchar dev, uchar func, uchar offset)
{
  uint v = pci_read32(bus, dev, func, offset & ~3);
  uint shift = (offset & 3) * 8;
  return (v >> shift) & 0xFF;
}

// Write is needed for future capabilities/DMA masks etc
void
pci_write32(uchar bus, uchar dev, uchar func, uchar offset, uint val)
{
  uint addr = pci_config_addr(bus, dev, func, offset);
  outl(PCI_CONFIG_ADDRESS, addr);
  outl(PCI_CONFIG_DATA, val);
}

// Offsets in PCI config space
#define PCI_VENDOR_ID       0x00
#define PCI_DEVICE_ID       0x02
#define PCI_COMMAND         0x04
#define PCI_STATUS          0x06
#define PCI_CLASS_CODE      0x0B
#define PCI_SUBCLASS        0x0A
#define PCI_PROG_IF         0x09
#define PCI_HEADER_TYPE     0x0E
#define PCI_BAR0            0x10     // added
#define PCI_INTERRUPT_LINE  0x3C     // added

// Very small name helper for some common classes (optional)
static char *
pci_class_name(uchar class, uchar subclass)
{
  switch(class) {
  case 0x00:
    switch(subclass) {
        case 0xff: return "edu device";
        default: return "unkown subclass";
    }
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

// Forward declaration
extern void edu_attach(uint bar0_raw, uchar irq_line);

#define MAX_BUS 256
#define AVAILABLE_BUS 1


#define PCI_COMMAND            0x04
#define PCI_COMMAND_IO         0x1
#define PCI_COMMAND_MEMORY     0x2
#define PCI_COMMAND_BUSMASTER  0x4
void
pci_write16(uchar bus, uchar dev, uchar func, uchar offset, ushort val)
{
    uint addr = pci_config_addr(bus, dev, func, offset);
    outl(PCI_CONFIG_ADDRESS, addr);

    // Write 16-bit value using 32-bit access
    // Because CONFIG_DATA always performs 32-bit writes
    uint old = inl(PCI_CONFIG_DATA);

    if (offset & 2) {
        // high 16 bits
        val = val << 16;
        old = (old & 0x0000FFFF) | val;
    } else {
        // low 16 bits
        old = (old & 0xFFFF0000) | val;
    }

    outl(PCI_CONFIG_DATA, old);
}

/* XXX: Without enabling dma_test for the edu device does not work */
void
pci_enable_device(uchar bus, uchar dev, uchar func)
{
    ushort cmd = pci_read16(bus, dev, func, PCI_COMMAND);
    cmd |= (PCI_COMMAND_MEMORY | PCI_COMMAND_BUSMASTER);
    pci_write16(bus, dev, func, PCI_COMMAND, cmd);
}



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

        cprintf("bus %d dev %d func %d: "
                "vendor 0x%x device 0x%x "
                "class 0x%x subclass 0x%x prog_if 0x%x hdr 0x%x  (%s)\n",
                bus, dev, func,
                vendor, device,
                class, subclass, prog_if, hdr,
                pci_class_name(class, subclass));

        // ********** EDU DEVICE *************
        if (vendor == 0x1234 && device == 0x11e8) {

          // Read BAR0 (MMIO)
          uint bar0 = pci_read32(bus, dev, func, PCI_BAR0);

          // Read interrupt line from PCI config space
          uchar irq_line = pci_read8(bus, dev, func, PCI_INTERRUPT_LINE);

          cprintf("  -> Found EDU at %d:%d.%d BAR0=0x%x IRQ=%d\n",
                  bus, dev, func, bar0, irq_line);


          pci_enable_device(bus, dev, func);
          // Fully initialize the EDU device
          edu_attach(bar0, irq_line);
        }

      }
    }
  }

  cprintf("=== PCI scan done ===\n");
}

