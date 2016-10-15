#include <efi.h>
#include <efilib.h>

EFI_STATUS
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut;

    ConOut = SystemTable->ConOut;
    InitializeLib(ImageHandle, SystemTable);
    ConOut->OutputString(ConOut, L"Hello World!!\n\r");

    return EFI_SUCCESS;
}
