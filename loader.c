#include "efi.h"
#include "gfx.h"
#include "kernel.h"
#include "serial.h"

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
    efi_fb->width = gop->Mode->Info->HorizontalResolution;
    efi_fb->height = gop->Mode->Info->VerticalResolution;
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

static void print_memory_type(EFI_MEMORY_TYPE t)
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

static EFI_MEMORY_DESCRIPTOR *NextMd(EFI_MEMORY_DESCRIPTOR *Md, UINT64 DescriptorSize)
{
    char *p = ((char *) Md) + DescriptorSize;
    return (EFI_MEMORY_DESCRIPTOR *) p;
}

static void print_memory_map(void)
{
    static char MemoryMapBuffer[256 * 1024];
    UINT64 MemoryMapSize = MEMORY_MAP_BUFFER_SIZE;
    UINT64 MapKey;
    UINT64 DescriptorSize;
    UINT32 DescriptorVersion;
    EFI_STATUS status;
    EFI_MEMORY_DESCRIPTOR *Md;
    UINT64 i;

    status = efi.st->BootServices->GetMemoryMap(
        &MemoryMapSize,
        (EFI_MEMORY_DESCRIPTOR *) MemoryMapBuffer,
        &MapKey,
        &DescriptorSize,
        &DescriptorVersion);

    // PRINT_U64(MemoryMapSize);
    // PRINT_U64(MapKey);
    // PRINT_U64(DescriptorSize);
    // PRINT_U64(DescriptorVersion);

    MemoryMapSize = 64;
    Md = (EFI_MEMORY_DESCRIPTOR *) MemoryMapBuffer; 
    for(i = 0; i < MemoryMapSize; ++i, Md = NextMd(Md, DescriptorSize)) {
        print_memory_type(Md->Type);
        Print(L";PS=%lx;VS=%lx;NoP=%ld;EMRT=%d\r\n",
            Md->PhysicalStart,
            Md->VirtualStart,
            Md->NumberOfPages,
            !!(Md->Attribute & EFI_MEMORY_RUNTIME));
    }

    PrintStatus(status);
}

static void crash() {
    typedef void (*F)(void);
    F f = (F) 0x0;
    f();
}

static void start_kernel(void *kernel_base, const KernelParams *kernel_params)
{
    KernelStart start = (KernelStart) kernel_base;
    start(kernel_params);
}

static void serial_print_mem(const void *mem, int n) {
    const char *memc = (const char *) mem;
    for(int i = 0; i < n; ++i) {
        serial_print_hex(memc[i]);
        serial_print(":");
    }
    serial_print("\r\n");
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    efi.ih = image_handle;
    efi.st = system_table;

    UINTN map_key;
    KernelParams kernel_params;
    kernel_params.efi_rts = efi.st->RuntimeServices;

    init_serial();

    void *kernel_base = (void *) KERNEL_PA;
    const void *kernel_img = (const void *) &_binary_kernel_img_start;
    size_t kernel_img_size = ((size_t) &_binary_kernel_img_end) - ((size_t) kernel_img);

    init_gnu_efi();

    // print_memory_map();

    init_graphics(&kernel_params.efi_fb);
    get_memory_map(&kernel_params.efi_mm, &map_key);
    exit_boot_services(map_key);

    memcpy(kernel_base, kernel_img, kernel_img_size);
    start_kernel(kernel_base, &kernel_params);

    return EFI_SUCCESS;
}
