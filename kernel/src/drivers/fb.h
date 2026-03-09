#pragma once
#include <stdint.h>
#include <limine.h>

void fb_init(struct limine_framebuffer *fb);
void fb_putpixel(uint32_t x, uint32_t y, uint32_t color);
void fb_fill(uint32_t color);
void fb_putchar(char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);
