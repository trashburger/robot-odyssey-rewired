#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
// Support for placement new
constexpr void *operator new(size_t, void *p) { return p; }
#endif

#define WASM_EXPORT(n) __attribute__((export_name(#n))) n

static inline void *memset(void *s, int c, size_t n) {
    return __builtin_memset(s, c, n);
}

static inline void *memcpy(void *__restrict dst, const void *__restrict src,
                           size_t n) {
    return __builtin_memcpy(dst, src, n);
}

static inline void *memmove(void *__restrict dst, const void *__restrict src,
                            size_t n) {
    return __builtin_memmove(dst, src, n);
}

[[noreturn]] static inline void abort() { __builtin_trap(); }

static inline size_t strnlen(const char *s, size_t maxlen) {
    const char *end = s;
    const char *limit = s + maxlen;
    while (*end && end < limit)
        end++;
    return (size_t)(end - s);
}

static inline int strcmp(const char *s1, const char *s2) {
    while (1) {
        uint8_t c1 = *s1;
        uint8_t c2 = *s2;
        if (c1 < c2)
            return -1;
        if (c1 > c2)
            return 1;
        if (c1 == 0)
            return 0;
        s1++;
        s2++;
    }
}

static inline int strcasecmp(const char *s1, const char *s2) {
    while (1) {
        uint8_t c1 = *s1;
        uint8_t c2 = *s2;
        if (c1 >= 'a' && c1 <= 'z')
            c1 += 'A' - 'a';
        if (c2 >= 'a' && c2 <= 'z')
            c2 += 'A' - 'a';
        if (c1 < c2)
            return -1;
        if (c1 > c2)
            return 1;
        if (c1 == 0)
            return 0;
        s1++;
        s2++;
    }
}
