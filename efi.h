#include <efi.h>
#include <wchar.h>

typedef struct {
	EFI_HANDLE ih; // Loader image handle
	EFI_SYSTEM_TABLE *st;
} Efi;

extern Efi efi;

void PrintStatus(EFI_STATUS status);

void AbortBoot(const wchar_t *error, EFI_STATUS status);

#define EFI_CHECK(expr) \
do { \
	EFI_STATUS __status = (expr); \
	Print(L ## #expr L"\r\n"); \
	PrintStatus(__status); \
} while(0)
