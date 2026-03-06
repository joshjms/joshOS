#include <interrupts/keyboard.h>

#include <stdint.h>

#include <arch/x86_64/io.h>
#include <arch/x86_64/apic.h>
#include <interrupts/interrupts.h>
#include <utils/printk.h>

void keyboard_isr(struct registers *regs) {
    (void)regs;

    uint8_t scancode = inb(0x60);

    printk("Scancode: %x\n", scancode);
}
