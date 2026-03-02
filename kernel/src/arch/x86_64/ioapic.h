#pragma once

#include <stdint.h>

void ioapic_set_irq(uint8_t irq, uint64_t apic_id, uint8_t vector);

void ioapic_mask(uint8_t irq);

void ioapic_unmask(uint8_t irq);

void ioapic_init(uintptr_t apic_base_addr);
