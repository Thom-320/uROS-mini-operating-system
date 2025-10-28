#include "uros.h"

// Compiler-provided stdarg
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

static void print_num(u64 n, int base, int is_signed) {
    char buf[32];
    int i = 0;
    int is_neg = 0;
    
    if (is_signed && (long)n < 0) {
        is_neg = 1;
        n = -(long)n;
    }
    
    if (n == 0) {
        uart_putc('0');
        return;
    }
    
    while (n > 0) {
        int digit = n % base;
        buf[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        n /= base;
    }
    
    if (is_neg) {
        uart_putc('-');
    }
    
    while (i > 0) {
        uart_putc(buf[--i]);
    }
}

void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 's': {
                    const char *s = va_arg(args, const char *);
                    if (s) {
                        uart_puts(s);
                    } else {
                        uart_puts("(null)");
                    }
                    break;
                }
                case 'd': {
                    int n = va_arg(args, int);
                    print_num(n, 10, 1);
                    break;
                }
                case 'u': {
                    u32 n = va_arg(args, u32);
                    print_num(n, 10, 0);
                    break;
                }
                case 'x': {
                    u64 n = va_arg(args, u64);
                    print_num(n, 16, 0);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    uart_putc(c);
                    break;
                }
                case '%': {
                    uart_putc('%');
                    break;
                }
                default:
                    uart_putc('%');
                    uart_putc(*fmt);
                    break;
            }
        } else {
            uart_putc(*fmt);
        }
        fmt++;
    }
    
    va_end(args);
}

// Utility functions
void *memset(void *s, int c, size_t n) {
    u8 *p = s;
    while (n--)
        *p++ = (u8)c;
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    u8 *d = dest;
    const u8 *s = src;
    while (n--)
        *d++ = *s++;
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (*s++)
        len++;
    return len;
}

int atoi(const char *s) {
    int result = 0;
    int sign = 1;
    
    while (*s == ' ' || *s == '\t')
        s++;
    
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s - '0');
        s++;
    }
    
    return result * sign;
}

