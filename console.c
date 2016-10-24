#include <stdint.h>

#include "console.h"
#include "serial.h"
#include "framebuffer.h"

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

static void blt_char(int dx, int dy, uint8_t c) {
	int i = c / FONT_MATRIX_WIDTH;
	int j = c % FONT_MATRIX_HEIGHT;
	int ipx = i * FONT_CHAR_HEIGHT;
	int jpx = j * FONT_CHAR_WIDTH;
	blt(dx, dy, jpx, ipx, FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT, &fontbm);
}

static void print_char(uint8_t c) {
	if(c == '\n') {
		console_i = (console_i + 1) % console_h;
		console_j = 0;
	} else {
		blt_char(console_j * FONT_CHAR_WIDTH, console_i * FONT_CHAR_HEIGHT, c);
		++console_j;
	}
}

void console_init(Framebuffer *_framebuffer)
{
	framebuffer = _framebuffer;

	fontbm.buffer = (uint8_t *)(&_binary_font_ppm_start + FONT_PPM_HEADER_SIZE);
	fontbm.width = FONT_PPM_WIDTH;
	fontbm.height = FONT_PPM_HEIGHT;

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
