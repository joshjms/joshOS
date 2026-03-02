#include <arch/x86_64/pit.h>

#include <stdint.h>

#include <arch/x86_64/io.h>
#include <arch/x86_64/pic.h>
#include <drivers/serial.h>

#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND_REGISTER 0x43
#define PIT_FREQUENCY 1193182

static uint32_t pit_hz = 0;

void pit_sleep_ms(unsigned int ms) {
    uint64_t startTicks = pitInteruptsTriggered;
    uint64_t targetTicks = startTicks + ((uint64_t)ms * pit_hz) / 1000;

    while (pitInteruptsTriggered < targetTicks) {
        uint64_t currentTicks = pitInteruptsTriggered;

        if (currentTicks < startTicks) {
            startTicks = currentTicks;
            targetTicks = startTicks + (ms * PIT_FREQUENCY) / 1000;
        }

        asm volatile("hlt");
    }
}

void pit_init(uint32_t frequency) {
    pit_hz = frequency;

    uint32_t div = PIT_FREQUENCY / frequency;

    outb(PIT_COMMAND_REGISTER, 0x36);
    io_wait();

    outb(PIT_CHANNEL0, div & 0xFF);
    io_wait();
    outb(PIT_CHANNEL0, (div >> 8) & 0xFF);
    io_wait();
}
