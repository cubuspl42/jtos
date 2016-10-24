#include "kernel.h"
#include "console.h"
#include "serial.h"

void kernel_main(KernelParams *kernel_params)
{
	init_serial();
	console_init(&kernel_params->efi_fb);
	console_print("### jtos 0.0.1 alpha ###\n");

	for(;;);
}
