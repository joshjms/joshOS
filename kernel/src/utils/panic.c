#include <utils/panic.h>

#include <utils/printk.h>

void panic(const char *message) {
    printk("panic: %s\n", message);
    __asm__ volatile("cli");
    while(1) __asm__ volatile("hlt");
}
