#include <memory/heap.h>

#include <stdbool.h>

#include <memory/pmm.h>
#include <memory/vmm.h>
#include <utils/panic.h>

#define HEAP_START 0xffff900000000000
#define HEAP_PAGES 1024

typedef struct block_t {
    size_t size;
    bool free;
    struct block_t *next;
} block_t;

static block_t *heap_start = NULL;

void heap_init(void) {
    for(size_t i = 0; i < HEAP_PAGES; i++) {
        uint64_t paddr = pmm_alloc();
        if((uint64_t)paddr == PMM_PAGE_NOT_FOUND) {
            panic("heap: failed to allocate page for heap");
        }
        vmm_map(vmm_get_kernel_pagemap(), HEAP_START + i * VMM_PAGE_SIZE, paddr, PTE_WRITABLE | PTE_NX);
    }

    heap_start = (block_t *)HEAP_START;
    heap_start->size = HEAP_PAGES * VMM_PAGE_SIZE - sizeof(block_t);
    heap_start->free = true;
    heap_start->next = NULL;
}

void *kmalloc(size_t size) {
    if(size == 0) return NULL;

    block_t *current = heap_start;
    while(current != NULL) {
        if(current->free && current->size >= size) {
            if(current->size > size + sizeof(block_t)) {
                block_t *new_block = (block_t *)((uint64_t) current + sizeof(block_t) + size);
                new_block->size = current->size - size - sizeof(block_t);
                new_block->free = true;
                new_block->next = current->next;
                current->next = new_block;
                current->size = size;
            }
            current->free = false;
            return (void *)((uint64_t)current + sizeof(block_t));
        }
        current = current->next;
    }

    panic("heap: out of memory");
    return NULL;
}

void kfree(void *ptr) {
    if(ptr == NULL) return;

    block_t *block = (block_t *)((uint64_t)ptr - sizeof(block_t));
    block->free = true;

    block_t *current = heap_start;
    while(current != NULL && current->next != NULL) {
        if(current->free && current->next->free) {
            current->size += sizeof(block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}
