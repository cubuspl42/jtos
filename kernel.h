#pragma once
#include <efi.h>
#include "framebuffer.h"

#define KERNEL_PA 0x100000 // 1MiB; sync with kernel0.ld
#define KERNEL_VIRTUAL_BASE 0xffff800000000000 // sync with kernel1.ld
#define EFI_VIRTUAL_BASE (KERNEL_VIRTUAL_BASE + 0x40000000)
#define FRAMEBUFFER_VIRTUAL_BASE (EFI_VIRTUAL_BASE + 0x40000000)

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
