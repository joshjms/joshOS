#include <memory/pmm.h>

#include <stddef.h>

#include <limine/requests.h>
#include <utils/memory.h>
#include <utils/printk.h>
#include <utils/panic.h>

static uint8_t *bitmap;
static size_t total_frames;
static size_t bitmap_size;
static size_t used_frames;

void pmm_init() {
    if(memmap_request.response == NULL) {
        panic("PMM: limine memmap request failed.");
    }

    struct limine_memmap_entry **entries = memmap_request.response->entries;
    size_t entry_count = memmap_request.response->entry_count;

    uint64_t highest_addr = 0;
    for(int i = 0; i < entry_count; i++) {
        uint64_t end_addr = entries[i]->base + entries[i]->length;
        if(end_addr > highest_addr) {
            highest_addr = end_addr;
        }
    }

    total_frames = highest_addr / PMM_PAGE_SIZE;
    bitmap_size = total_frames / 8 + 1;

    for(int i = 0; i < entry_count; i++) {
        if(entries[i]->type == LIMINE_MEMMAP_USABLE && entries[i]->length >= bitmap_size) {
            bitmap = (uint8_t *)(entries[i]->base + HHDM_OFFSET);
            break;
        }
    }

    if(bitmap == NULL) {
        panic("PMM: no suitable memory region for bitmap.");
    }

    for(size_t i = 0; i < bitmap_size; i++) {
        bitmap[i] = 0xFF;
    }

    for(int i = 0; i < entry_count; i++) {
        uint64_t base = entries[i]->base;
        uint64_t length = entries[i]->length;
        uint64_t type = entries[i]->type;

        if(type == LIMINE_MEMMAP_USABLE) {
            size_t start_frame = base / PMM_PAGE_SIZE;
            size_t end_frame = (base + length) / PMM_PAGE_SIZE;

            for(size_t frame = start_frame; frame < end_frame; frame++) {
                size_t byte_index = frame / 8;
                size_t bit_index = frame % 8;
                bitmap[byte_index] &= ~(1U << bit_index);
            }
        }
    }

    uint64_t bitmap_start = (uint64_t)bitmap - HHDM_OFFSET;
    size_t bitmap_frames = (bitmap_size + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
    for(size_t frame = bitmap_start / PMM_PAGE_SIZE; frame < (bitmap_start / PMM_PAGE_SIZE) + bitmap_frames; frame++) {
        size_t byte_index = frame / 8;
        size_t bit_index = frame % 8;
        bitmap[byte_index] |= (1U << bit_index);
    }
}

void *pmm_alloc(void) {
    for(size_t i = 0; i < bitmap_size; i++) {
        if(bitmap[i] != 0xFF) {
            for(size_t bit = 0; bit < 8; bit++) {
                if((bitmap[i] & (1U << bit)) == 0) {
                    bitmap[i] |= (1U << bit);
                    used_frames++;
                    return (void *)(uintptr_t)((i * 8 + bit) * PMM_PAGE_SIZE);
                }
            }
        }
    }

    return PMM_PAGE_NOT_FOUND;
}

void pmm_free(void *addr) {
    size_t frame = (size_t)addr / PMM_PAGE_SIZE;
    size_t byte_index = frame / 8;
    size_t bit_index = frame % 8;

    if((bitmap[byte_index] & (1U << bit_index)) != 0) {
        bitmap[byte_index] &= ~(1U << bit_index);
        used_frames--;
    }
}
