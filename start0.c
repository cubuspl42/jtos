#include "kernel.h"
#include "serial.h"

extern char _binary_kernel1_img_start;

static void start_kernel1(const KernelParams *params)
{
	KernelStart start = (KernelStart) KERNEL_VIRTUAL_BASE;
	SERIAL_DUMP_HEX(start);
	start(params);
}

void kernel_start0(KernelParams *_params)
{
	serial_print("> kernel_start0\r\n");

	extern char *_kernel0_end;
	uint64_t kernel0_end = (uint64_t) &_kernel0_end;
	uint64_t kernel0_size = kernel0_end - KERNEL_PA;
	char *kernel1_img = (char *) &_binary_kernel1_img_start;

	KernelParams params = *_params; // preserve params

	serial_print(">> enable_paging\r\n");
	enable_paging(kernel1_img, &params.efi_mm, &params.fb);
	serial_print("<< enable_paging\r\n");

	params.fb.base = (void *) FRAMEBUFFER_VIRTUAL_BASE; // fixup framebuffer

	SERIAL_DUMP_HEX(kernel1_img);
	serial_print(">> start_kernel1\r\n");
	start_kernel1(&params);
}
