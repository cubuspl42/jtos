#include <stdint.h>

#include "console.h"
#include "address.h"
#include "framebuffer.h"
#include "serial.h"

#define FONT_PPM_WIDTH 240
#define FONT_PPM_HEIGHT 240
#define FONT_CHAR_WIDTH 15
#define FONT_CHAR_HEIGHT 15
#define FONT_PPM_HEADER_SIZE 15
#define FONT_MATRIX_WIDTH 16
#define FONT_MATRIX_HEIGHT 16

typedef struct {
	uint8_t *buffer;
	int width;
	int height;
} Pixmap;

extern char _binary_font_ppm_start;
extern char _binary_font_ppm_end;
static Framebuffer *framebuffer;
static Pixmap fontbm;
static int console_i = 0;
static int console_j = 0;
static int console_w = 0;
static int console_h = 0;

static uint32_t bmpx(const Pixmap *bm, int x, int y)
{
	union {
		uint8_t b[4];
		uint32_t i;
	} u;
	uint8_t *px = bm->buffer + (y * bm->width + x) * 3;
	u.b[0] = px[0];
	u.b[1] = px[1];
	u.b[2] = px[2];
	u.b[3] = 0xFF;
	return u.i;
}

void putpx(int x, int y, uint32_t px)
{
	int32_t *fbb = (int32_t *) framebuffer->base;
	int32_t *pxa = fbb + y * framebuffer->pitch + x;
	*pxa = px;
}

static void sputpx(int x, int y, uint32_t px)
{
	if(x >= 0 && x < framebuffer->width && y >= 0 && x < framebuffer->height) {
		putpx(x, y, px);
	}
}

static void blt(int dx, int dy, int sx, int sy, int sw, int sh, const Pixmap *bm)
{
	for(int i = 0; i < sh; ++i) {
		for(int j = 0; j < sw; ++j) {
			uint32_t px = bmpx(bm, sx + j, sy + i);
			sputpx(dx + j, dy + i, px);
		}
	}
}

static void blt_char(int di, int dj, uint8_t c) {
	int i = c / FONT_MATRIX_WIDTH;
	int j = c % FONT_MATRIX_HEIGHT;
	int ipx = i * FONT_CHAR_HEIGHT;
	int jpx = j * FONT_CHAR_WIDTH;
	blt(dj * FONT_CHAR_WIDTH, di * FONT_CHAR_HEIGHT,
		jpx, ipx,
		FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT,
		&fontbm);
}

static void clean_line(int i) {
	for(int j = 0; j < console_w; ++j) {
		blt_char(i, j, ' ');
	}
}

static void print_char(uint8_t c) {
	if(c == '\n') {
		console_i = (console_i + 1) % console_h;
		clean_line(console_i);
		console_j = 0;
	} else {
		blt_char(console_i, console_j, c);
		++console_j;
	}
}

void console_init(Framebuffer *_framebuffer)
{
	framebuffer = _framebuffer;

	fontbm.buffer = (uint8_t *)(&_binary_font_ppm_start + FONT_PPM_HEADER_SIZE);
	fontbm.width = FONT_PPM_WIDTH;
	fontbm.height = FONT_PPM_HEIGHT;

	console_w = framebuffer->width / FONT_CHAR_WIDTH;
	console_h = framebuffer->height / FONT_CHAR_HEIGHT;
}

void console_print(const char *str)
{
	const uint8_t *p = (const uint8_t *) str;
	while(*p) {
		print_char(*p);
		++p;
	}
}

static char hex(uint8_t v) {
	if (v >= 0 && v <= 9) {
		return '0' + v;
	} else if(v >= 0xA && v <= 0xF) {
		return 'A' + (v - 0xA);
	} else {
		return '?';
	}
}

static char hexh(uint8_t b)
{
	uint8_t h = b >> 4;
	return hex(h);
}

static char hexl(uint8_t b)
{
	uint8_t l = b & 0x0F;
	return hex(l);
}

void console_print_u8(uint8_t v)
{
	char buf[] = {hexh(v), hexl(v), 0};
	console_print(buf);
}

void console_print_u16(uint16_t v)
{
	const uint8_t *b = (const uint8_t *) &v;
	for(int i = sizeof(v) - 1; i >= 0; --i) {
		char buf[] = {hexh(b[i]), hexl(b[i]), 0};
		console_print(buf);
	}
}

void console_print_u64(uint64_t v)
{
	const uint8_t *b = (const uint8_t *) &v;
	for(int i = sizeof(v) - 1; i >= 0; --i) {
		char buf[] = {hexh(b[i]), hexl(b[i]), 0};
		console_print(buf);
	}
}

void console_print_ptr(const void *ptr)
{
	console_print_u64((uint64_t) ptr);
}

void console_print_byte(uint8_t b)
{
	char buf[] = {'0', 'x', hexh(b), hexl(b), 0};
	console_print(buf);
}

void console_print_mem(const void *mem, int n)
{
	const uint8_t *b = (const uint8_t *) mem;
	for(int i = 0; i < n; ++i) {
		console_print(":");
		console_print_byte(b[i]);
	}
	console_print("\n");
}

void console_print_linaddr(const void *addr)
{
	const LinAddr *linaddr = (const LinAddr *) &addr;
	console_print_u16(linaddr->pml4);
	console_print(":");
	console_print_u16(linaddr->pdpt);
	console_print(":");
	console_print_u16(linaddr->pd);
	console_print(":");
	console_print_u16(linaddr->pt);
	console_print("\n");
}
