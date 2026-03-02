#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/pic.h>
#include <arch/x86_64/apic.h>
#include <interrupts/handlers.h>
#include <drivers/serial.h>
#include <printk.h>

// Set the base revision to 5, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(5);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

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

    idt_init();
    printk("idt_init() done.\n");

    __asm__ volatile ("sti");

    pic_init();
    printk("pic_init() done.\n");

    register_interrupt_handlers();

    __asm__ volatile ("sti");
    printk("Enabled interrupts\n");

    lapic_init(0xFEE00000);
    printk("lapic_init() done.\n");

    lapic_timer_init(0x20, 10);
    printk("lapic timer set up at 100Hz!\n");

    irq_mask(0);

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
