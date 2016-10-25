#pragma once
#include "common.h"

typedef struct {
	uint16_t offset: 12;
	uint16_t pt: 9;
	uint16_t pd: 9;
	uint16_t pdpt: 9;
	uint16_t pml4: 9;
	uint16_t reserved: 16;
} PACKED LinAddr;

_Static_assert(sizeof(LinAddr) == sizeof(uint64_t), "wrong size of LinAddr structure");
