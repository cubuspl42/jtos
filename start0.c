#include "kernel.h"
#include "serial.h"

extern char _binary_kernel1_img_start;

static char stack[64 * 1024]; // 64 KiB

static void start_kernel1(void *kernel1_img, const KernelParams *params)
{
	KernelStart start = (KernelStart) kernel1_img;
	start(params);
}

void kernel_start0(KernelParams *_params)
{
	KernelParams params = *_params; // preserve params

	enable_paging(&params.efi_mm, &params.fb);
	serial_print("> kernel_startup\r\n");

	void *kernel1_img = (void *) &_binary_kernel1_img_start;
	start_kernel1(kernel1_img, &params);
}
