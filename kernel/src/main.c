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
#include <drivers/fb.h>
#include <limine/requests.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <memory/heap.h>
#include <utils/printk.h>

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
void kmain(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    serial_init();

    gdt_init();

    pmm_init();

    vmm_init();

    heap_init();

    idt_init();

    __asm__ volatile ("sti");

    pic_init();

    if(!hhdm_request.response) {
        hcf();
    }
    uint64_t hhdm = hhdm_request.response->offset;

    pagemap_t *kernel_pagemap = vmm_get_kernel_pagemap();
    if(!kernel_pagemap) {
        hcf();
    }

    vmm_map(kernel_pagemap, hhdm + 0xFEE00000, 0xFEE00000, PTE_WRITABLE | PTE_NX | PTE_PWT | PTE_PCD);
    lapic_init(hhdm + 0xFEE00000);

    vmm_map(kernel_pagemap, hhdm + 0xFEC00000, 0xFEC00000, PTE_WRITABLE | PTE_NX | PTE_PWT | PTE_PCD);
    ioapic_init(hhdm + 0xFEC00000);

    register_interrupt_handlers();

    ps2_init();

    __asm__ volatile ("sti");

    lapic_timer_init(0x20, 10);

    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    fb_init(framebuffer);

    const char *msg = "joshjmsOS";
    for (int i = 0; msg[i]; i++) {
        fb_putchar(msg[i], 10 + i * 10, 10, 0xFFFFFF, 0x000000);
    }

    // We're done, just hang...
    hcf();
}
