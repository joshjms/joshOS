#pragma once
#include <stdint.h>

#define PSF2_MAGIC 0x864ab572

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t flags;
    uint32_t glyph_count;
    uint32_t glyph_size;
    uint32_t height;
    uint32_t width;
} psf2_header_t;

extern char _font_start[];
extern char _font_end[];
