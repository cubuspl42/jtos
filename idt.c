#include <stdint.h>

#include "common.h"
#include "console.h"
#include "kernel.h"
#include "serial.h"
#include "keyboard_map.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e

extern void keyboard_handler(void);

struct IDT_entry {
	uint16_t offset_lowerbits : 16;
	uint16_t selector : 16;
	uint8_t zero : 8;
	uint8_t type_attr : 8;
	uint64_t offset_higherbits : 48;
	uint32_t zero_2 : 32;
} PACKED;

_Static_assert(sizeof(struct IDT_entry) == 16, "wrong size of IDT_entry structure");

typedef struct {
	uint16_t limit;
	uint64_t offset;
} PACKED IDTR;

_Static_assert(sizeof(IDTR) == 10, "wrong size of IDTR structure");


struct IDT_entry IDT[IDT_SIZE];

static inline uint8_t read_port(uint16_t port)
{
	uint8_t data;
	__asm__ volatile("inb %w1,%b0" : "=a" (data) : "d"(port));
	return data;
}

static inline uint8_t write_port(uint16_t port, uint8_t value)
{
	__asm__ volatile("outb %b0,%w1" : : "a" (value), "d"(port));
	return value;
}

static inline void load_idt(IDTR *idtr)
{
	__asm__ volatile("lidtq %0" : : "m" (*idtr));
	__asm__ volatile("sti");
}

static inline void store_idt(IDTR *idtr)
{
	__asm__ volatile("sidtq %0" : "=m" (*idtr));
}

static void idte_init(struct IDT_entry *e) {
	uint64_t keyboard_address;
	keyboard_address = ((uint64_t) keyboard_handler); 
	e->offset_lowerbits = keyboard_address & 0xffff;
	e->selector = 0x38; 
	e->zero = 0;
	e->type_attr = 0x8e; /* INTERRUPT_GATE */
	e->offset_higherbits = (keyboard_address & 0xffffffffffff0000) >> 16;
	e->zero_2 = 0;
}

static void print_idtr(IDTR *idtr)
{
	SERIAL_DUMP_HEX(idtr->limit);
	SERIAL_DUMP_HEX(idtr->offset);
	return;
	for(int i = 0; i < IDT_SIZE; ++i) {
		SERIAL_DUMP_HEX(i);
		struct IDT_entry *e = (struct IDT_entry *)(idtr->offset) + i;
		SERIAL_DUMP_HEX(e->offset_higherbits);
		SERIAL_DUMP_HEX(e->offset_lowerbits);
	}
}

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1
#define IRQ1 0x21
#define ICW1 0x11
#define EOI 0x20

void idt_init(void)
{
	idte_init(&IDT[IRQ1]);

	/* ICW1 - begin initialization */
	write_port(PIC1_CMD, ICW1);
	write_port(PIC2_CMD, ICW1);

	/* ICW2 - remap offset address of IDT */
	write_port(PIC1_DATA, 0x20);
	write_port(PIC2_DATA, 0x28);

	/* ICW3 - setup cascading */
	write_port(PIC1_DATA, 0x00);  
	write_port(PIC2_DATA, 0x00);  

	/* ICW4 - environment info */
	write_port(PIC1_DATA, 0x01);
	write_port(PIC2_DATA, 0x01);

	/* mask all interrupts */
	write_port(PIC1_DATA, 0xff);
	write_port(PIC2_DATA, 0xff);

	IDTR idtr;
	idtr.limit = (sizeof (struct IDT_entry) * IDT_SIZE) - 1;
	idtr.offset = (uint64_t) IDT;

	load_idt(&idtr);

	/* 0xFD is 11111101 - enables only IRQ1 (keyboard)*/
	write_port(IRQ1, 0xFD);
}

void keyboard_handler_main(void) {
	unsigned char status;
	char keycode;

	/* write EOI */
	write_port(PIC1_CMD, EOI);

	status = read_port(KEYBOARD_STATUS_PORT);
	if (status & 0x01) {
		keycode = read_port(KEYBOARD_DATA_PORT);
		if(keycode < 0)
			return;

		char buf[] = {keyboard_map[keycode], 0};
		serial_print(buf);
		console_print(buf);
	}
}
