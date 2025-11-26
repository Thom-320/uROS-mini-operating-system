#include "uros.h"

// NS16550A register offsets relative to UART_BASE
#define UART_RBR 0 // Receiver Buffer Register (read)
#define UART_THR 0 // Transmit Holding Register (write)
#define UART_IER 1 // Interrupt Enable Register
#define UART_IIR 2 // Interrupt Identification Register (read)
#define UART_FCR 2 // FIFO Control Register (write)
#define UART_LCR 3 // Line Control Register
#define UART_MCR 4 // Modem Control Register
#define UART_LSR 5 // Line Status Register
#define UART_MSR 6 // Modem Status Register
#define UART_SCR 7 // Scratch Register

#define LSR_DR (1 << 0)   // Data Ready
#define LSR_THRE (1 << 5) // Transmitter Holding Register Empty

static inline u8 uart_reg_read(int offset) {
  return *(volatile u8 *)((u64)UART_BASE + offset);
}

static inline void uart_reg_write(int offset, u8 value) {
  *(volatile u8 *)((u64)UART_BASE + offset) = value;
}

void uart_init(void) {
  // Disable UART interrupts and make sure FIFOs start clean
  uart_reg_write(UART_IER, 0x00);
  uart_reg_write(UART_FCR, 0x07);

  // 8 data bits, no parity, 1 stop bit, DLAB cleared
  uart_reg_write(UART_LCR, 0x03);

  // Assert DTR/RTS so QEMU's console stays active
  uart_reg_write(UART_MCR, 0x03);
}

void uart_putc(char c) {
  if (c == '\n') {
    uart_putc('\r');
  }

  while ((uart_reg_read(UART_LSR) & LSR_THRE) == 0)
    ;

  uart_reg_write(UART_THR, (u8)c);
}

int uart_getc(void) {
  if ((uart_reg_read(UART_LSR) & LSR_DR) == 0) {
    return -1;
  }

  return (int)uart_reg_read(UART_RBR);
}

int uart_getc_blocking(void) {
  int ch;
  while ((ch = uart_getc()) < 0) {
    // Yield to let other tasks run while waiting for input
    task_yield();
  }
  return ch;
}

void uart_puts(const char *s) {
  while (*s) {
    uart_putc(*s++);
  }
}

char *uart_gets(char *buf, int maxlen) {
  int i = 0;

  while (i < maxlen - 1) {
    int ch = uart_getc_blocking();

    if (ch == '\r' || ch == '\n') {
      uart_putc('\n');
      break;
    }

    if (ch == 0x7f || ch == '\b') {
      if (i > 0) {
        i--;
        uart_putc('\b');
        uart_putc(' ');
        uart_putc('\b');
      }
      continue;
    }

    buf[i++] = (char)ch;
    uart_putc((char)ch);
  }

  buf[i] = '\0';
  return buf;
}
