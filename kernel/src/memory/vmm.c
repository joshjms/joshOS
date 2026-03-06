#include <memory/vmm.h>

#include <stddef.h>
#include <stdbool.h>

#include <utils/memory.h>
#include <utils/panic.h>
#include <memory/pmm.h>
#include <limine/requests.h>

static pagemap_t kernel_pagemap;

extern uint8_t _text_start[], _text_end[];
extern uint8_t _rodata_start[], _rodata_end[];
extern uint8_t _data_start[], _data_end[];
extern uint8_t _bss_start[], _bss_end[];

static uint64_t *get_next_level(uint64_t *table, uint64_t index, bool allocate, uint64_t flags) {
    if(table[index] & PTE_PRESENT) {
        return (uint64_t *)(PTE_GET_ADDR(table[index]) + HHDM_OFFSET);
    }
    if(!allocate) return NULL;

    void *phys = (void *)pmm_alloc();
    if((uint64_t)phys == PMM_PAGE_NOT_FOUND) {
        panic("VMM: failed to allocate page for page table.");
    }

    uint64_t *virt = (uint64_t *)((uint64_t)phys + HHDM_OFFSET);
    memset(virt, 0, VMM_PAGE_SIZE);

    table[index] = ((uint64_t)phys & PTE_ADDR_MASK) | flags | PTE_PRESENT;
    return (uint64_t *)virt;
}

static void vmm_map_page(pagemap_t *pagemap, uint64_t vaddr, uint64_t paddr, uint64_t flags) {
    size_t pml4i = (vaddr >> 39) & 0x1ff;
    size_t pdpti = (vaddr >> 30) & 0x1ff;
    size_t pdi = (vaddr >> 21) & 0x1ff;
    size_t pti = (vaddr >> 12) & 0x1ff;

    uint64_t *pdpt = get_next_level(pagemap->pml4, pml4i, true, PTE_WRITABLE);
    uint64_t *pd = get_next_level(pdpt, pdpti, true, PTE_WRITABLE);
    uint64_t *pt = get_next_level(pd, pdi, true, PTE_WRITABLE);

    pt[pti] = PTE_GET_ADDR(paddr) | flags | PTE_PRESENT;
    __asm__ volatile ("invlpg (%0)" :: "r"(vaddr) : "memory");
}

pagemap_t *vmm_get_kernel_pagemap(void) {
    return &kernel_pagemap;
}

void vmm_map(pagemap_t *pagemap, uint64_t vaddr, uint64_t paddr, uint64_t flags) {
    vmm_map_page(pagemap, vaddr, paddr, flags);
}

void vmm_switch_pagemap(pagemap_t *pagemap) {
    uint64_t cr3 = (uint64_t)pagemap->pml4 - HHDM_OFFSET;
    __asm__ volatile("mov %0, %%cr3" :: "r"(cr3));
}

void vmm_init() {
    void *pml4_phys = (void *)pmm_alloc();
    if((uint64_t)pml4_phys == PMM_PAGE_NOT_FOUND) {
        panic("VMM: failed to allocate page for PML4.");
    }

    uint64_t *pml4_virt = (uint64_t *)((uint64_t)pml4_phys + HHDM_OFFSET);
    memset(pml4_virt, 0, VMM_PAGE_SIZE);

    kernel_pagemap.pml4 = pml4_virt;

    uint64_t kern_phys_base = kernel_address_request.response->physical_base;
    uint64_t kern_virt_base = kernel_address_request.response->virtual_base;

    for(uint64_t v = kern_virt_base; v < (uint64_t)_text_start; v += VMM_PAGE_SIZE)
        vmm_map_page(&kernel_pagemap, v, v - kern_virt_base + kern_phys_base, PTE_NX);

    for(uint64_t v = (uint64_t)_text_start; v < (uint64_t)_text_end; v += VMM_PAGE_SIZE)
        vmm_map_page(&kernel_pagemap, v, v - kern_virt_base + kern_phys_base, 0);

    for(uint64_t v = (uint64_t)_rodata_start; v < (uint64_t)_rodata_end; v += VMM_PAGE_SIZE)
        vmm_map_page(&kernel_pagemap, v, v - kern_virt_base + kern_phys_base, PTE_NX);

    for(uint64_t v = (uint64_t)_data_start; v < (uint64_t)_data_end; v += VMM_PAGE_SIZE)
        vmm_map_page(&kernel_pagemap, v, v - kern_virt_base + kern_phys_base, PTE_WRITABLE | PTE_NX);

    for(uint64_t v = (uint64_t)_bss_start; v < (uint64_t)_bss_end; v += VMM_PAGE_SIZE)
        vmm_map_page(&kernel_pagemap, v, v - kern_virt_base + kern_phys_base, PTE_WRITABLE | PTE_NX);

    uint64_t entry_count = memmap_request.response->entry_count;
    struct limine_memmap_entry **entries = memmap_request.response->entries;
    for(size_t i = 0; i < entry_count; i++) {
        uint64_t base = entries[i]->base & ~(VMM_PAGE_SIZE - 1);
        uint64_t length = entries[i]->length;
        uint64_t top = (base + length + VMM_PAGE_SIZE - 1) & ~(VMM_PAGE_SIZE - 1);
        for (uint64_t p = base; p < top; p += VMM_PAGE_SIZE) {
            vmm_map_page(&kernel_pagemap, p + HHDM_OFFSET, p, PTE_WRITABLE | PTE_NX);
        }
    };

    vmm_switch_pagemap(&kernel_pagemap);
}
