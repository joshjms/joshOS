#pragma once

#include <stdint.h>

void pic_eoi(uint8_t irq);

void irq_mask(uint8_t irq_line);

void irq_unmask(uint8_t irq_line);

void pic_init();
