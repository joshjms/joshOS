#include <interrupts/interrupts.h>

#include <arch/x86_64/apic.h>
#include <drivers/serial.h>
#include <printk.h>

static const char *exception_names[] = {
    "Division by Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating Point",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating Point",
};

__attribute__((noreturn))
void exception_handler(struct registers *regs) {
    printk("Exception: %s (vector %d)\n", exception_names[regs->vector], regs->vector);
    
    __asm__ volatile("cli; hlt");
}

static void (*irq_handlers[256])(struct registers *);

void irq_register_handler(int irq, void (*handler)(struct registers *)) {
    irq_handlers[irq] = handler;
    irq_unmask(irq);
}

void interrupt_handler(struct registers *regs) {
    if (regs->vector == 0xFF) return;

    int irq = regs->vector - 32;
    if (irq_handlers[irq])
        irq_handlers[irq](regs);
    else {
        printk("Unhandled interrupt: vector %d\n", regs->vector);
    }
    lapic_eoi();
}
