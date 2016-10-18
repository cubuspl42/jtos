#include <efi.h>
#include <efilib.h>
#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

#define MEMORY_MAP_BUFFER_SIZE 512 * 1024 // 512 KiB

#define PRINT_U64(x) Print(L ## #x L" = %lx\r\n", (UINT64) x)

EFI_HANDLE IH;
EFI_SYSTEM_TABLE *ST;

#define KERNEL_PA 0x1000000 // 1MiB
#define KERNEL_VIRTUAL_BASE 0x8000000000000000 // ~0 / 2
#define KERNEL_VA (KERNEL_PA + KERNEL_VIRTUAL_BASE)
#define EFI_VIRTUAL_BASE 0xc000000000000000 // (~0 / 4) * 3

typedef struct {
    char MemoryMapBuffer[MEMORY_MAP_BUFFER_SIZE];
    UINT64 MemoryMapSize;
    UINT64 MapKey;
    UINT64 DescriptorSize;
    UINT32 DescriptorVersion;
} MemoryMap;

MemoryMap efi_mm = {0};

static void PrintStatus(EFI_STATUS status)
{
    #define CASE(x) case x: Print(L"status = "  L ## #x "\r\n"); break;
    switch(status) {
    CASE(EFI_SUCCESS)
    CASE(EFI_BUFFER_TOO_SMALL)
    CASE(EFI_OUT_OF_RESOURCES)
    CASE(EFI_INVALID_PARAMETER)
    default: Print(L"status = <unknown>\r\n");
    }
    #undef CASE
}

static void AbortBoot(const wchar_t *error, EFI_STATUS status)
{
    Print(L"error: %s\r\n", error);
    PrintStatus(status);
    Print(L"Aborting boot... \r\n", error);
    ST->BootServices->Exit(IH, 1, 0, NULL);
}

static void GetMemoryMap(void)
{
    Print(L"GetMemoryMap...\r\n");

    efi_mm.MemoryMapSize = MEMORY_MAP_BUFFER_SIZE;

    EFI_STATUS status = ST->BootServices->GetMemoryMap(
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
    EFI_STATUS status = ST->BootServices->ExitBootServices(IH, efi_mm.MapKey);

    if(status != EFI_SUCCESS) {
        AbortBoot(L"ExitBootServices failed", status);
    }

    // NOTE: From now on, printing crashes system (ConOut is NULL)
}

static void RemapMemory(void)
{
    EFI_STATUS status = ST->RuntimeServices->SetVirtualAddressMap(
        efi_mm.MemoryMapSize,
        efi_mm.DescriptorSize,
        efi_mm.DescriptorVersion,
        (EFI_MEMORY_DESCRIPTOR *) efi_mm.MemoryMapBuffer);

    if(status != EFI_SUCCESS) {
        // AbortBoot(L"SetVirtualAddressMap failed", status);
    }
}

/* Hardware text mode color constants. */
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

static void StartKernel(void)
{
    for(;;);
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

    status = ST->BootServices->GetMemoryMap(
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

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    IH = ImageHandle;
    ST = SystemTable;

    InitializeLib(ImageHandle, SystemTable);
    Print(L"jtos 0.0.1 alpha\r\n");
    PRINT_U64(ImageHandle);
    PRINT_U64(SystemTable);

    // PrintMemoryMap();

    GetMemoryMap();
    ExitBootServices();
    // RemapMemory();
    StartKernel();

    return EFI_SUCCESS;
}
