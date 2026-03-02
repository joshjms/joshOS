#pragma once

#include <stdint.h>

extern volatile uint64_t pitInteruptsTriggered;

void pit_sleep_ms(unsigned int ms);

void pit_init(uint32_t hz);
