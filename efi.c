#include "efi.h"

Efi efi;

void PrintStatus(EFI_STATUS status)
{
    #define CASE(x) case x: Print(L"status = "  L ## #x "\r\n"); break;
    switch(status) {
    CASE(EFI_SUCCESS)
    CASE(EFI_BUFFER_TOO_SMALL)
    CASE(EFI_OUT_OF_RESOURCES)
    CASE(EFI_INVALID_PARAMETER)
    CASE(EFI_NOT_FOUND)
    default: Print(L"status = <unknown>\r\n");
    }
    #undef CASE
}

void AbortBoot(const wchar_t *error, EFI_STATUS status)
{
    Print(L"error: %s\r\n", error);
    PrintStatus(status);
    Print(L"Aborting boot... \r\n", error);
    efi.st->BootServices->Exit(efi.ih, 1, 0, NULL);
}
