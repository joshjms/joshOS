#include <interrupts/interrupts.h>

#include <arch/x86_64/apic.h>
#include <arch/x86_64/ioapic.h>
#include <drivers/serial.h>
#include <utils/printk.h>

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
    printk("RIP=%x  CS=%x  RFLAGS=%x\n", (void*)regs->rip, regs->cs, regs->rflags);
    printk("RSP=%x  SS=%x\n", (void*)regs->rsp, regs->ss);

    printk("RAX=%x RBX=%x RCX=%x RDX=%x\n", regs->rax, regs->rbx, regs->rcx, regs->rdx);
    printk("RSI=%x RDI=%x RBP=%x\n", regs->rsi, regs->rdi, regs->rbp);
    printk("R8 =%x R9 =%x R10=%x R11=%x\n", regs->r8,  regs->r9,  regs->r10, regs->r11);
    printk("R12=%x R13=%x R14=%x R15=%x\n", regs->r12, regs->r13, regs->r14, regs->r15);
    
    __asm__ volatile("cli; hlt");
}

static void (*irq_handlers[256])(struct registers *);

void irq_register_handler(int irq, void (*handler)(struct registers *)) {
    irq_handlers[irq] = handler;
    ioapic_set_irq(irq, lapic_id(), 32 + irq);
    ioapic_unmask(irq);
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
