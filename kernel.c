#include "kernel.h"
#include "console.h"
#include "serial.h"

/* Surely you will remove the processor conditionals and this comment
   appropriately depending on whether or not you use C++. */
#if !defined(__cplusplus)
#include <stdbool.h> /* C doesn't have booleans by default. */
#endif
#include <stddef.h>
#include <stdint.h>

static void crash() {
    typedef void (*F)(void);
    F f = (F) 0x0;
    f();
}

extern char _binary_font_pbm_start;

#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif
void kernel_main(KernelParams *kernel_params)
{
	init_serial();
	console_init(&kernel_params->efi_fb);
	console_print("hello, world");

	for(;;);
}
