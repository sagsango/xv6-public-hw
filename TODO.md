+----------------------------------------------------------------------------------+
|                      COMPLETE XV6 DRIVER DEVELOPMENT ROADMAP                     |
|                    (ALL QEMU LEARNING DEVICES + VIRTIO)                          |
+----------------------------------------------------------------------------------+

LEVEL 0 — MMIO-ONLY (NO PCI, NO DMA, NO IRQ COMPLEXITY)
---------------------------------------------------------
  [L0-01] unimp
  [L0-02] memdev
  [L0-03] pmem
  [L0-04] fw_cfg (sysbus)
  [L0-05] pvpanic (sysbus)
  [L0-06] sysbus-testdev
  [L0-07] gpio-led
  [L0-08] gpio-key
  [L0-09] pl031 rtc

→ These build fundamental MMIO access + register read/write skills.
→ Each device is under ~30–50 LOC to support in xv6.

------------------------------------------------------------------------------------

LEVEL 1 — PCI BASICS (BAR + MMIO + IRQ)
----------------------------------------
  [L1-10] pci-testdev
  [L1-11] edu
  [L1-12] fw_cfg (PCI)
  [L1-13] pvpanic-pci
  [L1-14] ivshmem (basic shared-memory mode; no MSI)

→ You learn:
   - PCI config-space enumeration
   - BARx decoding
   - MMIO window mapping
   - Legacy INTx interrupts (IRQ lines)
   - Basic DMA (edu)

------------------------------------------------------------------------------------

LEVEL 2 — PCI + MSI / MSI-X (ADVANCED INTERRUPTS)
---------------------------------------------------
  [L2-15] msi-testdev
  [L2-16] msix-testdev
  [L2-17] ivshmem (MSI/MSI-X mode)
  
→ Requires extending xv6:
     - LAPIC MSI delivery
     - IOAPIC bypass
     - MSI-X table parsing
     - Capability list traversal

→ These devices teach you:
     MSI (single vector)
     MSI-X (multiple vectors)
     Shared-memory event signaling (ivshmem)

------------------------------------------------------------------------------------

LEVEL 3 — VIRTIO (QUEUE-BASED DEVICES + DMA + INTERRUPTS)
-----------------------------------------------------------
  [L3-18] virtio-rng
  [L3-19] virtio-blk
  [L3-20] virtio-net
  [L3-21] virtio-console
  [L3-22] virtio-balloon   (optional)
  [L3-23] virtio-input     (optional)
  [L3-24] virtio-gpu       (very advanced)
  [L3-25] virtio-mmio      (transport)
  [L3-26] virtio-pci       (transport)

→ These require:
    - VirtIO PCI capability parsing
    - Feature negotiation
    - Descriptor tables
    - Avail/Used rings
    - Interrupt suppression logic
    - DMA buffers management

VirtIO is the “final boss” of xv6 device support.

------------------------------------------------------------------------------------

LEVEL 4 — ACPI + COMPLEX QOM MODELS
-------------------------------------
  [L4-27] fw-path-provider
  [L4-28] bios-tables-test
  [L4-29] xlnx-testdev
  [L4-30] empty_slot

→ For when you want:
    ACPI parsing  
    SMBIOS / FADT interpretation  
    QEMU’s QOM model understanding  
    Multi-block MMIO windows  

------------------------------------------------------------------------------------
|                            TOTAL DEVICES: 30                                     |
------------------------------------------------------------------------------------

