#include "fb.h"
#include <fonts/psf.h>
#include <stdint.h>

static struct limine_framebuffer *_fb;

void fb_init(struct limine_framebuffer *fb) {
    _fb = fb;
}

void fb_putpixel(uint32_t x, uint32_t y, uint32_t color) {
    uint32_t *pixel = (uint32_t *)((uint8_t *)_fb->address
                                   + y * _fb->pitch
                                   + x * (_fb->bpp / 8));
    *pixel = color;
}

void fb_fill(uint32_t color) {
    for (uint32_t y = 0; y < _fb->height; y++) {
        for (uint32_t x = 0; x < _fb->width; x++) {
            fb_putpixel(x, y, color);
        }
    }
}

void fb_putchar(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg) {
    psf2_header_t *hdr = (psf2_header_t *)_font_start;
    if (hdr->magic != PSF2_MAGIC)
        return;

    unsigned char ch = (unsigned char)c;
    if (ch >= hdr->glyph_count)
        ch = '?';

    uint8_t *glyph = (uint8_t *)_font_start + hdr->header_size
                     + ch * hdr->glyph_size;

    uint32_t bytes_per_row = (hdr->width + 7) / 8;

    for (uint32_t row = 0; row < hdr->height; row++) {
        for (uint32_t col = 0; col < hdr->width; col++) {
            uint8_t byte = glyph[row * bytes_per_row + col / 8];
            uint32_t color = (byte >> (7 - (col % 8))) & 1 ? fg : bg;
            fb_putpixel(x + col, y + row, color);
        }
    }
}
