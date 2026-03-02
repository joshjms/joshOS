#include <arch/x86_64/apic.h>

#include <stdint.h>

#include <arch/x86_64/msr.h>
#include <arch/x86_64/io.h>

#define IA32_APIC_BASE_MSR      0x1B
#define IA32_APIC_BASE_MSR_EXTD (1 << 10)
#define IA32_APIC_BASE_MSR_EN   (1 << 11)

#define LAPIC_ID         0x020
#define LAPIC_TPR        0x080
#define LAPIC_EOI        0x0B0
#define LAPIC_SVR        0x0F0
#define LAPIC_LVT_TIMER  0x320
#define LAPIC_TIMER_INIT 0x380
#define LAPIC_TIMER_CUR  0x390
#define LAPIC_TIMER_DIV  0x3E0

#define LAPIC_SVR_ENABLE      (1 << 8)
#define LAPIC_SPURIOUS_VECTOR 0xFF

#define LAPIC_TIMER_PERIODIC (1 << 17)
#define LAPIC_TIMER_MASKED   (1 << 16)
#define LAPIC_TIMER_DIV16    0x3

static uintptr_t lapic_base_addr;
static int x2apic_mode;
static uint32_t lapic_ticks_per_ms;

static uint32_t lapic_read(uint32_t reg) {
    if (x2apic_mode) {
        return (uint32_t)rdmsr(0x800 + (reg >> 4));
    }
    return *(volatile uint32_t *)(lapic_base_addr + reg);
}

static void lapic_write(uint32_t reg, uint32_t val) {
    if (x2apic_mode) {
        wrmsr(0x800 + (reg >> 4), val);
    } else {
        *(volatile uint32_t *)(lapic_base_addr + reg) = val;
    }
}

void lapic_eoi(void) {
    lapic_write(LAPIC_EOI, 0);
}

uint32_t lapic_id(void) {
    if (x2apic_mode) {
        return (uint32_t)rdmsr(0x800 + (LAPIC_ID >> 4));
    }
    return lapic_read(LAPIC_ID) >> 24;
}

static void lapic_timer_calibrate(void) {
    // Configure PIT channel 0: mode 0 (one-shot), 10ms countdown
    uint16_t pit_count = 11931; // 1193182 Hz / 100 = ~10ms
    outb(0x43, 0x30);           // ch0, lobyte/hibyte, mode 0
    outb(0x40, pit_count & 0xFF);
    outb(0x40, (pit_count >> 8) & 0xFF);

    lapic_write(LAPIC_TIMER_DIV, LAPIC_TIMER_DIV16);
    lapic_write(LAPIC_LVT_TIMER, LAPIC_TIMER_MASKED);
    lapic_write(LAPIC_TIMER_INIT, 0xFFFFFFFF);

    // Poll PIT status until output pin goes high (terminal count reached)
    uint8_t status;
    do {
        outb(0x43, 0xE2); // readback: latch status for ch0
        status = inb(0x40);
    } while (!(status & (1 << 7)));

    uint32_t ticks_elapsed = 0xFFFFFFFF - lapic_read(LAPIC_TIMER_CUR);
    lapic_write(LAPIC_TIMER_INIT, 0);

    lapic_ticks_per_ms = ticks_elapsed / 10;
}

void lapic_timer_init(uint8_t vector, uint32_t ms) {
    lapic_timer_calibrate();

    lapic_write(LAPIC_TIMER_DIV, LAPIC_TIMER_DIV16);
    lapic_write(LAPIC_LVT_TIMER, vector | LAPIC_TIMER_PERIODIC);
    lapic_write(LAPIC_TIMER_INIT, lapic_ticks_per_ms * ms);
}

static int cpu_has_x2apic(void) {
    uint32_t ecx;
    __asm__ volatile ("cpuid" : "=c"(ecx) : "a"(1) : "ebx", "edx");
    return (ecx >> 21) & 1;
}

void lapic_init(uintptr_t lapic_base) {
    lapic_base_addr = lapic_base;

    uint64_t apic_msr = rdmsr(IA32_APIC_BASE_MSR);
    x2apic_mode = (apic_msr & IA32_APIC_BASE_MSR_EXTD) != 0;

    if (!x2apic_mode) {
        if (cpu_has_x2apic()) {
            // Enable xAPIC first, then upgrade to x2APIC
            wrmsr(IA32_APIC_BASE_MSR, apic_msr | IA32_APIC_BASE_MSR_EN);
            wrmsr(IA32_APIC_BASE_MSR, apic_msr | IA32_APIC_BASE_MSR_EN | IA32_APIC_BASE_MSR_EXTD);
            x2apic_mode = 1;
        } else {
            // xAPIC mode — lapic_base must be a valid virtual address
            wrmsr(IA32_APIC_BASE_MSR, apic_msr | IA32_APIC_BASE_MSR_EN);
        }
    }

    // Accept all interrupt priorities
    lapic_write(LAPIC_TPR, 0);

    // Enable LAPIC and set spurious interrupt vector
    lapic_write(LAPIC_SVR, LAPIC_SVR_ENABLE | LAPIC_SPURIOUS_VECTOR);
}
