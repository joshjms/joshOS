#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <limine.h>

#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/pic.h>
#include <arch/x86_64/apic.h>
#include <arch/x86_64/ioapic.h>
#include <interrupts/handlers.h>
#include <drivers/serial.h>
#include <drivers/ps2.h>
#include <limine/requests.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <utils/printk.h>

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

const int MAJOR = 0;
const int MINOR = 1;
const int PATCH = 0;

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
void kmain(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    serial_init();
    printk("Serial initialized.\n");

    gdt_init();
    printk("gdt_init() done.\n");

    pmm_init();
    printk("pmm_init() done.\n");

    vmm_init();
    printk("vmm_init() done.\n");

    idt_init();
    printk("idt_init() done.\n");

    __asm__ volatile ("sti");

    pic_init();
    printk("pic_init() done.\n");

    if(!hhdm_request.response) {
        hcf();
    }
    uint64_t hhdm = hhdm_request.response->offset;

    pagemap_t *kernel_pagemap = vmm_get_kernel_pagemap();
    if(!kernel_pagemap) {
        hcf();
    }

    // Map the APIC first
    vmm_map(kernel_pagemap, hhdm + 0xFEE00000, 0xFEE00000, PTE_WRITABLE | PTE_NX | PTE_PWT | PTE_PCD);
    lapic_init(hhdm + 0xFEE00000);
    printk("lapic_init() done.\n");

    // Map the IOAPIC first
    vmm_map(kernel_pagemap, hhdm + 0xFEC00000, 0xFEC00000, PTE_WRITABLE | PTE_NX | PTE_PWT | PTE_PCD);
    ioapic_init(hhdm + 0xFEC00000);
    printk("ioapic_init() done.\n");

    register_interrupt_handlers();

    ps2_init();
    printk("ps2_init() done.\n");

    __asm__ volatile ("sti");
    printk("Enabled interrupts\n");

    lapic_timer_init(0x20, 10);
    printk("lapic timer set up at 100Hz!\n");

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    printk("joshjmsOS v%d.%d.%d\n", MAJOR, MINOR, PATCH);

    // We're done, just hang...
    hcf();
}
