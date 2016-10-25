#pragma once
#include <stddef.h>

#define PACKED __attribute__((packed))

void* memcpy(void* dest, const void* src, size_t count);
