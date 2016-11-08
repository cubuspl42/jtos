#pragma once
#include "kernel.h"

#define PAGE_SIZE_BITS 12
#define PAGE_SIZE 4096
#define PAGE_ALIGNED __attribute__((aligned(PAGE_SIZE)))

void enable_paging(void *kernel_hh_pa, EfiMemoryMap *mm, Framebuffer *fb);
