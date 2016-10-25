#pragma once

#include "framebuffer.h"

void console_init(Framebuffer * _framebuffer);
void console_print(const char *str);
void console_print_u64(uint64_t v);
void console_print_ptr(const void *ptr);
void console_print_linaddr(const void *addr);
void console_print_mem(const void *mem, int n);
