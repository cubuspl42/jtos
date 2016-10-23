#pragma once

#include <efi.h>

typedef struct {
	void *base;
	int width;
	int height;
	int size; // in bytes?
	int pitch; // pixels per scanlines
} Framebuffer;
