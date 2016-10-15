#include <efi.h>
#include <efilib.h>
#include <stdbool.h>
#include <stdint.h>

#define MEMORY_MAP_BUFFER_SIZE 1024 * 1024

EFI_SYSTEM_TABLE *SysTab;
char MemoryMapBuffer[MEMORY_MAP_BUFFER_SIZE];

static void PrintStatus(EFI_STATUS status)
{
    if(status == EFI_SUCCESS) Print(L"EFI_SUCCESS\n\r");
    else if(status == EFI_BUFFER_TOO_SMALL) Print(L"EFI_BUFFER_TOO_SMALL\n\r");
    else if(status == EFI_OUT_OF_RESOURCES) Print(L"EFI_OUT_OF_RESOURCES\n\r");
    else if(status == EFI_INVALID_PARAMETER) Print(L"EFI_INVALID_PARAMETER\n\r");
    else Print(L"<UNKNOWN EFI ERROR>\n\r");
}

static void PrintMemoryMap(void)
{
    UINT64 MemoryMapSize = MEMORY_MAP_BUFFER_SIZE;
    UINT64 MapKey;
    UINT64 DescriptorSize;
    UINT32 DescriptorVersion;
    EFI_STATUS status;
    UINT64 i;
    char *p;

    status = SysTab->BootServices->GetMemoryMap(
        &MemoryMapSize, (EFI_MEMORY_DESCRIPTOR *) MemoryMapBuffer, &MapKey, &DescriptorSize, &DescriptorVersion);

    Print(L"DescriptorSize=%ld;sizeof(EFI_MEMORY_DESCRIPTOR)=%d\n\r\n\r", DescriptorSize, sizeof(EFI_MEMORY_DESCRIPTOR));
    Print(L"MemoryMapSize=%d;MapKey=%d;DescriptorSize=%d;DescriptorVersion=%d\n\r\n\r", MemoryMapSize, MapKey, DescriptorSize, DescriptorVersion);

    p = MemoryMapBuffer;
    for(i = 0; i < MemoryMapSize; ++i) {
        const EFI_MEMORY_DESCRIPTOR *Md = (const EFI_MEMORY_DESCRIPTOR *) p;
        if(Md->PhysicalStart != 0) {
            Print(L"PS=%lx;VS=%lx;NoP=%ld;EFI_MEMORY_RUNTIME=%d\n\r",
                Md->PhysicalStart, Md->VirtualStart, Md->NumberOfPages, !!(Md->Attribute & EFI_MEMORY_RUNTIME));
        }
        p += DescriptorSize;
    }

    PrintStatus(status);
}

EFI_STATUS
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut;

    SysTab = SystemTable;
    ConOut = SystemTable->ConOut;
    InitializeLib(ImageHandle, SystemTable);
    ConOut->OutputString(ConOut, L"jtos 0.0.1 alpha\n\r");

    PrintMemoryMap();

    return EFI_SUCCESS;
}
