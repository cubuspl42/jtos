#include <stdint.h>

#include "paging.h"
#include "address.h"
#include "common.h"
#include "console.h"
#include "efi.h"
#include "framebuffer.h"
#include "pagealloc.h"
#include "serial.h"

/**
 * Page Table size
 *
 * = PD size = PDPT size = PML4 size
 */
#define PAGE_TABLE_SIZE 512

/**
 * PE flags; Page Entry flags
 *
 * Page Table Entry; PTE (-> 4k) /
 * Page Directory Entry; PDE (-> PT) /
 * Page Directory Pointer Table Entry; PDPTE (-> PD) /
 * Page Mapping Level 4 Entry; PML4E (-> PDPT)
 */
typedef struct {
	/**
	 * P; Present
	 */
	uint8_t p : 1;
	/**
	 * R/W; Read/write
	 */
	uint8_t rw : 1;
	/**
	 * U/S; User/supervisor
	 */
	uint8_t us : 1;
	/**
	 * PWT; Page-level write-through
	 */
	uint8_t pwt : 1;
	/**
	 * PCD; Page-level cache disable
	 */
	uint8_t pcd : 1;
	/**
	 * A; Accessed
	 */
	uint8_t a : 1;
	/**
	 * PTE: D; Dirty
	 * PDE/PDPTE/PML4E: Ignored
	 */
	uint8_t d : 1;
	/**
	 * PTE: PAT
	 * PDE/PDPTE/PML4E: PS; Page size
	 */
	uint8_t pat_ps : 1;
	/**
	 * PTE: G; Global
	 * PDE/PDPTE/PML4E: Ignored
	 */
	uint8_t g : 1;
	/**
	 * Ignored
	 */
	uint8_t ignored_0 : 3;
	/**
	 * Physical address
	 */
	uint64_t phy : 40;
	/**
	 * Ignored
	 */
	uint8_t ignored_1 : 7;
	/**
	 * PTE: Protection key
	 * PDE/PDPTE/PML4E: Ignored
	 */
	uint8_t pk : 4;
	/**
	 * XD; Execution-disable
	 */
	uint8_t xd : 1;
} PACKED Pe;

_Static_assert(sizeof(Pe) == sizeof(uint64_t), "wrong size of Pe structure");

Pe pml4[PAGE_TABLE_SIZE] PAGE_ALIGNED;

static inline uint64_t read_cr3()
{
	uint64_t value;
	__asm__("movq %%cr3, %0" : "=r"(value));
	return value;
}

static inline void write_cr3(uint64_t value)
{
	__asm__("movq %0, %%cr3" :: "r"(value));
}

static LinAddr linaddr(uint64_t a)
{
	LinAddr *linaddrp = (LinAddr *) &a;
	return *linaddrp;
}

static void init_pe(Pe *pe, uint64_t ppa)
{
	pe->phy = ppa >> PAGE_SIZE_BITS;
	pe->p = 1;
	pe->rw = 1;
}

// get page map
static void * get_pm(Pe *pt, uint64_t pi) {
	Pe *pe = &pt[pi];
	if(pe->p == 0) {
		void *page = pgalloc();
		init_pe(pe, (uint64_t) page);
		return page;
	} else {
		void *page = (void *)(pe->phy << PAGE_SIZE_BITS);
		return page;
	}
}

static void map_page_pt(Pe pt[], uint64_t vpa, uint64_t ppa)
{
	Pe *pe = &pt[linaddr(vpa).pt];
	// kassert(pe->phy == 0);
	// kassert(pe->phy == 1);
	init_pe(pe, ppa);
}

static void map_page_pd(Pe pd[], uint64_t vpa, uint64_t ppa)
{
	void *pt = get_pm(pd, linaddr(vpa).pd);
	map_page_pt(pt, vpa, ppa);
}

static void map_page_pdpt(Pe pdpt[], uint64_t vpa, uint64_t ppa)
{
	void *pd = get_pm(pdpt, linaddr(vpa).pdpt);
	map_page_pd(pd, vpa, ppa);
}

static void map_page_pml4(Pe pml4[], uint64_t vpa, uint64_t ppa)
{
	void *pdpt = get_pm(pml4, linaddr(vpa).pml4);
	map_page_pdpt(pdpt, vpa, ppa);
}

static void map_region(uint64_t vra, uint64_t pra, int np)
{
	// kassert(vra & OFFSET_MASK == 0)
	// kassert(pra & OFFSET_MASK == 0)
	// 
	serial_print("> map_region\r\n");
	SERIAL_DUMP_HEX(vra);
	SERIAL_DUMP_HEX(pra);
	SERIAL_DUMP_HEX(np);

	uint64_t vrae = vra + np * PAGE_SIZE; // virtual address end
	for(; vra < vrae; vra += PAGE_SIZE, pra += PAGE_SIZE) {
		map_page_pml4(pml4, vra, pra);
	}
}

static void trigger_paging()
{
	serial_print("> trigger_paging\r\n");
	write_cr3((uint64_t) pml4);
	serial_print("< trigger_paging\r\n");
}

static void map_efi_runtime(EfiMemoryMap *mm)
{
	const EFI_MEMORY_DESCRIPTOR *md = mm->memory_map;
	const UINT64 ds = mm->descriptor_size;
	for(int i = 0; i < (int) mm->memory_map_size; ++i, md = next_md(md, ds)) {
		if(md->Attribute & EFI_MEMORY_RUNTIME || md->Type == EfiLoaderCode || md->Type == EfiLoaderData
			// || md->Type == EfiBootServicesData
			) {
			map_region(md->PhysicalStart, md->PhysicalStart, md->NumberOfPages);
		}
	}
}

static void map_framebuffer(Framebuffer *fb)
{
	uint64_t fbb = (uint64_t) fb->base;
	map_region(fbb, fbb, fb->size / PAGE_SIZE + 1);
}

static void print_rip()
{
	a:;
	uint64_t rip = (uint64_t) &&a; // ~
	SERIAL_DUMP_HEX(rip);
}

static void ind(int ilv)
{
	for(int i = 0; i < ilv; ++i) {
		serial_print("    ");
	}
}

static void indlv(int lv)
{
	int i = 4 - lv;
	ind(i);
}

static void sp(void)
{
	serial_print(" ");
}

static void print_pe(Pe *pe, int lv);

static void print_n(int lv, int *n) {
	if(*n) {
		int i = 4 - lv;
		ind(i);
		serial_print("[");
		serial_print_int(*n);
		serial_print("]\r\n");
		*n = 0;
	}
}

static void print_pm(Pe pm[], int lv) {
	indlv(lv);
	switch(lv) {
		case 4: serial_print("PML4"); break;
		case 3: serial_print("PDPT"); break;
		case 2: serial_print("__PD"); break;
		case 1: serial_print("__PT"); break;
		case 0: serial_print("PAGE"); break;
	}
	serial_print("#");
	serial_print_ptr(pm);
	serial_print("\r\n");

	int n = 0;
	for(int i = 0; i < PAGE_TABLE_SIZE; ++i) {
		Pe *pe = &pm[i];
		if(pe->p) {
			print_n(lv, &n);
			print_pe(&pm[i], lv);
		} else {
			++n;
		}
	}
	print_n(lv, &n);
}

static void print_pe(Pe *pe, int lv)
{
	if(pe->p) {
		indlv(lv);
		switch(lv) {
			case 4: serial_print("PML4e"); break;
			case 3: serial_print("PDPTe"); break;
			case 2: serial_print("__PDe"); break;
			case 1: serial_print("__PTe"); break;
			case 0: serial_print("PAGEe"); break;
		}
		serial_print("@");
		serial_print_ptr(pe);
		serial_print(" ");

		SERIAL_DUMP_HEX0(pe->rw); sp(); SERIAL_DUMP_HEX(pe->phy);
	} else {
		serial_print(".");
	}
	#if 1
	if(pe->p && lv > 1) {
		Pe *pm = (Pe *)(pe->phy << PAGE_SIZE_BITS);
		print_pm(pm, lv - 1);
	}
	#endif
}

void enable_paging(EfiMemoryMap *mm, Framebuffer *fb)
{
	map_region(KERNEL_PA, KERNEL_PA, 256);
	map_efi_runtime(mm);
	map_framebuffer(fb);


	// map_region(0, 0, (256 * 1024 * 1024) >> PAGE_SIZE_BITS);

	SERIAL_DUMP_HEX((uint64_t) pml4);
	print_rip();

	// print_pm(pml4, 4);

	// for(;;);
	trigger_paging();
}
