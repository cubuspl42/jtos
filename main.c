#include <efi.h>
#include <efilib.h>
#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

#define MEMORY_MAP_BUFFER_SIZE 512 * 1024 // 1 MiB

#define PRINT_VAR_U64(x) Print(L ## #x L" = %lx\r\n", (UINT64) x)

EFI_HANDLE IH;
EFI_SYSTEM_TABLE *ST;

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
    // Print(L"GetMemoryMap...\r\n");

    efi_mm.MemoryMapSize = MEMORY_MAP_BUFFER_SIZE;

    EFI_STATUS status = ST->BootServices->GetMemoryMap(
        &efi_mm.MemoryMapSize,
        (EFI_MEMORY_DESCRIPTOR *) efi_mm.MemoryMapBuffer,
        &efi_mm.MapKey,
        &efi_mm.DescriptorSize,
        &efi_mm.DescriptorVersion);

    // Print(L"MapKey = %lx\r\n", efi_mm.MapKey);

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
}

static void RemapMemory(void)
{
    EFI_STATUS status = ST->RuntimeServices->SetVirtualAddressMap(
        efi_mm.MemoryMapSize,
        efi_mm.DescriptorSize,
        efi_mm.DescriptorVersion,
        (EFI_MEMORY_DESCRIPTOR *) efi_mm.MemoryMapBuffer);

    if(status != EFI_SUCCESS) {
        AbortBoot(L"SetVirtualAddressMap failed", status);
    }
}

static void StartKernel(void)
{
    for(;;);
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    IH = ImageHandle;
    ST = SystemTable;

    InitializeLib(ImageHandle, SystemTable);
    Print(L"jtos 0.0.1 alpha\r\n");
    PRINT_VAR_U64(ImageHandle);
    PRINT_VAR_U64(SystemTable);

    GetMemoryMap();
    ExitBootServices();
    RemapMemory();
    StartKernel();

    return EFI_SUCCESS;
}
