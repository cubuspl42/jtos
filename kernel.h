#pragma once
#include <efi.h>
#include "framebuffer.h"

#define KERNEL_PA 0x100000 // 1MiB
#define KERNEL_VIRTUAL_BASE 0x8000000000000000 // ~0 / 2
#define KERNEL_VA (KERNEL_PA + KERNEL_VIRTUAL_BASE)
#define EFI_VIRTUAL_BASE 0xc000000000000000 // (~0 / 4) * 3

typedef struct {
	EFI_MEMORY_DESCRIPTOR *memory_map;
	UINT64 memory_map_size;
	UINT64 map_key;
	UINT64 descriptor_size;
	UINT32 descriptor_version;
} EfiMemoryMap;

typedef struct {
	EFI_RUNTIME_SERVICES *efi_rts;
	EfiMemoryMap efi_mm;
	Framebuffer fb;
} KernelParams;

typedef void (*KernelStart)(const KernelParams *);
