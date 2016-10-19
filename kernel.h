#include <efi.h>

typedef struct {
	int magic;
} KernelParams;

typedef void (*KernelStart)(KernelParams *);
