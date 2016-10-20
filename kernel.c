#include "kernel.h"

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

static void put_pixel(Framebuffer *fb, int w, int h, uint32_t px)
{
    int32_t *fbb = (int32_t *) fb->base;
    int32_t *pxa = fbb + h * fb->pitch + w;
    *pxa = px;
}

#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif
void kernel_main(KernelParams *kernel_params) {
	/* Initialize terminal interface */
	// terminal_initialize();

	/* Newline support is left as an exercise. */
	// terminal_writestring("Hello, kernel World!\n");

	for(int i = 0; i < 64; ++i)
		for(int j = 0; j < 64; ++j)
			put_pixel(&kernel_params->efi_fb, i, j, 0xFFFFFFFF);

	// crash();

	for(;;);
}
