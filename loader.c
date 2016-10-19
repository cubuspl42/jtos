#include "efi.h"
#include "gfx.h"
#include "kernel.h"

#include <efi.h>
#include <efilib.h>
#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

#define MEMORY_MAP_BUFFER_SIZE 512 * 1024 // 512 KiB

#define PRINT_U64(x) Print(L ## #x L" = %lx\r\n", (UINT64) x)

#define KERNEL_PA 0x100000 // 1MiB
#define KERNEL_VIRTUAL_BASE 0x8000000000000000 // ~0 / 2
#define KERNEL_VA (KERNEL_PA + KERNEL_VIRTUAL_BASE)
#define EFI_VIRTUAL_BASE 0xc000000000000000 // (~0 / 4) * 3

extern char _binary_kernel_img_start;
extern char _binary_kernel_img_end;
extern int _binary_kernel_img_size;

typedef struct {
    char MemoryMapBuffer[MEMORY_MAP_BUFFER_SIZE];
    UINT64 MemoryMapSize;
    UINT64 MapKey;
    UINT64 DescriptorSize;
    UINT32 DescriptorVersion;
} MemoryMap;

MemoryMap efi_mm = {0};

static void* memcpy(void* dest, const void* src, size_t count) {
    char* dst8 = (char*)dest;
    char* src8 = (char*)src;

    while (count--) {
        *dst8++ = *src8++;
    }
    return dest;
}

static void GetMemoryMap(void)
{
    Print(L"GetMemoryMap...\r\n");

    efi_mm.MemoryMapSize = MEMORY_MAP_BUFFER_SIZE;

    EFI_STATUS status = efi.st->BootServices->GetMemoryMap(
        &efi_mm.MemoryMapSize,
        (EFI_MEMORY_DESCRIPTOR *) efi_mm.MemoryMapBuffer,
        &efi_mm.MapKey,
        &efi_mm.DescriptorSize,
        &efi_mm.DescriptorVersion);

    // NOTE: From now on, printing makes `MapKey` outdated

    if(status != EFI_SUCCESS) {
        AbortBoot(L"GetMemoryMap failed", status);
    }
}

static void ExitBootServices()
{
    EFI_STATUS status = ST->BootServices->ExitBootServices(efi.ih, efi_mm.MapKey);

    if(status != EFI_SUCCESS) {
        AbortBoot(L"ExitBootServices failed", status);
    }

    // NOTE: From now on, printing crashes system (ConOut is NULL)
}

static void RemapMemory(void)
{
    EFI_STATUS status = efi.st->RuntimeServices->SetVirtualAddressMap(
        efi_mm.MemoryMapSize,
        efi_mm.DescriptorSize,
        efi_mm.DescriptorVersion,
        (EFI_MEMORY_DESCRIPTOR *) efi_mm.MemoryMapBuffer);

    if(status != EFI_SUCCESS) {
        // AbortBoot(L"SetVirtualAddressMap failed", status);
    }
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

static void PrintMemoryMap(void)
{
    UINT64 MemoryMapSize = MEMORY_MAP_BUFFER_SIZE;
    UINT64 MapKey;
    UINT64 DescriptorSize;
    UINT32 DescriptorVersion;
    EFI_STATUS status;
    EFI_MEMORY_DESCRIPTOR *Md;
    UINT64 i;

    status = efi.st->BootServices->GetMemoryMap(
        &MemoryMapSize,
        (EFI_MEMORY_DESCRIPTOR *) efi_mm.MemoryMapBuffer,
        &MapKey,
        &DescriptorSize,
        &DescriptorVersion);

    PRINT_U64(MemoryMapSize);
    PRINT_U64(MapKey);
    PRINT_U64(DescriptorSize);
    PRINT_U64(DescriptorVersion);

    MemoryMapSize = 64;
    Md = (EFI_MEMORY_DESCRIPTOR *) efi_mm.MemoryMapBuffer; 
    for(i = 0; i < MemoryMapSize; ++i, Md = NextMd(Md, DescriptorSize)) {
        PrintMemoryType(Md->Type);
        Print(L";PS=%lx;VS=%lx;NoP=%ld;EMRT=%d\r\n",
            Md->PhysicalStart,
            Md->VirtualStart,
            Md->NumberOfPages,
            !!(Md->Attribute & EFI_MEMORY_RUNTIME));
    }

    PrintStatus(status);
}

static void CopyKernelImage(void)
{
    size_t count = ((size_t) &_binary_kernel_img_end) - ((size_t) &_binary_kernel_img_start);
    memcpy((void*) KERNEL_PA, &_binary_kernel_img_start, count);
}

static void StartKernel(void)
{
    KernelStart start = (KernelStart) KERNEL_PA;
    KernelParams kernel_params;
    kernel_params.magic = 0xABCD;
    start(&kernel_params);
}

static void crash() {
    typedef void (*F)(void);
    F f = (F) 0x0;
    f();
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    efi.ih = ImageHandle;
    efi.st = SystemTable;

    InitializeLib(ImageHandle, SystemTable); // Initialize GNU EFI
    Print(L"jtos 0.0.1 alpha\r\n");

    Gfx gfx;
    init_gfx(&gfx);

    // GetMemoryMap();
    // ExitBootServices();

    // CopyKernelImage();
    // StartKernel();

    return EFI_SUCCESS;
}
