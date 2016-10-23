#pragma once
#include <stdint.h>

#define SERIAL_DUMP_HEX(expr) \
serial_print(#expr " = "); \
serial_print_hex((uint64_t) (expr)); \
serial_print("\r\n"); \

int init_serial();

uint64_t serial_port_write(uint8_t *buffer, uint64_t size);

void serial_print(const char *print);

void serial_print_int(uint64_t n);

void serial_print_hex(uint64_t n);
