#include <klib-macros.h>
#include <klib.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
    const char *p = s;
    while (*p != '\0')
        p++;
    return p - s;
}

char *strcpy(char *dst, const char *src) {
    char *orig_dst = dst;
    while ((*dst++ = *src++) != '\0')
        ;
    return orig_dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; i++)
        dst[i] = src[i];
    for (; i < n; i++)
        dst[i] = '\0';

    return dst;
}

char *strcat(char *dst, const char *src) {
    char *orig_dst = dst;

    // Find the end of dst
    while (*dst != '\0')
        dst++;

    // Copy src to the end of dst
    while ((*dst++ = *src++) != '\0')
        ;

    return orig_dst;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n-- > 0) {
        if (*s1 != *s2 || *s1 == '\0')
            return *(const unsigned char *)s1 - *(const unsigned char *)s2;
        s1++;
        s2++;
    }
    return 0;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--)
        *p++ = (unsigned char)c;
    return s;
}

void *memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;

    if (d < s) {
        // Copy forward
        while (n--)
            *d++ = *s++;
    } else if (d > s) {
        // Copy backward to handle overlapping memory regions
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }

    return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
    unsigned char *d = (unsigned char *)out;
    const unsigned char *s = (const unsigned char *)in;

    while (n--)
        *d++ = *s++;

    return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    while (n--) {
        if (*p1 != *p2)
            return *p1 - *p2;
        p1++;
        p2++;
    }

    return 0;
}

#endif
