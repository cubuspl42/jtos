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

static const EFI_MEMORY_DESCRIPTOR *next_md(
	const EFI_MEMORY_DESCRIPTOR *Md, UINT64 DescriptorSize)
{
	const char *p = ((const char *) Md) + DescriptorSize;
	return (const EFI_MEMORY_DESCRIPTOR *) p;
}

