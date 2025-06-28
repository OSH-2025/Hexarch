// 简单的字符串函数实现，用于支持 FatFs

#include <stddef.h>

// memset 实现
void* memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    while (n-- > 0) {
        *p++ = (unsigned char)c;
    }
    return s;
}

// memcpy 实现
void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n-- > 0) {
        *d++ = *s++;
    }
    return dest;
}

// memcmp 实现
int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    while (n-- > 0) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

// strchr 实现
char* strchr(const char* s, int c) {
    while (*s != '\0') {
        if (*s == (char)c) {
            return (char*)s;
        }
        s++;
    }
    if (c == '\0') {
        return (char*)s;
    }
    return NULL;
}