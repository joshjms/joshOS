#include <drivers/ps2.h>

#include <arch/x86_64/io.h>

#define PS2_DATA    0x60
#define PS2_CMD     0x64
#define PS2_STATUS  0x64

#define PS2_CMD_READ_CONFIG  0x20
#define PS2_CMD_WRITE_CONFIG 0x60

static void ps2_wait_write(void) {
    while (inb(PS2_STATUS) & 0x02);
}

static void ps2_wait_read(void) {
    while (!(inb(PS2_STATUS) & 0x01));
}

void ps2_init() {
    // Read controller configuration byte
    ps2_wait_write();
    outb(PS2_CMD, PS2_CMD_READ_CONFIG);
    ps2_wait_read();
    uint8_t config = inb(PS2_DATA);

    // Enable keyboard interrupt (bit 0) and keyboard port clock (clear bit 4)
    config |= (1 << 0);
    config &= ~(1 << 4);

    // Write config back
    ps2_wait_write();
    outb(PS2_CMD, PS2_CMD_WRITE_CONFIG);
    ps2_wait_write();
    outb(PS2_DATA, config);
}
