#pragma once

#include <stdint.h>

#include <limine/requests.h>

#define HHDM_OFFSET hhdm_request.response->offset

#define PMM_PAGE_SIZE       4096
#define PMM_PAGE_NOT_FOUND  UINT64_MAX

void pmm_init();

void *pmm_alloc(void);

void pmm_free(void *addr);
