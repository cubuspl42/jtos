#include <stdbool.h>
#include <stdlib.h>

#include "serial.h"

#define SERIAL_REGISTER_BASE 0x03F8
#define SERIAL_BAUD_RATE 115200
#define SERIAL_CLOCK_RATE 1843200
#define SERIAL_LINE_CONTROL 0x03
#define SERIAL_REGISTER_STRIDE 1
#define SERIAL_FIFO_CONTROL 0x07
#define SERIAL_EXTENDED_FIFO_TX_SIZE 64
#define SERIAL_USE_HW_FLOW_CONTROL false
#define SERIAL_DETECT_CABLE false

// 16550 UART register offsets and bitfields
#define R_UART_RXBUF          0
#define R_UART_TXBUF          0
#define R_UART_BAUD_LOW       0
#define R_UART_BAUD_HIGH      1
#define R_UART_FCR            2
#define   B_UART_FCR_FIFOE    (1 << 0)
#define   B_UART_FCR_FIFO64   (1 << 5)
#define R_UART_LCR            3
#define   B_UART_LCR_DLAB     (1 << 7)
#define R_UART_MCR            4
#define   B_UART_MCR_DTRC     (1 << 0)
#define   B_UART_MCR_RTS      (1 << 1)
#define R_UART_LSR            5
#define   B_UART_LSR_RXRDY    (1 << 0)
#define   B_UART_LSR_TXRDY    (1 << 5)
#define   B_UART_LSR_TEMT     (1 << 6)
#define R_UART_MSR            6
#define   B_UART_MSR_CTS      (1 << 4)
#define   B_UART_MSR_DSR      (1 << 6)
#define   B_UART_MSR_RI       (1 << 7)
#define   B_UART_MSR_DCD      (1 << 8)

void *memset(void *dest, int e, unsigned long len) {
    uint8_t *d = dest;
    for(uint64_t i = 0; i < len; i++, d++) {
        *d = e;
    }
    return dest;
}

uint64_t kstrlen(const char *data) {
    uint64_t r;
    for(r = 0; *data != 0; data++, r++);
    return r;
}

int init_serial_port();

int init_serial()
{
    return init_serial_port();
}


static inline uint8_t port_inb(uint16_t port)
{
    uint8_t data;
    __asm__ volatile("inb %w1,%b0" : "=a" (data) : "d"(port));
    return data;
}

static inline uint8_t port_outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %b0,%w1" : : "a" (value), "d"(port));
    return value;
}

uint8_t read_serial_register(uint16_t offset)
{
    return port_inb(SERIAL_REGISTER_BASE + offset * SERIAL_REGISTER_STRIDE);
}

void write_serial_register(uint16_t offset, uint8_t d)
{
    port_outb(SERIAL_REGISTER_BASE + offset * SERIAL_REGISTER_STRIDE, d);
}

int init_serial_port()
{
    // Calculate divisor for baud generator
    //    Ref_Clk_Rate / Baud_Rate / 16
    uint32_t divisor = SERIAL_CLOCK_RATE / (SERIAL_BAUD_RATE * 16);
    if((SERIAL_CLOCK_RATE % (SERIAL_BAUD_RATE * 16)) >= SERIAL_BAUD_RATE * 8) {
        divisor++;
    }
    // See if the serial port is already initialized
    bool initialized = true;
    if ((read_serial_register(R_UART_LCR) & 0x3F)
        != (SERIAL_LINE_CONTROL & 0x3F)) {
        initialized = false;
    }
    // Calculate current divisor
    write_serial_register(R_UART_LCR,
        read_serial_register(R_UART_LCR) | B_UART_LCR_DLAB);
    uint32_t current_divisor = read_serial_register(R_UART_BAUD_HIGH) << 8;
    current_divisor |= (uint32_t)read_serial_register(R_UART_BAUD_LOW);
    write_serial_register(R_UART_LCR,
        read_serial_register(R_UART_LCR) & ~B_UART_LCR_DLAB);
    // Fast path
    initialized = initialized && current_divisor == divisor;
    if (initialized) {
        return 1;
    }
    // Wait for the serial port to be ready.
    // Verify that both the transmit FIFO and the shift register are empty.
    while ((read_serial_register(R_UART_LSR)
                & (B_UART_LSR_TEMT | B_UART_LSR_TXRDY))
            != (B_UART_LSR_TEMT | B_UART_LSR_TXRDY));
    // Configure baud rate
    write_serial_register(R_UART_LCR, B_UART_LCR_DLAB);
    write_serial_register(R_UART_BAUD_HIGH, (uint8_t) (divisor >> 8));
    write_serial_register(R_UART_BAUD_LOW, (uint8_t) (divisor & 0xff));
    // Clear DLAB and configure Data Bits, Parity, and Stop Bits.
    // Strip reserved bits from PcdSerialLineControl
    write_serial_register(R_UART_LCR, (uint8_t)(SERIAL_LINE_CONTROL & 0x3F));
    // Enable and reset FIFOs
    // Strip reserved bits from PcdSerialFifoControl
    write_serial_register(R_UART_FCR, 0x00);
    write_serial_register(R_UART_FCR,
                          (uint8_t)(SERIAL_LINE_CONTROL & (B_UART_FCR_FIFOE | B_UART_FCR_FIFO64)));
    // Put Modem Control Register(MCR) into its reset state of 0x00.
    write_serial_register(R_UART_MCR, 0x00);
    return 1;
}

bool serial_port_writable ()
{
    if (SERIAL_USE_HW_FLOW_CONTROL) {
        if (SERIAL_DETECT_CABLE) {
            // Wait for both DSR and CTS to be set
            //   DSR is set if a cable is connected.
            //   CTS is set if it is ok to transmit data
            //
            //   DSR  CTS  Description                               Action
            //   ===  ===  ========================================  ========
            //    0    0   No cable connected.                       Wait
            //    0    1   No cable connected.                       Wait
            //    1    0   Cable connected, but not clear to send.   Wait
            //    1    1   Cable connected, and clear to send.       Transmit
            return (bool)((read_serial_register(R_UART_MSR) & (B_UART_MSR_DSR | B_UART_MSR_CTS))
                           == (B_UART_MSR_DSR | B_UART_MSR_CTS));
        } else {
            // Wait for both DSR and CTS to be set OR for DSR to be clear.
            //   DSR is set if a cable is connected.
            //   CTS is set if it is ok to transmit data
            //
            //   DSR  CTS  Description                               Action
            //   ===  ===  ========================================  ========
            //    0    0   No cable connected.                       Transmit
            //    0    1   No cable connected.                       Transmit
            //    1    0   Cable connected, but not clear to send.   Wait
            //    1    1   Cable connected, and clar to send.        Transmit
            return (bool)((read_serial_register(R_UART_MSR) & (B_UART_MSR_DSR | B_UART_MSR_CTS))
                           != (B_UART_MSR_DSR));
        }
    }
    return true;
}


uint64_t serial_port_write(uint8_t *buffer, uint64_t size)
{
    if (buffer == NULL) { return 0; }
    if (size == 0) {
        // Flush the hardware
        //
        // Wait for both the transmit FIFO and shift register empty.
        while ((read_serial_register(R_UART_LSR) & (B_UART_LSR_TEMT | B_UART_LSR_TXRDY))
                != (B_UART_LSR_TEMT | B_UART_LSR_TXRDY));
        while (!serial_port_writable());
        return 0;
    }
    // Compute the maximum size of the Tx FIFO
    uint64_t fifo_size = 1;
    if ((SERIAL_FIFO_CONTROL & B_UART_FCR_FIFOE) != 0) {
        if ((SERIAL_FIFO_CONTROL & B_UART_FCR_FIFO64) == 0) {
            fifo_size = 16;
        } else {
            fifo_size = SERIAL_EXTENDED_FIFO_TX_SIZE;
        }
    }
    while (size != 0) {
        // Wait for the serial port to be ready, to make sure both the transmit FIFO
        // and shift register empty.
        while ((read_serial_register(R_UART_LSR) & B_UART_LSR_TEMT) == 0);
        // Fill then entire Tx FIFO
        for (uint64_t index = 0; index < fifo_size && size != 0; index++, size--, buffer++) {
            // Wait for the hardware flow control signal
            while (!serial_port_writable());
            // Write byte to the transmit buffer.
            write_serial_register(R_UART_TXBUF, *buffer);
        }
    }
    return size;
}

void serial_print(const char *print) {
    serial_port_write((uint8_t *)print, kstrlen(print));
}

void serial_print_int(uint64_t n) {
    char buf[24] = {0}, *bp = buf + 24;
    do {
        bp--;
        *bp = '0' + n % 10;
        n /= 10;
    } while (n != 0);
    serial_port_write((uint8_t *)bp, buf - bp + 24);
}

void serial_print_hex(uint64_t n) {
    char buf[16], *bp = buf + 16;
    for(int i = 0; i < 16; i++) buf[i] = '0';
    do {
        bp--;
        uint8_t mod = n % 16;
        if(mod < 10) {
            *bp = '0' + mod;
        } else {
            *bp = 'A' - 10 + mod;
        }
        n /= 16;
    } while (n != 0);
    serial_port_write((uint8_t *)buf, 16);
}
