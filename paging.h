#pragma once
#include "kernel.h"

#define PAGE_SIZE_BITS 12
#define PAGE_SIZE 4096
#define PAGE_ALIGNED __attribute__((aligned(PAGE_SIZE)))

void setup_paging(
	EfiMemoryMap *mm, Framebuffer *fb, uint64_t kernel_pa, uint64_t kernel_va);
