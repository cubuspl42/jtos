#include "efi.h"
#include "gfx.h"
#include "kernel.h"

#include <efi.h>
#include <efilib.h>
#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

#define MEMORY_MAP_BUFFER_SIZE 512 * 1024 // 512 KiB

#define KERNEL_PA 0x100000 // 1MiB
#define KERNEL_VIRTUAL_BASE 0x8000000000000000 // ~0 / 2
#define KERNEL_VA (KERNEL_PA + KERNEL_VIRTUAL_BASE)
#define EFI_VIRTUAL_BASE 0xc000000000000000 // (~0 / 4) * 3

extern char _binary_kernel_img_start;
extern char _binary_kernel_img_end;

char MemoryMapBuffer[MEMORY_MAP_BUFFER_SIZE];

static void* memcpy(void* dest, const void* src, size_t count) {
    char* dst8 = (char*)dest;
    char* src8 = (char*)src;

    while (count--) {
        *dst8++ = *src8++;
    }
    return dest;
}

void init_gnu_efi(void)
{
    InitializeLib(efi.ih, efi.st);
}

void init_graphics(Framebuffer *efi_fb)
{
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    efi.st->BootServices->LocateProtocol(&gop_guid, NULL, (void **) &gop);

    efi_fb->base = (void *) gop->Mode->FrameBufferBase;
    efi_fb->size = gop->Mode->FrameBufferSize;
    efi_fb->pitch = gop->Mode->Info->PixelsPerScanLine;

    gop->SetMode(gop, gop->Mode->Mode);
}

static void get_memory_map(EfiMemoryMap *efi_mm, UINTN *map_key)
{
    efi_mm->memory_map_size = MEMORY_MAP_BUFFER_SIZE;

    EFI_STATUS status = efi.st->BootServices->GetMemoryMap(
        &efi_mm->memory_map_size,
        (EFI_MEMORY_DESCRIPTOR *) MemoryMapBuffer,
        map_key,
        &efi_mm->descriptor_size,
        &efi_mm->descriptor_version);

    if(status != EFI_SUCCESS) {
        AbortBoot(L"get_memory_map failed", status);
    }

    // NOTE: From now on, printing makes `map_key` outdated
}

static void exit_boot_services(UINTN map_key)
{
    EFI_STATUS status = ST->BootServices->ExitBootServices(efi.ih, map_key);

    if(status != EFI_SUCCESS) {
        AbortBoot(L"ExitBootServices failed", status);
    }

    // NOTE: From now on, printing crashes system (ConOut is NULL)
}

static EFI_MEMORY_DESCRIPTOR *NextMd(EFI_MEMORY_DESCRIPTOR *Md, UINT64 DescriptorSize)
{
    char *p = ((char *) Md) + DescriptorSize;
    return (EFI_MEMORY_DESCRIPTOR *) p;
}

static void PrintMemoryType(EFI_MEMORY_TYPE t)
{
#define CASE(x) case x: Print(L ## #x); break;
    switch(t) {
    CASE(EfiReservedMemoryType)
    CASE(EfiLoaderCode)
    CASE(EfiLoaderData)
    CASE(EfiBootServicesCode)
    CASE(EfiBootServicesData)
    CASE(EfiRuntimeServicesCode)
    CASE(EfiRuntimeServicesData)
    CASE(EfiConventionalMemory)
    CASE(EfiUnusableMemory)
    CASE(EfiACPIReclaimMemory)
    CASE(EfiACPIMemoryNVS)
    CASE(EfiMemoryMappedIO)
    CASE(EfiMemoryMappedIOPortSpace)
    CASE(EfiPalCode)
    default: Print(L"unknown");
    }
#undef CASE
}

static void start_kernel(void *kernel_base, const KernelParams *kernel_params)
{
    KernelStart start = (KernelStart) kernel_base;
    start(kernel_params);
}

static void crash() {
    typedef void (*F)(void);
    F f = (F) 0x0;
    f();
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    efi.ih = image_handle;
    efi.st = system_table;

    UINTN map_key;
    KernelParams kernel_params;
    kernel_params.efi_rts = efi.st->RuntimeServices;

    void *kernel_base = (void *) KERNEL_PA;
    const void *kernel_img = (const void *) &_binary_kernel_img_start;
    size_t kernel_img_size = ((size_t) &_binary_kernel_img_end) - ((size_t) kernel_img);

    init_gnu_efi();
    init_graphics(&kernel_params.efi_fb);
    get_memory_map(&kernel_params.efi_mm, &map_key);
    exit_boot_services(map_key);
    
    memcpy(kernel_base, kernel_img, kernel_img_size);
    start_kernel(kernel_base, &kernel_params);

    return EFI_SUCCESS;
}
