#include <am.h>
#include <klib-macros.h>
#include <klib.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
    char c;
    char *str;
    long int num;
    unsigned long int unum;
    int base;
    int neg;
    size_t width;
    int pad_zero;
    size_t len = 0;  // Output length
    size_t pos = 0;  // Current position in output buffer

    for (; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            if (pos < n - 1)
                out[pos++] = *fmt;
            len++;
            continue;
        }

        // Process flags
        pad_zero = 0;
        fmt++;  // Skip '%'

        // Handle zero padding
        if (*fmt == '0') {
            pad_zero = 1;
            fmt++;
        }

        // Get field width
        width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        // Handle conversion specifiers
        switch (*fmt) {
            case 's':
                str = va_arg(ap, char *);
                if (str == NULL)
                    str = "(null)";
                while (*str) {
                    if (pos < n - 1)
                        out[pos++] = *str;
                    len++;
                    str++;
                }
                break;

            case 'd':
            case 'i':
                num = va_arg(ap, int);
                if (num < 0) {
                    if (pos < n - 1)
                        out[pos++] = '-';
                    len++;
                    num = -num;
                    neg = 1;
                } else {
                    neg = 0;
                }
                base = 10;
                goto number;

            case 'u':
                unum = va_arg(ap, unsigned int);
                base = 10;
                neg = 0;
                goto unsigned_number;

            case 'x':
            case 'X':
                unum = va_arg(ap, unsigned int);
                base = 16;
                neg = 0;
                goto unsigned_number;

            case 'p':
                if (pos + 2 < n - 1) {
                    out[pos++] = '0';
                    out[pos++] = 'x';
                }
                len += 2;
                unum = (unsigned long int)va_arg(ap, void *);
                base = 16;
                neg = 0;
                goto unsigned_number;

            case 'c':
                c = (char)va_arg(ap, int);
                if (pos < n - 1)
                    out[pos++] = c;
                len++;
                break;

            case '%':
                if (pos < n - 1)
                    out[pos++] = '%';
                len++;
                break;

            default:
                if (pos < n - 1)
                    out[pos++] = *fmt;
                len++;
                break;
        }
        continue;

    unsigned_number : {
        char buf[32];
        char *p = buf + sizeof(buf) - 1;
        static char digits[] = "0123456789abcdef";
        static char DIGITS[] = "0123456789ABCDEF";
        char *digits_to_use = (*fmt == 'X') ? DIGITS : digits;
        size_t number_len;

        *p = '\0';

        do {
            *--p = digits_to_use[unum % base];
            unum /= base;
        } while (unum != 0);

        number_len = buf + sizeof(buf) - 1 - p;

        // Handle padding
        if (width > 0 && width > number_len + neg) {
            size_t padding = width - number_len - neg;
            char pad_char = pad_zero ? '0' : ' ';

            while (padding-- > 0) {
                if (pos < n - 1)
                    out[pos++] = pad_char;
                len++;
            }
        }

        while (*p) {
            if (pos < n - 1)
                out[pos++] = *p;
            len++;
            p++;
        }
    }
        continue;

    number : {
        char buf[32];
        char *p = buf + sizeof(buf) - 1;
        static char digits[] = "0123456789";
        size_t number_len;

        *p = '\0';

        do {
            *--p = digits[num % base];
            num /= base;
        } while (num != 0);

        number_len = buf + sizeof(buf) - 1 - p;

        // Handle padding
        if (width > 0 && width > number_len + neg) {
            size_t padding = width - number_len - neg;
            char pad_char = pad_zero ? '0' : ' ';

            while (padding-- > 0) {
                if (pos < n - 1)
                    out[pos++] = pad_char;
                len++;
            }
        }

        while (*p) {
            if (pos < n - 1)
                out[pos++] = *p;
            len++;
            p++;
        }
    }
    }

    // Ensure null termination
    if (n > 0)
        out[pos < n - 1 ? pos : n - 1] = '\0';

    return len;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
    return vsnprintf(out, (size_t)-1, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vsprintf(out, fmt, ap);
    va_end(ap);

    return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vsnprintf(out, n, fmt, ap);
    va_end(ap);

    return ret;
}

int printf(const char *fmt, ...) {
    char buf[1024];  // Fixed buffer for simplicity
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    // Assuming putstr is defined elsewhere in the AM (Abstract Machine)
    // implementation
    putstr(buf);

    return ret;
}

#endif
