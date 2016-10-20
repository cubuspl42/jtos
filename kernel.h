#include <efi.h>

typedef struct {
	EFI_MEMORY_DESCRIPTOR *memory_map;
	UINT64 memory_map_size;
	UINT64 map_key;
	UINT64 descriptor_size;
	UINT32 descriptor_version;
} EfiMemoryMap;

typedef struct {
	void *base;
	UINT64 size; // in bytes?
	UINT32 pitch; // pixels per scanlines
} Framebuffer;

typedef struct {
	EFI_RUNTIME_SERVICES *efi_rts;
	EfiMemoryMap efi_mm;
	Framebuffer efi_fb;
} KernelParams;

typedef void (*KernelStart)(const KernelParams *);
