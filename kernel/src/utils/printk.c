#include <utils/printk.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include <drivers/serial.h>

#define MAX_UINT64_STR 65

static void put_uint(uint64_t x, unsigned base, bool upper) {
    if(base < 2 || base > 16) {
        serial_puts("<bad base>");
        return;
    }

    if(x == 0) {
        serial_putc('0');
        return;
    }

    char buf[MAX_UINT64_STR];

    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";

    int i = 0;
    while(x && i < (MAX_UINT64_STR - 1)) {
        buf[i++] = digits[x % base];
        x /= base;
    }

    while(i--) serial_putc(buf[i]);
}

static void put_int(int64_t x) {
    if(x < 0) {
        serial_putc('-');
        put_uint((uint64_t)(~x) + 1, 10, false);
    } else {
        put_uint((uint64_t)x, 10, 0);
    }
}

static void vprintk(const char *fmt, va_list ap) {
    for(const char *p = fmt; *p; p++) {
        if(*p != '%') {
            serial_putc(*p);
            continue;
        }

        p++;
        if(!*p) break;

        switch(*p) {
        case '%': {
            serial_putc('%');
            break;
        }
        case 'c': {
            int c = va_arg(ap, int);
            serial_putc((char) c);
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char *);
            if(!s) s = "<null>";
            serial_puts(s);
            break;
        }
        case 'd':
        case 'i': {
            int64_t x = va_arg(ap, int64_t);
            put_int(x);
            break;
        }
        case 'u': {
            uint64_t x = va_arg(ap, uint64_t);
            put_uint(x, 10, false);
            break;
        }
        case 'x': {
            uint64_t x = va_arg(ap, uint64_t);
            put_uint(x, 16, false);
            break;
        }
        case 'X': {
            uint64_t x = va_arg(ap, uint64_t);
            put_uint(x, 16, true);
            break;
        }
        default: {
            serial_putc('%');
            serial_putc(*p);
            break;
        }
        }
    }
}

void printk(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    vprintk(fmt, ap);

    va_end(ap);
}
