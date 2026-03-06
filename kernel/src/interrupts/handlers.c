#include <interrupts/handlers.h>

#include <interrupts/interrupts.h>
#include <interrupts/timer.h>
#include <interrupts/keyboard.h>

void register_interrupt_handlers() {
    irq_register_handler(0, apic_timer_isr);
    irq_register_handler(1, keyboard_isr);
}
