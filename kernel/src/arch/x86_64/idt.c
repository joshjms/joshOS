#include <arch/x86_64/idt.h>

#include <stdint.h>
#include <stdbool.h>

#include <interrupts/interrupts.h>
#include <drivers/serial.h>

struct __attribute__((packed)) idt_entry {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t ist;
    uint8_t attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
};

__attribute__((aligned(0x10))) static struct idt_entry idt[256];

struct __attribute__((packed)) idt_ptr {
    uint16_t limit;
    uint64_t base;
};

static struct idt_ptr idtr;

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    struct idt_entry *descriptor = &idt[vector];

    descriptor->isr_low = (uint64_t)isr & 0xFFFF;
    descriptor->kernel_cs = 0x08;
    descriptor->ist = 0;
    descriptor->attributes = flags;
    descriptor->isr_mid = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->isr_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;
}

static bool vectors[256];

extern void* isr_stub_table[];
extern void isr_stub_255(void);

void idt_init() {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(struct idt_entry) * 256 - 1;

    for(uint8_t i = 0; i < 48; i++) {
        idt_set_descriptor(i, isr_stub_table[i], 0x8E);
        vectors[i] = true;
    }

    idt_set_descriptor(255, (void*)isr_stub_255, 0x8E);

    __asm__ volatile ("lidt %0" : : "m"(idtr));
}
