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

extern char _binary_kernel0_img_start;
extern char _binary_kernel0_img_end;

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

void init_graphics(Framebuffer *fb)
{
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    efi.st->BootServices->LocateProtocol(&gop_guid, NULL, (void **) &gop);

    fb->base = (void *) gop->Mode->FrameBufferBase;
    fb->width = gop->Mode->Info->HorizontalResolution;
    fb->height = gop->Mode->Info->VerticalResolution;
    fb->size = gop->Mode->FrameBufferSize;
    fb->pitch = gop->Mode->Info->PixelsPerScanLine;

    gop->SetMode(gop, gop->Mode->Mode);
}

static void get_memory_map(EfiMemoryMap *efi_mm, UINTN *map_key)
{
    efi_mm->memory_map = (EFI_MEMORY_DESCRIPTOR *) MemoryMapBuffer;
    efi_mm->memory_map_size = MEMORY_MAP_BUFFER_SIZE;

    EFI_STATUS status = efi.st->BootServices->GetMemoryMap(
        &efi_mm->memory_map_size,
        efi_mm->memory_map,
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

static void start_kernel0(void *kernel_base, const KernelParams *params)
{
    KernelStart start = (KernelStart) kernel_base;
    start(params);
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    efi.ih = image_handle;
    efi.st = system_table;

    UINTN map_key;
    KernelParams params;
    params.efi_rts = efi.st->RuntimeServices;

    init_serial();
    serial_print("> efi_main\r\n");
    // for(;;);

    void *kernel_base = (void *) KERNEL_PA;
    const void *kernel0_img = (const void *) &_binary_kernel0_img_start;
    size_t kernel0_img_size = ((size_t) &_binary_kernel0_img_end) - ((size_t) kernel0_img);

    init_gnu_efi();

    // print_memory_map();

    init_graphics(&params.fb);
    get_memory_map(&params.efi_mm, &map_key);
    exit_boot_services(map_key);

    memcpy(kernel_base, kernel0_img, kernel0_img_size);
    serial_print(">> start_kernel0\r\n");
    start_kernel0(kernel_base, &params);

    return EFI_SUCCESS;
}
