#pragma once

#include <stdint.h>

void lapic_init(uintptr_t lapic_base);

void lapic_timer_init(uint8_t vector, uint32_t ms);

void lapic_eoi(void);

uint32_t lapic_id(void);
