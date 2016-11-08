#include "kernel.h"
#include "serial.h"

extern char _binary_kernel1_img_start;

static void start_kernel1(char *kernel1_img_pa, const KernelParams *params)
{
	// char *kernel1_img_va = KERNEL_VIRTUAL_BASE + kernel1_img_pa;
	// serial_print_mem(kernel1_img_pa, 8);
	// serial_print("\r\n");
	// serial_print_mem(kernel1_img_va, 8);
	// serial_print("\r\n");
	serial_print_mem((void *) 0xffff800000000080, 32);
	// serial_print("\r\n");

	// for(;;);
	// serial_print(">> kernel1_img_va\r\n");
	// KernelStart start = (KernelStart) kernel1_img_va;
	// KernelStart start = (KernelStart) KERNEL_VIRTUAL_BASE;
	KernelStart start = (KernelStart) 0xffff800000000080;
	SERIAL_DUMP_HEX(start);
	// for(;;);
	start(params);
}

static inline uint64_t read_cr0()
{
	uint64_t value;
	__asm__("movq %%cr0, %0" : "=r"(value));
	return value;
}

static inline uint64_t read_cr4()
{
	uint64_t value;
	__asm__("movq %%cr4, %0" : "=r"(value));
	return value;
}

const uint32_t IA32_EFER_MSR = 0xC0000080;

static uint64_t
cpu_read_msr(uint32_t msr) {
    uint32_t a, d;
    __asm__ volatile("rdmsr" : "=a"(a), "=d"(d) : "c"(msr));
    return ((uint64_t)d) << 32 | a;
}

static uint64_t read_efer() {
    return cpu_read_msr(IA32_EFER_MSR);
}

void kernel_start0(KernelParams *_params)
{
	serial_print("> kernel_start0\r\n");
	// for(;;);

	extern char *_kernel0_end;
	uint64_t kernel0_end = (uint64_t) &_kernel0_end;
	uint64_t kernel0_size = kernel0_end - KERNEL_PA;
	char *kernel1_img = (char *) &_binary_kernel1_img_start;

	SERIAL_DUMP_HEX(kernel0_size);
	SERIAL_DUMP_BITS(read_cr0());
	SERIAL_DUMP_BITS(read_cr4());
	SERIAL_DUMP_BITS(read_efer());

	KernelParams params = *_params; // preserve params

	SERIAL_DUMP_HEX(params.fb.base);

	serial_print(">> enable_paging\r\n");
	enable_paging(kernel1_img, &params.efi_mm, &params.fb);
	serial_print("<< enable_paging\r\n");

	serial_print_mem((void *) KERNEL_PA, 4);
	SERIAL_DUMP_HEX(KERNEL_VIRTUAL_BASE);
	// for(;;);
	serial_print_mem((void *) KERNEL_VIRTUAL_BASE, 4);
	// for(;;);

	params.fb.base = (void *) FRAMEBUFFER_VIRTUAL_BASE;

	SERIAL_DUMP_HEX(kernel1_img);
	serial_print(">> start_kernel1\r\n");
	start_kernel1(kernel1_img, &params);
}
